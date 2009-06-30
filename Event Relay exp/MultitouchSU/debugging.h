/***************************************************************
Credits:Event Relay and MultitouchSU
License devs ToBeDone
/*
	This file is part of Eventrelay, a Ruby extension for SketchUp
	Copyright (C) 2008,2009 Pecan Heber

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
