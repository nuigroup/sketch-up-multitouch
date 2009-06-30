/***************************************************************
Credits:Event Relay and MultitouchSU
License devs ToBeDone
*/

// Test program to drive the MultitouchSu Ruby extension Dll

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include <iostream>
using namespace std;

#include "MultitouchSuTestFrame.h"
#include "Debugging.h"
#include "RubyEval.h"
#include "ruby.h"

// ----------------------------------------------------------------------------
//  MultitouchSu Test App
// ----------------------------------------------------------------------------
IMPLEMENT_APP(MultitouchSuTestApp);

// ----------------------------------------------------------------------------
bool MultitouchSuTestApp::OnInit()
// ----------------------------------------------------------------------------
{
    MultitouchSuTestFrame* frame = new MultitouchSuTestFrame(0L, _("Fake - SketchUp "));
    frame->SetIcon(wxICON(aaaa)); // To Set App Icon
    frame->Show();
    frame->Move(40,800);

    return true;
}
// ----------------------------------------------------------------------------
//helper functions
// ----------------------------------------------------------------------------
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}
// ----------------------------------------------------------------------------
//  Events Table
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MultitouchSuTestFrame, wxFrame)
    EVT_CLOSE(MultitouchSuTestFrame::OnClose)
    EVT_MENU(idMenuQuit, MultitouchSuTestFrame::OnQuit)
    EVT_MENU(idMenuAbout, MultitouchSuTestFrame::OnAbout)
    EVT_MENU(idMenuTest1, MultitouchSuTestFrame::OnTest)
    EVT_MENU(idMenuTest2, MultitouchSuTestFrame::OnTest)
    EVT_MENU(idMenuTest3, MultitouchSuTestFrame::OnTest)
    EVT_MENU(idMenuTest4, MultitouchSuTestFrame::OnTest)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
MultitouchSuTestFrame::MultitouchSuTestFrame(wxFrame *frame, const wxString& title)
// ----------------------------------------------------------------------------
    : wxFrame(frame, -1, title)
{
#if wxUSE_MENUS
    // create a menu bar
    wxMenuBar* mbar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tAlt-F4"), _("Quit the application"));
    mbar->Append(fileMenu, _("&File"));

    wxMenu* helpMenu = new wxMenu(_T(""));
    helpMenu->Append(idMenuAbout, _("&About\tF1"), _("Show info about this application"));
    mbar->Append(helpMenu, _("&Help"));

    wxMenu* testMenu = new wxMenu(_T(""));
    testMenu->Append(idMenuTest1, _("1 DLL - Run MultitouchSuTest.rb"), _("Ruby Init"));
    testMenu->Append(idMenuTest2, _("2 Run - ClassTest.rb"), _("Ruby Class"));
    testMenu->Append(idMenuTest3, _("3 Exit Ruby"), _("Ruby Class"));
    mbar->Append(testMenu, _("&Test"));

    SetMenuBar(mbar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar with some information about the used wxWidgets version
    CreateStatusBar(2);
    SetStatusText(_("Hello Code::Blocks user!"),0);
    SetStatusText(wxbuildinfo(short_f), 1);
#endif // wxUSE_STATUSBAR

    m_pLogging = new Debugging(_T("MultitouchSu Testing Log"));
    // Move this log, so it isn't hidden behind DLL log
    m_pLogging->m_pMyLog->GetFrame()->SetSize(4,4,600,400);
}


// ----------------------------------------------------------------------------
MultitouchSuTestFrame::~MultitouchSuTestFrame()
// ----------------------------------------------------------------------------
{
}
// ----------------------------------------------------------------------------
void MultitouchSuTestFrame::OnClose(wxCloseEvent &event)
// ----------------------------------------------------------------------------
{
    if ( m_pLogging )
        delete m_pLogging;
    m_pLogging = 0;
    Destroy();
}
// ----------------------------------------------------------------------------
void MultitouchSuTestFrame::OnQuit(wxCommandEvent &event)
// ----------------------------------------------------------------------------
{
    Close();
}
// ----------------------------------------------------------------------------
void MultitouchSuTestFrame::OnAbout(wxCommandEvent &event)
// ----------------------------------------------------------------------------
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}
// ----------------------------------------------------------------------------
void MultitouchSuTestFrame::OnTest(wxCommandEvent &event)
// ----------------------------------------------------------------------------
{
    ostream& errout = cout;
    wxString msg = wxbuildinfo(long_f);
    //wxMessageBox(msg, _("Welcome to..."));

    int TestId = event.GetId();
    #if defined(LOGGING)
    LOGIT( _T("onTest[%d]"), TestId);
    #endif

    // Embed Ruby
    RubyEval& ruby = *RubyEval::instance();

    switch(TestId)
    {
        case idMenuTest1:
        {
            // You've painted up your lips And rolled and curled your tinted hair
            // Ruby are you contemplating going out somewhere

            ruby.eval("puts 'Oh Ruby.. For gods sake turn around'");
            assert( NUM2INT( ruby.eval("1+1") ) == 2 );
            assert(RubyEval::val2str(ruby.eval("'Regexp'.gsub(/x/, 'X')")) == "RegeXp");
            //-ruby.run_file("garbage.rb", errout);
            // Require MultitouchSu.dll & define the Ruby class
            // This .rb will load the DLL with 'Require "MultitouchSu.Dll" '
            ruby.run_file("MultitouchSuTest.rb", errout);
            break;
        }//case1

        case idMenuTest2:
        {   // Use a test.rb to drive the MultitouchSu Ruby class from Ruby itself
            // The ruby Class is declared by the DLL in RubyClass.cpp
            ruby.run_file("ClassTest.rb", errout);
            break;
        }//case2

        case idMenuTest3:
        {
            // Exit Ruby
            ruby_cleanup(0);
            ruby_finalize();
            delete &ruby;
            break;
        }//case3

        case idMenuTest4:
        {
            // run ERKeyJokey.rb script
            ruby.run_file("ERKbJockeyTest.rb", errout);
            break;
        }//case3

        default: break;
    }//switch

}//MultitouchSuTestFrame
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
