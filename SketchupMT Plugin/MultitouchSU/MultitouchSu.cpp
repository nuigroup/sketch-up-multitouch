/***************************************************************
credits info etc.
 **************************************************************/
/*
	Lincense tbd
*/

// ----------------------------------------------------------------------------
// Changes to Ruby config.h to compile with MINGW32 compiler
// ----------------------------------------------------------------------------
//
// If you have the following lines at the top of \lib\ruby\1.8\i386-mswin32\config.h
// comment them out.
//
// //conficting definition with mingw
// //#if _MSC_VER != 1200
// //#error MSC version unmatch
// //#endif
//
// In config.h find the line "#define HAVE_FSYNC 1" and change it to
//
// //conficting definition with mingw
// #if !defined(HAVE_FSYNC)
//     #define HAVE_FSYNC 1
// #endif
//
// Add HAVE_ISINF to the compiler defines
//

#include "wx/wx.h"
#include "wx/msw/private.h"

#include "MultitouchSU.h"
#include "RubyClass.h"
#include "HiddenFrame.h"
#include "Debugging.h"
#include "TuioSuDump.h"
#include "MtSuGlobals.h"

// ----------------------------------------------------------------------------
// Prototypes and definitions
// ----------------------------------------------------------------------------
#define MAPVK_VK_TO_VSC 0
#define MAPVK_VSC_TO_VK 1

// ----------------------------------------------------------------------------
//  Globals
// ----------------------------------------------------------------------------
int     g_DllIsInitialized = 0;                 //guard while initializing
char*   g_lpszTitleToFind = " - SketchUp";      //Title of window to find
char*   g_lpszClassToFind = "AfxFrameOrView";   //Class of View window
HWND    g_hWndSketchUp = 0;                     //handle of SketchUp window
HWND    g_hWndSketchUpView = 0;                 //handle of SketchUp View subwindow
HWND    g_hWndSketchUpStatusBar = 0;            //handle of SketchUp StatusBar
DWORD   g_dwCurrentProcessId = 0;               //PID of SketchUp process
LONG    OldWndProc = 0;                         //Main windoww proc that we subclassed
//-wxString g_wxStringMsg;                         //global string storage
enum    {g_enumMtSuUserMsgNum = WM_USER+0x8001};//MultiTouchSu WM_USER message id
UINT    g_MtSuUserMsgNum = g_enumMtSuUserMsgNum;//variable of above

structMtSuTuioData g_MtSuTuioData;

// ----------------------------------------------------------------------------
//  pointers to instantiations of classes used by Dll
// ----------------------------------------------------------------------------
//  pointer to one and only RubyClass object
RubyClassHandler* MultitouchSUApp::pRubyClassHandler = 0;
class HiddenFrame;
MultitouchSUApp*  pApp = 0;             //"this" pointer for MultitouchSUApp class calls
HiddenFrame*    pHiddenFrame = 0;       //wxWidgets frame
Debugging*      pMyLog = 0;             //debugging log
wxLog*          pOldLog = 0;            //log to restore on exit
Debugging*      pVersion = 0;           //access to version string

    int port = 3333;
	TuioDump dump;
	TuioClient client(port);

// ----------------------------------------------------------------------------
// We use IMPLEMENT_APP_NO_MAIN so we can start the app from DllMain
// as we don't have a WinMain or main entry point
//
    IMPLEMENT_APP_NO_MAIN(MultitouchSUApp)
    // ----------------------------------------------------------------------------
    //  Events intercept table not used
    // ----------------------------------------------------------------------------
    //    BEGIN_EVENT_TABLE(MultitouchSUApp, wxApp)
    //            //EVT_BUTTON(123,MultitouchSUApp::OnButton) //testing only
    //            //EVT_CLOSE(MyEvents::OnClose) unused; CB wizard generated
    //    END_EVENT_TABLE()

