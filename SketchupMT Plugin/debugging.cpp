/***************************************************************
 from EventRelay
 **************************************************************/
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
#include "wx/app.h"
#include "wx/frame.h"
#include "wx/log.h"

#include "debugging.h"

#if defined(LOGGING)
  wxLogWindow* Debugging::pLog = 0;
#endif
// ----------------------------------------------------------------------------
Debugging::Debugging(const wxString& logtitle)
// ----------------------------------------------------------------------------
{

    m_version = VERSION;

    #if defined(LOGGING)
     m_pMyLog = 0;
     m_pOldLog = 0;

        wxWindow* pcbWindow = (wxWindow*)0;
        wxLog::EnableLogging(true);
        //wxLogWindow*
            m_pMyLog = new wxLogWindow(pcbWindow, logtitle, true, false);
        wxFrame* pLogFrame = m_pMyLog->GetFrame();
        m_pMyLog->SetVerbose(TRUE);
        m_pOldLog = wxLog::SetActiveTarget(m_pMyLog);
        m_pMyLog->Flush();
        //-pMyLog->GetFrame()->Move(20,20);
        pLogFrame->SetSize(20,40,600,600);
        wxLogMessage("Logging enabled");
        pLog = m_pMyLog;
	#endif
}
// ----------------------------------------------------------------------------
Debugging::~Debugging()
// ----------------------------------------------------------------------------
{
    //dtor
    #if defined(LOGGING)
        if (m_pMyLog)
        {
            m_pMyLog->Flush();
            wxLog::SetActiveTarget(m_pOldLog);
            //m_pMyLog->GetFrame()->Close(); <== hangs on termination
            m_pMyLog->GetFrame()->Destroy();
        }
    #endif
}

//      Ruby GetKeyInfo() to show virtual key and macro names
//      Example of ToolsObserver in MultitouchSu.rb script