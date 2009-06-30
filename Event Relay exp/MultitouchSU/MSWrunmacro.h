/***************************************************************
Credits:Event Relay and MultitouchSU
License devs ToBeDone
*/
//////////////////////////////////////////////////////////////////////////////
#if defined(__WXMSW__)
//////////////////////////////////////////////////////////////////////////////

#ifndef PROCESSMACRO_H
#define PROCESSMACRO_H

// layout.h and X11/extensions/XI.h both have "Absolute" defined
//#include <wx/layout.h>

#include <string>
#include <sstream>
using std::string;

//-#include "macrothread.h"
// ----------------------------------------------------------------------------
class MSW_ProcessMacro
// ----------------------------------------------------------------------------
{
public:
	// Functions
	//-MSW_ProcessMacro(MacroThread* pMacroThread);		// Constructor
	MSW_ProcessMacro();		// Constructor
   ~MSW_ProcessMacro();									// Destructor

    void     SendMacroString(wxString wsMacro, wxWindow* pWin);
	void	 SetKeyDwell(int n) { m_nKeyDwell = n; }						// Change delay between keypresses
	void	 SetKeyDownDwell(int n) { m_nKeyDownDwell = n; }				// Change delay between keypresses
	wxString GetKeySymFromVKey(UINT vkey);

	// Variables
	int		m_nKeyDwell;							// Time in between keystrokes
	int		m_nKeyDownDwell;						// Delay after pressing the key down before releasing
	bool	m_bCapsLockState;						// Store/restore capslock state

private:
	// Variables
	wxWindow*       pWin                ;
	int		        m_nShiftFlags     ;
	//-MacroThread*    m_pMacroThread      ;

	const int SHIFT_SCANCODE ;
	const int CTRL_SCANCODE  ;
	const int ALT_SCANCODE   ;

	// Functions
	void	InjectMacroChar(char c, int count);
    void    InjectNamedKeyMacro(string& tempString);
	void	InjectAsALTxxx(unsigned short ch);
	bool	IsExtendedKey(unsigned key);
	bool	DoKeyToggle(unsigned vkCode, bool OnOff);
	void	DoKeyShifts(unsigned vkCode, bool bShiftDown);
	void	DoKeyUpEvent(unsigned vkCode, bool bKEYEVENTF_EXTENDEDKEY = false);
	void	DoKeyDownEvent(unsigned vkCode, bool bKEYEVENTF_EXTENDEDKEY = false);
	void	DoKeyInjectEvent(unsigned vkCode, bool bKEYEVENTF_EXTENDEDKEY = false);
	bool    FindChar(char macroChar, const string& inputLine, string& outString, string::size_type& posn);
	inline void DoKeyUpDwell()   { ::wxMilliSleep(m_nKeyDwell>0?m_nKeyDwell:1); }
	inline void DoKeyDownDwell() { ::wxMilliSleep(m_nKeyDownDwell>0?m_nKeyDownDwell:1); }

    void InjectNamedMouseMacro(string& namedMacroStr);
    void DoMouseDownEvent(unsigned vkCode );
    void DoMouseUpEvent(unsigned vkCode );

    //-void    WriteLog( wxString str ) { m_pMacroThread->WriteLog( str);}
};

// ----------------------------------------------------------------------------
#endif // PROCESSMACRO_H

#endif //defined(__WXMSW__)