// ----------------------------------------------------------------------------
//  DllMain
// ----------------------------------------------------------------------------
extern "C"
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
// ----------------------------------------------------------------------------
{
    switch (ul_reason_for_call)
    {

        case DLL_PROCESS_ATTACH:
        {
            if ( g_DllIsInitialized ++ )
                return true; //Initialize only once per process

            wxSetInstance((HINSTANCE)hModule);

            //MessageBox( 0, "This is Dll MultitouchSU initializing", "MultitouchSU", MB_OK);
            int argc = 0;
            char **argv = NULL;

            #if defined(LOGGING)
            clock_t initStartTicks = clock();
            #endif

            if (not wxEntryStart(argc, argv))
            {
                MessageBox( 0, "MultitouchSU:wxEntryStart did not initialize", "MultitouchSU", MB_OK);
                return FALSE;
            }
            if ( !wxTheApp || !wxTheApp->CallOnInit() ) // calls OnInit() below
            {
                MessageBox( 0, "MultitouchSU:wxTheApp did not initialize", "MultitouchSU", MB_OK);
                return FALSE;
            }

            // wxTheApp is allocated, we must not return a FALSE
            // or we'll crash during DLL_PROCESS_DETACH
            if ((not pApp) || (not pApp->DllProcessAttach()) )
            {
                MessageBox( 0, "MultitouchSU:DLL_PROCESS_ATTACH did not initialize", "MultitouchSU", MB_OK);
                //return FALSE; <<-- causes crash in DLL_PROCESS_DETACH/wxEntryCleanup()
                return TRUE;
                // Although we return TRUE, the g_DllIsInitialized has not been
                // updated. So hooks and subclassing are disabled.
            }

            //allow work to proceed
            g_DllIsInitialized += 1;

            #if defined(LOGGING)
            LOGIT( _T("Time to initialize[%lu]"), clock() - initStartTicks);
            #endif
            break;

        }//DLL_PROCESS_ATTACH

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
        // The MSDN docs say to *not* deallocate on pgm termination. Just
        // to let the system reclaim the memory.
        //if ( 0 == lpReserved)  //Dll being explicitly unloaded
        do{
            #if defined(LOGGING)
                if (pMyLog)
                {   pMyLog->GetLog()->Flush();
                    //wxLog::SetActiveTarget(pMyLog->GetOldLog());
                    pMyLog->GetLog()->SetActiveTarget(NULL);
                    wxLog::SetActiveTarget(NULL);
                    pMyLog->GetLog()->GetFrame()->Destroy();
                    //-pMyLog->GetFrame()->Close();   <== worthless call
                    //wxLog::SetActiveTarget(NULL);
                    pMyLog = 0;
                }
            #endif

            wxTheApp->OnExit();
            //-wxTheApp->CleanUp(); causes crash deleting objects
            //-wxEntryCleanup(); causes crash deleting objects

        }while(0);
        break;
    }
    return TRUE;
}
// ----------------------------------------------------------------------------
extern "C" DLLFUNCTIONS_API void Init_MultitouchSU(void)
// ----------------------------------------------------------------------------
{//This routine must remain global extern "C" or it won't get called

    // This init function is called by Ruby when this Dll is loaded
    // (Ruby calls it an extension).
    // as.../SketchUp/Plugins/MultitouchSU.dll

    // A simple MultitouchSU.rb contains a 'require "MultitouchSU.dll"' statement

    // This function is invoked *after* DLL initialization DLL_PROCESS_ATTACH
    // and wxWidgets OnInit() routine.

    // sanity tests
    if (g_DllIsInitialized < 2) return;
    //-if (0 == pHiddenFrame) return;

    // Create RubyClassHandler to handle calls from the user script
    // and for callbacks to the ruby script
    // Set some globals needed by the RubyClassHandler
    MultitouchSUApp::pRubyClassHandler = new RubyClassHandler;

    if (pHiddenFrame)
        ((HiddenFrame*)pHiddenFrame)->SetRubyClassHandler(MultitouchSUApp::pRubyClassHandler);

    if(MultitouchSUApp::pRubyClassHandler)
    {
        #if defined(LOGGING)
        LOGIT( _T("Init_MultitouchSU invoked from Ruby 'require'"));
        #endif
    }
    return;
}
// ----------------------------------------------------------------------------
MultitouchSUApp::MultitouchSUApp()
// ----------------------------------------------------------------------------
{
    //ctor
    pApp = this;
}
// ----------------------------------------------------------------------------
MultitouchSUApp::~MultitouchSUApp()
// ----------------------------------------------------------------------------
{
    //dtor
    ;
}
// ----------------------------------------------------------------------------
bool MultitouchSUApp::OnInit()
// ----------------------------------------------------------------------------
{
    // Called by wxApp->CallOnInit;
    // return true here or the wxApp will exit and we won't cleanup
    // properly when DLL_PROCESS_DETACH is called.

    pApp = this;

    return true;
}
// ----------------------------------------------------------------------------
bool MultitouchSUApp::DllProcessAttach()
// ----------------------------------------------------------------------------
{
    // Called by DLL_PROCESS_ATTACH and
    // called first; before Ruby's Init_MultitouchSU() routine

    pRubyClassHandler = 0;

    #if defined(LOGGING)
        //Create a dummy hidden window so wxWidgets behaves
        if (not CreateHiddenFrame() )
        {
            MessageBox( 0, "MultitouchSU: failed to create internal window", "MultitouchSU", MB_OK);
            return FALSE;
        }

        CreateDebuggingLog();
    #endif

    if (pMyLog)
        pVersion = pMyLog;
    else
        pVersion = new Debugging("NoLogging");

    // Find main program window and subclass it. We want to see user
    // events before SketchUp does.
    if (SubClassMainWindow(0))
    {
            #if defined(LOGGING)
            LOGIT( _T("MultitouchSu Hooking & Creation of SubClassing Done."));
            #endif
    }
    else
    {
        MessageBox( 0, "MultitouchSu:Creation of SubClassing failed", "MultitouchSu", MB_OK);
        return false;
    }

    if (FindSuMainWindow())
    {

        // Startup TUIO client to listen for touch data

        //int port = 3333;
        //TuioDump dump;
        //TuioClient client(port);
        client.addTuioListener(&dump);
        client.connect(false); //non-blocking connection
        return true;
    }

    MessageBox( 0, "MultitouchSU:FindSuMainWindow() failed", "MultitouchSU", MB_OK);
    return false;

}
// ----------------------------------------------------------------------------
LRESULT CALLBACK NewWndProc(HWND Hwnd, UINT message, WPARAM wParam, LPARAM lParam)
// ----------------------------------------------------------------------------
{
    // This is the windows message loop that gets to see WM_messages
    // just before SketchUp does.

    LRESULT rc = 0;

    // Writing to the log here during our initialization causes crashes
    if (g_DllIsInitialized < 2)
         goto ContinueProcessing;

    switch(message)
    {
        case g_enumMtSuUserMsgNum:  //WM_USER message
        {
            rc = true;
            if( MultitouchSUApp::pRubyClassHandler )
            {
                MultitouchSUApp::pRubyClassHandler->Call_Ruby_OnTuioDataString(g_MtSuTuioData.packetMsg);
                MultitouchSUApp::pRubyClassHandler->Call_Ruby_OnTuioData(g_MtSuTuioData);
            }
            break;
        }
        default: break;
    }//switch

    if (rc)
    {
        #if defined(LOGGING_TUIO)
        LOGIT( _T("NewWndProc PROCCESSED[%s]"), g_MtSuTuioData.packetMsg.c_str());
        #endif
        return true; //message was processed
    }

    ContinueProcessing: // Pass the message onto SketchUp
    return CallWindowProc((WNDPROC)OldWndProc, Hwnd, message, wParam, lParam);
}
    //Doc: WM_COMMAND
    //  Message     wParam      wParam                          lParam
    //  Source      high word)	(low word)
    //  ---------   ----------  -----------------------         -------------
    //  Menu	    0	        Menu identifier (IDM_*)	        0
    //  Accelerator	1	        Accelerator identifier (IDM_*)	0
    //  Control	    Control-    Control                         Handle to the
    //              defined     identifier                      control window
    //              notification
    //              code


