/***************************************************************
Credits:Event Relay and MultitouchSU
License devs ToBeDone
*/

#include "wx/wx.h"
//#include "wx/evtloop.h"

#include "windows.h"

class RubyClassHandler;

#ifdef DLLFUNCTIONS_EXPORTS
    #define DLLFUNCTIONS_API __declspec(dllexport)
#else
    #define DLLFUNCTIONS_API __declspec(dllimport)
#endif

// ----------------------------------------------------------------------------
class MultitouchSUApp : public wxApp
// ----------------------------------------------------------------------------
{
    public:
        MultitouchSUApp();
        ~MultitouchSUApp();

        UINT PostWxEvent( const UINT Message, const WPARAM wParam, const LPARAM lParam);
        bool DllProcessAttach();
        RubyClassHandler* GetRubyClassHandler(){return pRubyClassHandler;}

        static RubyClassHandler* pRubyClassHandler;

    private:

        bool OnInit();
        wxWindow* CreateHiddenFrame();
        bool FindSuMainWindow();
        static BOOL CALLBACK EnumWindowsProc( HWND hWnd, LPARAM lParam);
        static BOOL CALLBACK EnumChildWindowsProc( HWND hWnd, LPARAM lParam);
        UINT VirtualKeyToScanCode(UINT virtualKey);
        UINT ScanCodeToVirtualKey(UINT scanCode);
        wxString GetMSWndMenuLabel(const unsigned menuId);
        bool CreateDebuggingLog();

        wxString m_charMouseEventType;

        //-DECLARE_EVENT_TABLE()
};
// ----------------------------------------------------------------------------
