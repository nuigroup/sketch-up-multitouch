/***************************************************************
Credits:Event Relay and MultitouchSU
License devs ToBeDone
// -------------------------------------------------------

#if defined(__WXMSW__) //<<---------- This source for MSWindows only ----------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#if wxCHECK_VERSION(2, 8, 0)
    #ifdef wxUSE_UNICODE
    #define _UNICODE
    #define UNICODE
    #endif
    #include <windows.h>
#endif //wxCHECK_VERSION(2, 8, 0)

//-#include "global.h"
#include "debugging.h"
#include "MSWrunmacro.h"
#include "keytables.h"

extern HWND         g_hWndSketchUp;            //handle of SketchUp window
// ----------------------------------------------------------------------------
namespace
// ----------------------------------------------------------------------------
{
// Return @c str as a proper unicode-compatible string
    wxString kmC2U(const char* str)
    {
        #if wxUSE_UNICODE
            return wxString(str, wxConvUTF8);
        #else
            return wxString(str);
        #endif
    }

    // Return multibyte (C string) representation of the string
    wxWX2MBbuf kmU2C(const wxString& str)
    {
        #if wxUSE_UNICODE
            return str.mb_str(wxConvUTF8);
        #else
            return (wxChar*)str.mb_str();
        #endif
    }

}// end namespace
// ----------------------------------------------------------------------------
//      __WXMSW__ windows code
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//-MSW_ProcessMacro::MSW_ProcessMacro( MacroThread* pMacroThread) not threading here
// ^^^  Not using threads in this Ruby/SketchUp extension because the dll,
//      of which this module is a part, runs in the same process as SketchUp.

MSW_ProcessMacro::MSW_ProcessMacro( )
// ----------------------------------------------------------------------------
    :   SHIFT_SCANCODE(MapVirtualKey( VK_SHIFT,   MAPVK_VK_TO_VSC)),
        CTRL_SCANCODE (MapVirtualKey( VK_CONTROL, MAPVK_VK_TO_VSC)),
        ALT_SCANCODE  (MapVirtualKey( VK_MENU,    MAPVK_VK_TO_VSC))
{
    //-m_pMacroThread = pMacroThread; removed threading
	m_nKeyDwell				= 5;				// Default milliseconds between keystrokes
	m_nKeyDownDwell			= 1;				// Default milliseconds before a pressed key is released
	m_nShiftFlags			= NO_FLAGS;		    // Key shifts
	m_bCapsLockState	    = true;			    // Store/restore caps ON

    //#if defined(LOGGING)
    //for (int i=0; i<(int)keyTables.wnKeyCode.GetCount(); ++i )
    //{
    //    LOGIT( _T("Index[%d] VK_KEY[%d] Macro[%s]"),
    //            i, keyTables.wnKeyCode.Item(i), keyTables.wsMacTable.Item(i).c_str());
    //}
    //#endif

}//ProcessMacro() ctor
// ----------------------------------------------------------------------------
MSW_ProcessMacro::~MSW_ProcessMacro()
// ----------------------------------------------------------------------------
{
    //dtor
}//ProcessMacro()
// ----------------------------------------------------------------------------
wxString MSW_ProcessMacro::GetKeySymFromVKey(UINT vkey)
// ----------------------------------------------------------------------------
{
    return keyTables.GetKeySymFromVKey(vkey);
}
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::SendMacroString(wxString wsMacro, wxWindow* pWin)
// ----------------------------------------------------------------------------
{
    #ifdef LOGGING
     wxString msg;
     msg.Printf(wxT("ProcessMacro:SendMacroString[%s]"), wsMacro.c_str() );
     //-WriteLog( msg );
     #if defined(LOGGING)
     LOGIT( _T("[%s]"), msg.c_str());
     #endif
    #endif //LOGGING

	char     macroChar;
	string::size_type macroStrIndex = 0 ;
	bool	 bPreviousCapsLockState = false;
	long     attachError = 0;

	// Focus window to accept keystrokes
	// Dont do the following or {ALT} to activate menu won't work
    //-::SetForegroundWindow( (HWND)pWin->GetHandle());
    //-::SetFocus((HWND)pWin->GetHandle());

    // Set focus to SU, else HiddenFrame can get focus after menu scans
    ::SetFocus(g_hWndSketchUp);
    ::SetForegroundWindow(g_hWndSketchUp);

	// Memorize CapsLock position
	if ( m_bCapsLockState == true )
		bPreviousCapsLockState = DoKeyToggle(VK_CAPITAL, OFF);

    // wait for user to stop leaning on the keyboard so we can type in the macro
    DWORD thisThread = GetCurrentThreadId();
    DWORD winThread = GetWindowThreadProcessId( (HWND)(pWin->GetHandle()), NULL);

    //FIXME: attaching fails and is unnecessary when code used in same process
    bool attachOk = AttachThreadInput(thisThread, winThread, true);
    if (not attachOk)
        attachError = GetLastError();
    BYTE lpKeyState[256]={0};
    BOOL getStateOk = GetKeyboardState( lpKeyState);
    //wxString str(wxChar('.'),256);
    //wxString str;
    //msg.Clear();
    if ( attachOk && getStateOk )
    {
        // -------------------------------------------------------------------
        //// -- ways *not* to hold macro while user sleeps on the keyboard ---
        // -------------------------------------------------------------------
        ////for ( int i = 0 ; i < 255; ++i){
        //    //-if ( lpKeyState[i] & 0x01) continue;  // ignore toggle keys
        //    //-if ( lpKeyState[i] & 0x80 ){          // key down
        //    //-if ( lpKeyState[i]  ) str.Append(msg.Format(wxT("[%d][%x]"),i,lpKeyState[i]));   // key down
        ////    if ( (lpKeyState[VK_CONTROL] | lpKeyState[VK_SHIFT] | lpKeyState[VK_MENU]) & 0x80)
        ////    {   //str = str.Format(wxT("Ctrl Status[%d][%x]"),i,lpKeyState[VK_CONTROL]);
        ////        wxMilliSleep(128);
        ////        getStateOk = GetKeyboardState( lpKeyState);
        ////        if (not getStateOk) break;
        ////       i = 0;
        ////    }//else str.Append('.');
        ////}//for
        ////WriteLog(str);

        // If user is leaning on the keyboard, our chars can't get stuffed into the keyboard.
        // wait for user to release Ctrl &/or Shift &/or Alt

        while ((lpKeyState[VK_CONTROL] | lpKeyState[VK_SHIFT] | lpKeyState[VK_MENU]) & 0x80)
        {
            wxMilliSleep(128);
            getStateOk = GetKeyboardState( lpKeyState);
            if (not getStateOk) break;
        }//while
    }//if
   #ifdef LOGGING
    else
    {
        // Attach always fails when we're operating in the same process
        msg.Printf( _T("SendMacroString: failed attach[%d %ld] getState[%d]"),attachOk, attachError, getStateOk);
        //-WriteLog( msg );
        #if defined(LOGGING)
        //LOGIT( _T("[%s]"), msg.c_str());
        #endif
    }
   #endif
    //FIXME: attaching fails and is unnecessary when code in same process
    attachOk = AttachThreadInput(thisThread, winThread, false);
    if (not attachOk)
    {
        attachError = GetLastError();
        #if defined(LOGGING)
        // Attach always fails when we're operating in the same process
        msg.Printf( _T("SendMacroString: failed AttachThreadInput[%d %ld]"),attachOk, attachError);
        //-WriteLog( msg );
        //LOGIT( _T("[%s]"), msg.c_str());
        #endif

    }

    // translate unicode to ascii
    wxWX2MBbuf cb = kmU2C(wsMacro);
    string asciiString(cb);
    string namedMacroString(cb);

	// Process macro characters and named Macro Keys
	while ( macroStrIndex < asciiString.size() )
	{
	    macroChar = asciiString[macroStrIndex++];

		// check for ALT '!',CTRL '^',SHIFT '+',and named macros '{'
		if ( '!' == macroChar ) { m_nShiftFlags |= SEND_ALT_FLAG;  }
		else
		if ( '^' == macroChar ) { m_nShiftFlags |= SEND_CTRL_FLAG; }
		else
		if ( '+' == macroChar ) { m_nShiftFlags |= SEND_SHIFT_FLAG;}
		else
		if ( '{' == macroChar )
        {   // have a named macro eg., {CTRL DN}, {INSERT ON}, {ENTER} etc.
				string::size_type savedMacroStrIndex = macroStrIndex;
				if  ( not FindChar( '}', asciiString, namedMacroString, macroStrIndex) )
				{
					// missing '}' to end named macro, just send single char
					macroStrIndex = savedMacroStrIndex;
					InjectMacroChar('{', 1);
				}//if
				else
				{   // translate named macro to virtual key and inject to kbd queue
					InjectNamedKeyMacro( namedMacroString );
					m_nShiftFlags &= CLEAR_SEND_FLAGS;
				}//else

        }
        else    // inject a single key to keyboard
        {		InjectMacroChar(macroChar, 1);
				//#ifdef LOGGING
                // WriteLog( _T("SendMacroSting[%c]"),wxChar(macroChar) );
				//#endif //LOGGING
				m_nShiftFlags &= CLEAR_SEND_FLAGS;
        }//else

	} // End While


	// restore CapsLock position
	if ( m_bCapsLockState )
		DoKeyToggle(VK_CAPITAL, bPreviousCapsLockState);

}//SendMacro()
// ----------------------------------------------------------------------------
bool MSW_ProcessMacro::DoKeyToggle(unsigned vkCode, bool requestedState)
// ----------------------------------------------------------------------------
{
    // CAPSLOCK, SCROLL, NUMLOCK, INSERT etc

    bool priorToggleState ;

    // Check requested toggle state
    if ( requestedState == ( priorToggleState = (GetKeyState(vkCode) & ON)))
        return priorToggleState;

	//WriteLog(wxT("DoKeyToggle: vk[%d] bState[%d] priorState[%d]"), vk, bState, priorState);

    // Set requested toggle state
	keybd_event((byte)vkCode, MapVirtualKey(vkCode, 0), 0, 0);
	DoKeyDownDwell();
	keybd_event((byte)vkCode, MapVirtualKey(vkCode, 0), KEYEVENTF_KEYUP, 0);
	DoKeyUpDwell();

	return priorToggleState;

}//DoKeyToggle()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::InjectMacroChar(char macroChar, int count)
// ----------------------------------------------------------------------------
{
    //WriteLog( wxString::Format(wxT("InjectMacroChar[%d][%c]"),count, macroChar) );

    int vkCode;
    if ( -1 != ( vkCode = VkKeyScan( macroChar )) )
    {
        DoKeyShifts(vkCode, SHIFT_DOWN);
        while ( count-- )
            DoKeyInjectEvent((byte)(vkCode & 0xff));
        DoKeyShifts(vkCode, SHIFT_UP);
    }//if
    else
        InjectAsALTxxx( (unsigned short)macroChar);

}//InjectMacroChar()
// ----------------------------------------------------------------------------
void  MSW_ProcessMacro::InjectAsALTxxx(unsigned short macroChar)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("ProcessMacro: InjectAsALTxxx: macroChar[%d]"), macroChar);
    std::string ascBuf;
    std::ostringstream ostream;
    ostream << "ASC 0" << (unsigned short)macroChar;
    ascBuf = ostream.str();
    InjectNamedKeyMacro( ascBuf );
    m_nShiftFlags &= CLEAR_SEND_FLAGS;

}//InjectAsALTxxx()
// ----------------------------------------------------------------------------
bool MSW_ProcessMacro::IsExtendedKey(unsigned key)
// ----------------------------------------------------------------------------
{
    // Check if key should use key_event() KEYEVENTF_EXTENDEDKEY flag
    if ( wxNOT_FOUND == keyTables.wnExtendedKeys.Index( key ) )
		return false;

    return true;

}//IsExtendedKey()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::DoKeyShifts(unsigned vkCode, bool bShiftDown)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("DoKeyShiftsDown:m_nShiftFlags[%d]"), m_nShiftFlags);

	//(pecan 2006/11/17)
	// Low order byte from VkKeyScan(char)
	const unsigned VK_SHIFT_FLAG = 0x0100;
	const unsigned VK_CTRL_FLAG  = 0x0200;
	const unsigned VK_ALT_FLAG   = 0x0400;

    if ( bShiftDown )   // make sure both shift flags not set at same time
    {    if ( (vkCode & VK_CTRL_FLAG)  && (not(m_nShiftFlags & CTRL_DOWN_FLAG)) )
            m_nShiftFlags |= SEND_CTRL_FLAG;

        if ( (vkCode & VK_ALT_FLAG) && (not(m_nShiftFlags & ALT_DOWN_FLAG)) )
            m_nShiftFlags |= SEND_ALT_FLAG;

        if ( (vkCode & VK_SHIFT_FLAG) && (not(m_nShiftFlags & SHIFT_DOWN_FLAG)) )
            m_nShiftFlags |= SEND_SHIFT_FLAG;

        // now set the keyboard key flag
        if ( m_nShiftFlags & SEND_SHIFT_FLAG )
            keybd_event(VK_SHIFT, SHIFT_SCANCODE, 0, 0);
        if ( m_nShiftFlags & SEND_CTRL_FLAG )
            keybd_event(VK_CONTROL, CTRL_SCANCODE, 0, 0);
        if ( m_nShiftFlags & SEND_ALT_FLAG )
            keybd_event(VK_MENU, ALT_SCANCODE, 0, 0);

        // allow time for key processing
        DoKeyDownDwell();
    }
    else //This is a shift up. Remove keyboard shift flags
    {
		if ( m_nShiftFlags & SEND_ALT_FLAG ) 				// ALT required?
			keybd_event(VK_MENU, ALT_SCANCODE, KEYEVENTF_KEYUP, 0);
		if ( m_nShiftFlags & SEND_CTRL_FLAG ) 				// CTRL required?
			keybd_event(VK_CONTROL, CTRL_SCANCODE, KEYEVENTF_KEYUP, 0);
		if ( m_nShiftFlags & SEND_SHIFT_FLAG ) 			// SHIFT required?
			keybd_event(VK_SHIFT, SHIFT_SCANCODE, KEYEVENTF_KEYUP, 0);

        // allow time for key processing
        DoKeyUpDwell();
    }

}//DoKeyShifts()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::DoKeyInjectEvent(unsigned vkCode, bool bKEYEVENTF_EXTENDEDKEY)
// ----------------------------------------------------------------------------
{
    // issue keybd_event() with virtual key code
	DoKeyDownEvent(vkCode, bKEYEVENTF_EXTENDEDKEY);
	DoKeyDownDwell();

	DoKeyUpEvent(vkCode, bKEYEVENTF_EXTENDEDKEY);
	DoKeyUpDwell();

}//DoKeyInjectEvent()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::DoKeyDownEvent(unsigned vkCode, bool bKEYEVENTF_EXTENDEDKEY)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("DoKeyDownEvent vkCode[%d] extended[%d]"), vkCode, bKEYEVENTF_EXTENDEDKEY);

	unsigned scCode = MapVirtualKey(vkCode, 0);
    DWORD dwFlags = ( bKEYEVENTF_EXTENDEDKEY ||  IsExtendedKey(vkCode) ) ? KEYEVENTF_EXTENDEDKEY : 0;
    keybd_event((byte)vkCode, (byte)scCode, dwFlags, 0);

}//DoKeyDownEvent()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::DoKeyUpEvent(unsigned vkCode, bool bKEYEVENTF_EXTENDEDKEY)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("ProcessMacro::DoKeyUpEvent : vkCode[%d] extended[%d]"), vkCode, bKEYEVENTF_EXTENDEDKEY);

	unsigned scCode = MapVirtualKey(vkCode, 0);
    DWORD dwFlags = ( bKEYEVENTF_EXTENDEDKEY  || IsExtendedKey(vkCode) ) ? KEYEVENTF_EXTENDEDKEY : 0;
    keybd_event((byte)vkCode, (byte)scCode, dwFlags | KEYEVENTF_KEYUP, 0);

}//DoKeyUpEvent()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::InjectNamedKeyMacro(string& namedMacroStr)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("InjectNamedKeyMacro:parm[%s]"), cbC2U(namedMacroStr.c_str()).c_str());

    string sNamedMacro    ( "" );
	string sNamedMacroParm( "" );

    //parse input
    std::stringstream istream( namedMacroStr);
    istream >> sNamedMacro ;
    istream >> sNamedMacroParm ;

    // translate macro and parameter to upper case
    transform(sNamedMacro.begin(), sNamedMacro.end(),
                sNamedMacro.begin(), toupper);
    transform(sNamedMacroParm.begin(), sNamedMacroParm.end(),
                sNamedMacroParm.begin(), toupper);

     //WriteLog( _T("InjectNamedKeyMacro: sNamedMacro[%s] sNamedMacroParm[%s]"),
     //   kmC2U(sNamedMacro.c_str()).c_str(), kmC2U(sNamedMacroParm.c_str()).c_str() );

    // valid namedMacroParms are "UP" "DOWN" "DN" "ON" "OFF" or numeric
    enum macroParm
    {
        NoParm = 0,
        UpParm ,
        DownParm,
        OnParm,
        OffParm
    };

    macroParm namedMacroParm = NoParm ;
	int		count       = 1;

	if ( not sNamedMacroParm.empty() )
	{
		if ( 0 ==  sNamedMacroParm.compare("UP"))
			namedMacroParm = UpParm;
		else if ( 0 == sNamedMacroParm.compare("DOWN"))
			namedMacroParm = DownParm;
		else if ( 0 == sNamedMacroParm.compare("DN"))
			namedMacroParm = DownParm;
		else if ( 0 == sNamedMacroParm.compare("ON"))
			namedMacroParm = OnParm ;
		else if ( 0 == sNamedMacroParm.compare("OFF"))
			namedMacroParm = OffParm;
		else
		{
		    istream.clear();
            istream.str( (std::string)sNamedMacroParm );
            istream >> count;
			count = (count <= 0) ? 1 : count ;
		}
	}
    int macIdx = 0;
	// validate key macro
    if ( (macIdx = keyTables.wsMacTable.Index( kmC2U(sNamedMacro.c_str()) ),false) == wxNOT_FOUND)
        macIdx = wxNOT_FOUND;

    // no matching named macro, spit out the first char
	if (macIdx == wxNOT_FOUND)
	{
	    string::iterator iter;
        iter = namedMacroStr.begin();
        while( iter != namedMacroStr.end() )
        {
            char macroChar = *iter & 0xff;
            InjectMacroChar( macroChar, 1);
            ++iter;
        }
	}
	else
	{   // found matching named macro
		if ( keyTables.wnMacType[macIdx] == VKB_KEYSYM)
		{   //VKB_KEYSYM, send as is
            switch ( namedMacroParm )
            {
                case DownParm: DoKeyDownEvent(keyTables.wnKeyCode[macIdx]);   break;
                case UpParm:   DoKeyUpEvent(keyTables.wnKeyCode[macIdx]);     break;
                case OnParm:   DoKeyToggle(keyTables.wnKeyCode[macIdx], ON);  break;
                case OffParm:  DoKeyToggle(keyTables.wnKeyCode[macIdx], OFF); break;
                default:
                {   DoKeyShifts(NO_FLAGS, SHIFT_DOWN);
                    unsigned vkCode = keyTables.wnKeyCode[macIdx] ;
                    while (count--) DoKeyInjectEvent(vkCode );
                    DoKeyShifts(NO_FLAGS, SHIFT_UP);
                    break;
                }//default
            }//switch
		}
		else
		if ( keyTables.wnMacType[macIdx] == VK_MOUSESYM )
		{
		    InjectNamedMouseMacro( namedMacroStr );
        }
		else    //USR_KEYSYM, needs additional processing
        if ( keyTables.wnMacType[macIdx] == USR_KEYSYM)
		{   //USR_KEYSYM, needs additional processing
			int usrKeyCode = keyTables.wnKeyCode[macIdx];
				if ( ALT == usrKeyCode)
				{	if ( not (m_nShiftFlags & ALT_DOWN_FLAG) )
					{   // ALT key needs ALT flag
						m_nShiftFlags |= SEND_ALT_FLAG;
                        switch (namedMacroParm)
                        {   case DownParm: DoKeyDownEvent(VK_MENU); break;
                            case UpParm:   DoKeyUpEvent(VK_MENU);   break;
                            default:       DoKeyInjectEvent(VK_MENU); break;
                        }//switch
					}//if
				}//if
				if ( NUMPADENTER == usrKeyCode)
				{	// Numpad enter is an extednded key
                    switch (namedMacroParm)
                    {   case DownParm: DoKeyDownEvent(VK_RETURN); break;
                        case UpParm:   DoKeyUpEvent(VK_RETURN);   break;
                        default:       DoKeyInjectEvent(VK_RETURN, KEYEVENTF_EXTENDEDKEY); break;
                    }//switch
				}//if
				if ( ALTDOWN == usrKeyCode)
				{	if ( not (m_nShiftFlags & ALT_DOWN_FLAG) )
					{   // ALT key needs ALT flags
						m_nShiftFlags |= SEND_ALT_FLAG;
						m_nShiftFlags |= ALT_DOWN_FLAG;
						DoKeyDownEvent(VK_MENU);
					}
				}//if
				if ( ALTUP == usrKeyCode )
				{	if ( m_nShiftFlags & ALT_DOWN_FLAG)
					{   // ALT key needs ALT flags
						m_nShiftFlags |= SEND_ALT_FLAG;
						m_nShiftFlags ^= ALT_DOWN_FLAG;
						DoKeyUpEvent(VK_MENU);
					}
				}//if
				if ( SHIFTDOWN == usrKeyCode )
				{	if ( not (m_nShiftFlags & SHIFT_DOWN_FLAG) )
					{   //Shift key needs shift flags
						m_nShiftFlags |= SHIFT_DOWN_FLAG;
						DoKeyDownEvent(VK_SHIFT);
					}
				}//if
				if ( SHIFTUP == usrKeyCode)
				{   //Shift key needs shift flags
					if ( m_nShiftFlags & SHIFT_DOWN_FLAG)
					{
						m_nShiftFlags ^= SHIFT_DOWN_FLAG;
						DoKeyUpEvent(VK_SHIFT);
					}
				}
				if ( CTRLDOWN == usrKeyCode )
				{	if ( not (m_nShiftFlags & CTRL_DOWN_FLAG) )
					{   //ctrl key needs ctrl flags
						m_nShiftFlags |= CTRL_DOWN_FLAG;
						DoKeyDownEvent(VK_CONTROL);
					}
				}//if
				if ( CTRLUP == usrKeyCode)
				{	if ( m_nShiftFlags & CTRL_DOWN_FLAG)
					{   //ctrl key needs ctrl flags
						m_nShiftFlags ^= CTRL_DOWN_FLAG;
						DoKeyUpEvent(VK_CONTROL);
					}
				}//if
				if ( ASC == usrKeyCode)
				{	// send ALT 0nnn
					if ( not (m_nShiftFlags & ALT_DOWN_FLAG) )
					{   // Depress ALT key
						keybd_event(VK_MENU, ALT_SCANCODE, 0, 0);
						DoKeyDownDwell();
					}
					// Send numerics
                    string::iterator iter;
                    iter = sNamedMacroParm.begin();
                    while( iter != sNamedMacroParm.end() )
                    {
                        DoKeyInjectEvent( *iter+48);
                        iter++;
                    }
					if ( not (m_nShiftFlags & ALT_DOWN_FLAG) )
					{   // Release ALT key
						keybd_event(VK_MENU, ALT_SCANCODE, KEYEVENTF_KEYUP, 0);
						DoKeyUpDwell();
					}
				}//if
				if ( WAIT == usrKeyCode)
				{   // dow dwell for user milliseconds
				    DWORD waitdword = atoi(sNamedMacroParm.c_str());
				    ::Sleep( waitdword);
				}
        } // End If
	} // End If

}//InjectNamedKeyMacro()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::InjectNamedMouseMacro(string& namedMacroStr)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("InjectNamedKeyMacro:parm[%s]"), cbC2U(namedMacroStr.c_str()).c_str());

    string sNamedMacro    ( "" );
	string sNamedMacroParm( "" );

    //parse input
    std::stringstream istream( namedMacroStr);
    istream >> sNamedMacro ;
    istream >> sNamedMacroParm ;

    // translate macro and parameter to upper case
    transform(sNamedMacro.begin(), sNamedMacro.end(),
                sNamedMacro.begin(), toupper);
    transform(sNamedMacroParm.begin(), sNamedMacroParm.end(),
                sNamedMacroParm.begin(), toupper);

     //WriteLog( _T("InjectNamedKeyMacro: sNamedMacro[%s] sNamedMacroParm[%s]"),
     //   kmC2U(sNamedMacro.c_str()).c_str(), kmC2U(sNamedMacroParm.c_str()).c_str() );

    // valid namedMacroParms are "UP" "DOWN" "DN" "ON" "OFF" or numeric
    enum macroParm
    {
        NoParm = 0,
        UpParm ,
        DownParm,
        OnParm,
        OffParm
    };

    macroParm namedMacroParm = NoParm ;
	int		count       = 1;

	if ( not sNamedMacroParm.empty() )
	{
		if ( 0 ==  sNamedMacroParm.compare("UP"))
			namedMacroParm = UpParm;
		else if ( 0 == sNamedMacroParm.compare("DOWN"))
			namedMacroParm = DownParm;
		else if ( 0 == sNamedMacroParm.compare("DN"))
			namedMacroParm = DownParm;
		else if ( 0 == sNamedMacroParm.compare("ON"))
			namedMacroParm = OnParm ;
		else if ( 0 == sNamedMacroParm.compare("OFF"))
			namedMacroParm = OffParm;
		else
		{
		    istream.clear();
            istream.str( (std::string)sNamedMacroParm );
            istream >> count;
			count = (count <= 0) ? 1 : count ;
		}
	}
    int macIdx = 0;
	// validate key macro
    if ( (macIdx = keyTables.wsMacTable.Index( kmC2U(sNamedMacro.c_str()) ),false) == wxNOT_FOUND)
        macIdx = wxNOT_FOUND;

    // no matching named macro, spit out the first char
	if (macIdx == wxNOT_FOUND)
        return;

	{   // found matching named macro
		if ( keyTables.wnMacType[macIdx] == VK_MOUSESYM)
		{   //VK_MOUSESYM
            switch ( namedMacroParm )
            {
                case DownParm: DoMouseDownEvent(keyTables.wnKeyCode[macIdx]);   break;
                case UpParm:   DoMouseUpEvent(keyTables.wnKeyCode[macIdx]);     break;
                //case OnParm:   DoKeyToggle(keyTables.wnKeyCode[macIdx], ON);  break;
                //case OffParm:  DoKeyToggle(keyTables.wnKeyCode[macIdx], OFF); break;
                default:
                {   //DoKeyShifts(NO_FLAGS, SHIFT_DOWN);
                    //unsigned vkCode = keyTables.wnKeyCode[macIdx] ;
                    //while (count--) DoKeyInjectEvent(vkCode );
                    //DoKeyShifts(NO_FLAGS, SHIFT_UP);
                    break;
                }//default
            }//switch
		}
	} // End If

}//InjectNamedKeyMacro()
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::DoMouseDownEvent(unsigned vkCode)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("DoKeyDownEvent vkCode[%d] extended[%d]"), vkCode, bKEYEVENTF_EXTENDEDKEY);
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    switch (vkCode)
    {
    	case VK_LBUTTON:
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    		break;
    	case VK_MBUTTON:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
    		break;
    	case VK_RBUTTON:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    		break;
    	default:
            return;
    }
    ::SendInput(1,&input,sizeof(input));

}//DoMouseDownEvent
// ----------------------------------------------------------------------------
void MSW_ProcessMacro::DoMouseUpEvent(unsigned vkCode)
// ----------------------------------------------------------------------------
{
	//WriteLog(wxT("DoKeyDownEvent vkCode[%d] extended[%d]"), vkCode, bKEYEVENTF_EXTENDEDKEY);
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    switch (vkCode)
    {
    	case VK_LBUTTON:
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    		break;
    	case VK_MBUTTON:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
    		break;
    	case VK_RBUTTON:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    		break;
    	default:
            return;
    }
    ::SendInput(1,&input,sizeof(input));

}//DoMouseUpEvent
// ----------------------------------------------------------------------------
bool MSW_ProcessMacro::FindChar(char macroChar, const string& inputLine, string& outString, string::size_type& posn)
// ----------------------------------------------------------------------------
{
    bool found = false;
    string::size_type inPosn  = posn;

    string::size_type n  = inputLine.find(macroChar,inPosn);
    outString = inputLine.substr(inPosn, n-inPosn);
    posn = inputLine.length();
    if ( n != string::npos ) {
        found = true;
        posn = ++n;
    }
     //WriteLog( _T("FindChar[%s] inPosn[%d] OutPosn[%d]"),cbC2U(outString.c_str()).c_str(), inPosn, posn );
    return found ;
}
// ----------------------------------------------------------------------------
#endif //(__WXMSW__)