// ----------------------------------------------------------------------------
BOOL CALLBACK MultitouchSUApp::EnumWindowsProc( HWND hWnd, LPARAM lParam)
// ----------------------------------------------------------------------------
{

    // Look for a window named in variable g_lpszTitleToFind.
    // It must share our Pid, ie., be in the same process as this
    // routine. This is how we find the correct SketchUp window when
    // the system is running multiple SketchUp processes.

    // Do not depend on the return value. We return FALSE to stop
    // further callbacks after we've found the target window

    DWORD EnumWindowProcessId;
    GetWindowThreadProcessId( hWnd, &EnumWindowProcessId);
    if ( EnumWindowProcessId == g_dwCurrentProcessId )
    {   // Found one of our windows

        int nLen = GetWindowTextLength(hWnd);
        if (nLen)
        {
            LPSTR lpszTitle = new CHAR[nLen + 2];
            GetWindowText(hWnd, lpszTitle, nLen+1);
            if ( strstr( lpszTitle, g_lpszTitleToFind) )
            {
                g_hWndSketchUp = hWnd;
                #if defined(LOGGING)
                LOGIT( _T("Found target window %s"), lpszTitle);
                #endif
                EnumChildWindows(g_hWndSketchUp, EnumChildWindowsProc, 0);
                return FALSE; //done
            }
            #if defined(LOGGING)
            LOGIT( _T("Found window %s"), lpszTitle);
            #endif
            delete [] lpszTitle;
        }
    }
    return TRUE;
}
// ----------------------------------------------------------------------------
BOOL CALLBACK MultitouchSUApp::EnumChildWindowsProc( HWND hWnd, LPARAM lParam)
// ----------------------------------------------------------------------------
{
    // Find the first child window w/class AfxFrameOrView. This
    // will be the users drawing window and the one needed to
    // translate screen coordinates to client coordinates.

    // Look for a window with class name in g_lpszClassToFind.
    // It must share our Pid, ie., be in the same process as this
    // routine. This is how we find the correct SketchUp window when
    // the system is running multiple SketchUp processes.

    // Do not depend on the return value. We return FALSE to stop
    // further callbacks after we've found the target window

    DWORD EnumWindowProcessId;
    GetWindowThreadProcessId( hWnd, &EnumWindowProcessId);
    if ( EnumWindowProcessId == g_dwCurrentProcessId )
    {   // Found one of our child windows

        TCHAR lpszClassName[80] = {0};
        TCHAR lpszTitle[80] = {0};
        int nLen = GetWindowTextLength(hWnd);
        if (nLen)
            GetWindowText(hWnd, lpszTitle, nLen);
        nLen = ::GetClassName(hWnd, lpszClassName, 20);
        int id = GetDlgCtrlID(hWnd);
        if ( id ) {;}
        if ( nLen && strstr( lpszClassName, g_lpszClassToFind) )
        {
            g_hWndSketchUpView = hWnd;
            #if defined(LOGGING)
            LOGIT( _T("Found target window Title[%s] class[%s]"), lpszTitle, lpszClassName);
            #endif
            //-return FALSE; //done
        }
        if ( nLen && strstr( lpszClassName, _T("statusbar")) )
        {
            g_hWndSketchUpStatusBar = hWnd;
            #if defined(LOGGING)
            LOGIT( _T("Found StatusBar Title[%s] class[%s]"), lpszTitle, lpszClassName);
            #endif
            //-return FALSE; //done
        }
        #if defined(LOGGING)
        LOGIT( _T("Found CHILD window Title[%s] class[%s] ID[%d]"), lpszTitle, lpszClassName, id);
        #endif
    }
    return TRUE;
}
// ----------------------------------------------------------------------------
bool MultitouchSUApp::FindSuMainWindow()
// ----------------------------------------------------------------------------
{

    // Place our MS message loop ahead of the SketchUp window so we get
    // to see the window messages before SketchUp does.

    // Get our pid to match against enumerated windows (in case of multiple same named wins)
    g_dwCurrentProcessId = GetCurrentProcessId();
    EnumWindows( EnumWindowsProc, 0);
    HWND hWnd = g_hWndSketchUp;
    if (not hWnd)
    {
        wxMessageBox(wxString::Format(_T("MultitouchSU:Failed to locate %s"), g_lpszTitleToFind));
        return FALSE;
    }

    return TRUE;
}
// ----------------------------------------------------------------------------
bool MultitouchSUApp::SubClassMainWindow(LPVOID)
// ----------------------------------------------------------------------------
{

    // Place our MS message loop ahead of the SketchUp window so we get
    // to see the window messages before SketchUp does.

    // Get our pid to match against enumerated windows (in case of multiple same named wins)
    g_dwCurrentProcessId = GetCurrentProcessId();
    EnumWindows( EnumWindowsProc, 0);
    HWND hWnd = g_hWndSketchUp;
    if (not hWnd)
    {
        wxMessageBox(wxString::Format(_T("MultitouchSu:Failed to locate %s"), g_lpszTitleToFind));
        return FALSE;
    }

    //subclass the original window procedure with our new one
    ::EnableWindow(hWnd, false);
    OldWndProc = SetWindowLong(hWnd, GWL_WNDPROC, (long)NewWndProc);
    ::EnableWindow(hWnd, true);

     //Now all messages sent to the original pgms window, are sent to
     // OUR window proc first.
    return TRUE;
}
// ----------------------------------------------------------------------------
wxWindow* MultitouchSUApp::CreateHiddenFrame()
// ----------------------------------------------------------------------------
{
    // Create a wxWidgets hidden frame else wxWidgets behaves badly

    pHiddenFrame = new HiddenFrame(0, -1, "HiddenFrame");
    SetTopWindow(pHiddenFrame);

    if (not pHiddenFrame )
    {
        return 0;
    }

	return pHiddenFrame;
}
// ----------------------------------------------------------------------------
bool MultitouchSUApp::CreateDebuggingLog()
// ----------------------------------------------------------------------------
{
    // Create debugging log window

    // Testing: create a log window for debugging messages
    if (pMyLog) return true;
    pMyLog = new Debugging(_T("MultitouchSU Dll Log"));
    #if defined(LOGGING)
      LOGIT( "DLL Debug Logging initialized.");
	#endif

	return TRUE;
}
//// ----------------------------------------------------------------------------
//UINT MultitouchSUApp::PostWxEvent( const UINT Message, const WPARAM wParam, const LPARAM lParam)
//// ----------------------------------------------------------------------------
//{
//    // Post an event to the hidden wxWidgets window which will parse it.
//    // If it's an event of interest, it will get passed on to the Ruby class
//    // user.
//
//    POINT point;
//    point.x = 0;
//    point.y = 0;
//    WXMSG wxmsg;
//    wxmsg.hwnd    = 0;
//    wxmsg.message = Message;
//    wxmsg.wParam  = wParam;
//    wxmsg.lParam  = lParam;
//    wxmsg.time    = 0;
//    wxmsg.pt      = point;
//    LRESULT result;
//
//    if (not pHiddenFrame) return FALSE;
//
//    wxCommandEvent event;
//    pHiddenFrame->SetProcessed( event, false);
//    HWND myWinHwnd = (HWND)(pHiddenFrame->GetHandle());
//    result = SendMessage( myWinHwnd, wxmsg.message, wxmsg.wParam, wxmsg.lParam);
//
//    //Doc: result = pHiddenFrame->MSWWindowProc( Message, wParam, lParam);
//    #if defined(LOGGING)
//        //LOGIT( _T("PostWxEvent Processed[%d]as[%d][%s]rc[%ld]"), Message, wxmsg.message, pHiddenFrame->GetProcessed() ? _T("TRUE"):_T("FALSE"), result);
//    #endif
//
//    if ( pHiddenFrame->GetProcessed() )
//        return true;
//
//    return false;
//}//PostWxEvent
// ----------------------------------------------------------------------------
UINT MultitouchSUApp::VirtualKeyToScanCode(UINT virtualKey)
// ----------------------------------------------------------------------------
{
    return MapVirtualKey( virtualKey,  MAPVK_VK_TO_VSC);
    //return MapVirtualKeyEx( virtualKey,  MAPVK_VK_TO_VSC, 0);
}//VirtualKeyToScanCode
// ----------------------------------------------------------------------------
UINT MultitouchSUApp::ScanCodeToVirtualKey(UINT scanCode)
// ----------------------------------------------------------------------------
{
    return MapVirtualKey( scanCode,  MAPVK_VSC_TO_VK);
}//VirtualKeyToScanCode

    // The MapVirtualKeyEx function translates (maps) a virtual-key code into a
    // scan code or character value, or translates a scan code into a virtual-key code.
    // The function translates the codes using the input language and an input locale
    // identifier.
    //
    // Syntax
    //
    //    UINT MapVirtualKeyEx(
    //        UINT uCode,
    //        UINT uMapType,
    //        HKL dwhkl
    //    );
    //
    // Parameters
    //
    //    uCode
    //      [in] Specifies the virtual-key code or scan code for a key. How this
    //      value is interpreted depends on the value of the uMapType parameter.
    //    uMapType
    //        [in] Specifies the translation to perform. The value of this parameter
    //        depends on the value of the uCode parameter.
    //
    //        MAPVK_VK_TO_VSC
    //            The uCode parameter is a virtual-key code and is translated into
    //            a scan code. If it is a virtual-key code that does not distinguish
    //            between left- and right-hand keys, the left-hand scan code is returned. If there is no translation, the function returns 0.
    //        MAPVK_VSC_TO_VK
    //            The uCode parameter is a scan code and is translated into a virtual-key
    //            code that does not distinguish between left- and right-hand keys.
    //            If there is no translation, the function returns 0.
    //
    //            Windows Vista and later: The high byte of the uCode value can contain
    //            either 0xe0 or 0xe1 to specify the extended scan code.
    //        MAPVK_VK_TO_CHAR
    //            The uCode parameter is a virtual-key code and is translated into
    //            an unshifted character value in the low order word of the return value.
    //            Dead keys (diacritics) are indicated by setting the top bit of the
    //            return value. If there is no translation, the function returns 0.
    //        MAPVK_VSC_TO_VK_EX
    //            Windows NT/2000/XP and later: uCode is a scan code and is translated
    //            into a virtual-key code that distinguishes between left- and
    //            right-hand keys. If there is no translation, the function returns 0.
    //
    //            Windows Vista and later: The high byte of the uCode value can contain
    //            either 0xe0 or 0xe1 to specify the extended scan code.
    //        MAPVK_VK_TO_VSC_EX
    //            Windows Vista and later: The uCode parameter is a virtual-key code
    //            and is translated into a scan code. If it is a virtual-key code that
    //            does not distinguish between left- and right-hand keys, the left-hand scan code is returned. If the scan code is an extended scan code, the high byte of the uCode value can contain either 0xe0 or 0xe1 to specify the extended scan code. If there is no translation, the function returns 0.
    //
    //    dwhkl
    //        [in] Input locale identifier to use for translating the specified code.
    //        This parameter can be any input locale identifier previously returned
    //        by the LoadKeyboardLayout function.
    //
    // Return Value
    //
    //    The return value is either a scan code, a virtual-key code, or a character value,
    //    depending on the value of uCode and uMapType. If there is no translation,
    //    the return value is zero.
    //
    // Remarks
    //
    //    The input locale identifier is a broader concept than a keyboard layout,
    //    since it can also encompass a speech-to-text converter, an Input Method
    //    Editor (IME), or any other form of input.
    //
    //    An application can use MapVirtualKeyEx to translate scan codes to the
    //    virtual-key code constants VK_SHIFT, VK_CONTROL, and VK_MENU, and vice versa.
    //    These translations do not distinguish between the left and right instances
    //    of the SHIFT, CTRL, or ALT keys.
    //
    //    Windows NT/2000/XP: An application can get the scan code corresponding to
    //    the left or right instance of one of these keys by calling MapVirtualKeyEx
    //    with uCode set to one of the following virtual-key code constants.
    //
    //        * VK_LSHIFT
    //        * VK_RSHIFT
    //        * VK_LCONTROL
    //        * VK_RCONTROL
    //        * VK_LMENU
    //        * VK_RMENU
    //
    //    These left- and right-distinguishing constants are available to an application
    //    only through the GetKeyboardState, SetKeyboardState, GetAsyncKeyState,
    //    GetKeyState, MapVirtualKey, and MapVirtualKeyEx functions. For list complete
    //    table of virtual key codes, see Virtual-Key Codes.
    //
