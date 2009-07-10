/***************************************************************
  
*/

#ifndef DEBUGGING_H_INCLUDED
#define DEBUGGING_H_INCLUDED

// ---------------------------------------------------------------------------
// Logging / debugging
// ---------------------------------------------------------------------------
//debugging control
#include <wx/log.h>

#define LOGIT wxLogDebug
#if defined(LOGGING)
 #define LOGGING 1
 #undef LOGIT
 #define LOGIT wxLogMessage
 #define TRAP asm("int3")
#endif

//-----Release-Feature-Fix------------------
#define VERSION wxT("1.2.26 2009/01/21")
//------------------------------------------
// Release - Current development identifier
// Feature - User interface level
// Fix     - bug fix or non UI breaking addition

// ----------------------------------------------------------------------------
class Debugging
// ----------------------------------------------------------------------------
{
  public:
    Debugging(const wxString& logtitle);
    ~Debugging();

    wxString GetVersion(){return m_version;}

    #if defined(LOGGING)
     wxLogWindow* GetLog(){return m_pMyLog;}
     wxLog*       GetOldLog(){return m_pOldLog;}
     void         Flush(){m_pMyLog->Flush();}

     wxLogWindow* m_pMyLog;
     wxLog* m_pOldLog;
     static wxLogWindow* pLog;
    #endif

  private:
    Debugging();
    wxString m_version;

};
//extern wxLogWindow* pLog;
#endif // DEBUGGING_H_INCLUDED
