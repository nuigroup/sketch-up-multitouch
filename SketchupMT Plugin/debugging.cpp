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
// ----------------------------------------------------------------------------
//  Version/History
// ----------------------------------------------------------------------------
// Commit 1.0.0
//  0.0.0 2008/11/5
//      Ruby GetKeyInfo() to show virtual key and macro names
//      Example of ToolsObserver in MultitouchSu.rb script
// ----------------------------------------------------------------------------
// Commit 1.0.10 2008/11/21
//      01) Fixed crash when search name contains no blank past "- SketchUp "
//          and Dll does not load. Use test http://www.sketchucation.com/forums/scf/viewtopic.php?f=180&t=13953&sid=8e41de56299fd1a9a384999431705117#p104599
//          Click on link and use SU to download.
//      02) Fixed crash in DLL_PROCESS_DETACH. Must not return FALSE in DLL
//          initialization after wxApp is allocated.
//      03) Added sanity test in Init_MultitouchSu() to avoid creating RubyClassHandler
//          when DLL failed initialization.
//      04) Added AppObserver and ToolsObserver to MultitouchSu.rb for LMouseButton
//          MoveTool dragging/copy test. Avoids having to depend on Menu IDs and
//          possible conflicts with user RubyTool selections.
//      05) Document changes to Ruby headers necessary to compile with mingw32
//      06) Fixed misplaced menu label length test in GetMSWndMenuLabel()
//      07) Give Ruby Mouse x,y relative to client window
//      08) GetKeyInfo(): if key is vk_letter, show the ascii letter in macro position
//      09) Intial Wiki page with MultitouchSuInfo.rb example.
//      10) Add separate wiki pages: for Copy/Move tool, RightMousePan & F1 intercept
// ----------------------------------------------------------------------------
// Commit 1.1.03 2008/11/23
//      01) Add Timer capability called from Ruby as ERStartTimer(dblSeconds).
//          Added OnERTimerEvent() to Ruby MultitouchSu class.
//      02) add 'require "sketchup.rb' to scripts (Thanks RickW)
//      03) add dll version and ERGetVersion() for ERKbJockey to verify against.
//          Re-establish wxKeyEvent type flags OnKeyDown/OnKeyUp.
//          Translate KeyUp/Down flags to Ruby flags.
//          Backspace for command entry.
//          Change timer to use lowest millisecond reqest.
//          Change documentation for key flags.
//          Rescue crash after calling user command
// ----------------------------------------------------------------------------
// Commit 1.2.24 2009/01/10
//        ) 2008/12/2
//      04) Fix SUPPRESS on some {ENTER}. Move/copy deletes selection on
//          timed out undefined command. eg., v{ENTER}.
//          Text changes in ERKbJockey.rb
//      05) Added ERGetMenuId("menu/path") for Sketchup.send_action(menuId)
//      06) Eliminated problematic menu chars in MSWScanMenuBar() menu building
//      07) Work arounds for SketchUp splats when {ENTER} and selection active.
//      08) Add keyBinder with dialog to define user cmds via menu paths
//          Save keyBinder defined cmds to UserKBCommand.rb
//          Load saved UserKBCommands via ruby in ERKbjockey.rb
//        ) 2008/12/23
//      09) Dont allow shortcut input when no menu tree item selected
//      10) Don't show active menu choosing keystrokes to script
//      11) Move group/component menu initialization just before keybinder config
//        ) 2009/01/7
//      12) Fixed disappearing secondary shortcut commands
//      13) Ignore menu items beginning with a numberic,eg, "2 Groups", "18 items"
//      14) Fix SendMenuActionsNow() to verify that MSW menu item is actually active.
//          Output msg if command incompatible with current selection/active edit menu
//        ) 2009/01/8
//      15) Bring focus/forground to SU in MSWrunmacro.cpp. HiddenFrame menu scans were stealing focus
//      16) Fix wxKeyBinder::Load() to recognize previously loaded wxCmds and append shortcut cmds.
//      17) SU plugin menu item for config
//      19) Changed 14) to invoke main menu instead of wxMessageBox
//        ) 2009/01/9
//      20) Set wx item enabled/disabled status when scanning menus
//      21) Raise menu when command mismatches current menu structure.
//      22) Allow "# Groups (# in model)" etc in menu scanning (as Groups,Components,Entities)
//          when they occur by user selection. Advanced user may want them.
//      23) Disable RButtonDown when initializing KbCmdConfig(). Mouse unselects our
//          Component/Group temp selections.
//      24) Added restart check if KbCmdConfig() did not finish intialization
//
// ----------------------------------------------------------------------------
// Commit 1.2.25 2009/01/10
//      25) Fix: MakeIdFromPath() ssigning same wx id to multiple menu paths.
// ----------------------------------------------------------------------------
// Version 1.2.27 2009/01/26
//      26) Move mouse events back to HiddenFrame. Will help cross platform port.
//      27) KeyBinder: modify to delete unused cmds from UserCommandTable.
//          ERKbJockey: add routine to unregister unused user commands
//      28) 1.2.28 2009/04/23
//          When mismatched menu id, sent !x (where x = lower case) in
//          SendMenuActionsNow(). Invokes main menu items on mismatch between
//          MSW hash table and HiddenWindow subMenu ids's
// ----------------------------------------------------------------------------
// Testing 1.2.28 2009/06/4
//      29) If Keybinder tree item is a sub-menu don't allow key assignment
// ----------------------------------------------------------------------------
//BUGS:
//      Bad flashing when keyBinder dlg is dismissed.
//      Config is allowing user to assign cmds to submenu items, e.g,
//          Tools_Fredo6_Collection_FreeScale. 2009/04/23
// ----------------------------------------------------------------------------
//TODO:
//      Clear shortcut chars in keybinder dlg left when user did not hit add or enter
//      HiddenFrame::MSWScanMenuBar() could use serious refactoring
//      Move mouse routines back to HiddenFrame from EventRely.DLL
//      Documentation/wiki page for keyBinder KBUserCommand usage
//      Add wxIdle call with timer/subclass routine coordinated.
//
//      Cmd show dlg that can sort both columes ascending/decending
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