// ----------------------------------------------------------------------------
wxString MultitouchSUApp::GetMSWndMenuLabel(const unsigned menuId)
// ----------------------------------------------------------------------------
{
    // Get label from MS window menu item (not wxWindow menu)

    wxString menuLabel("");
    HMENU hMenu = GetMenu(g_hWndSketchUp);
    int iMenuItemCount = GetMenuItemCount( hMenu );

    if ( (0 == iMenuItemCount) or (-1 == iMenuItemCount) )
        return menuLabel;

        TCHAR* zBuffer = 0;
        bool rc = false;
    do{
        // Get Item Text length
        MENUITEMINFO mii;
        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof (MENUITEMINFO);
        mii.fMask = MIIM_STRING | MIIM_ID;
        mii.dwTypeData = NULL;

        rc = GetMenuItemInfo (hMenu, menuId, false, &mii);
        if ( false == rc) break;
        if ( 0 == mii.cch ) break; //label length is 0

        // allocate buffer from length
        TCHAR* zBuffer = new TCHAR[mii.cch+1];

        // Get menu label itself
        MENUITEMINFO mii2;
        memset(&mii2, 0, sizeof(mii));
        mii2.cbSize = sizeof (MENUITEMINFO);
        mii2.fMask = MIIM_STRING;
        mii2.dwTypeData = zBuffer;
        mii2.cch = mii.cch+1;

        rc = GetMenuItemInfo (hMenu, menuId, false, &mii2);
        if ( false == rc) break;

        menuLabel = zBuffer;

    }while(0);

    if (zBuffer) delete [] zBuffer;

    return menuLabel;
}
    //Doc
    //    typedef struct tagMENUITEMINFO {
    //      UINT      cbSize;
    //      UINT      fMask;
    //      UINT      fType;
    //      UINT      fState;
    //      UINT      wID;
    //      HMENU     hSubMenu;
    //      HBITMAP   hbmpChecked;
    //      HBITMAP   hbmpUnchecked;
    //      ULONG_PTR dwItemData;
    //      LPTSTR    dwTypeData;
    //      UINT      cch;
    //      HBITMAP   hbmpItem;
    //    } MENUITEMINFO, *LPMENUITEMINFO;
// ----------------------------------------------------------------------------
