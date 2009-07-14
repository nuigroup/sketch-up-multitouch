#include "wx/utils.h"
#include "wx/app.h"
#include <wx/msgdlg.h>
#include "windows.h"

#include "RubyClass.h"
#include "debugging.h"
#include "MSWrunmacro.h"
#include "HiddenFrame.h"

#include "commctrl.h"
#include "ruby.h"
//#include "intern.h"

 extern HWND         g_hWndSketchUp;            //handle of SketchUp window
 extern HWND         g_hWndSketchUpView;        //graphics view window
 extern HWND         g_hWndSketchUpStatusBar;   //handle of SketchUp StatusBar
 extern HMENU        g_MenuActiveHandle;        //handle of active menu or zero
 extern Debugging*   pVersion;                  //Debugging class
 extern HiddenFrame* pHiddenFrame;              //wxFrame

#define MAPVK_VK_TO_VSC 0
#define MAPVK_VSC_TO_VK 1
#define MAPVK_VK_TO_CHAR 2
#define MAPVK_VSC_TO_VK_EX 3
// ----------------------------------------------------------------------------
// Events relayed to Ruby
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
// Calls handled from Ruby
// ----------------------------------------------------------------------------
//      VALUE initialize(VALUE self)
//      VALUE Logit(VALUE self, VALUE stringToLog)
//      VALUE SendKeyMacro(VALUE self, VALUE rMacroString) See notes at bottom
//      VALUE GetMousePosition(VALUE self)
//              returns array of [x, y]
// ----------------------------------------------------------------------------
//  Globals
// ----------------------------------------------------------------------------
    // Use this typedef to make the compiler happy when
    // calling rb_define_method()
    typedef VALUE (ruby_method)(...);
    typedef VALUE (ruby_protected_func)(VALUE);

// ----------------------------------------------------------------------------
namespace
// ----------------------------------------------------------------------------
{
    // Global Pointer to this class object. Used by static methods to
    // call C++ methods.
    RubyClassHandler* This = 0;
}
// ----------------------------------------------------------------------------
//  Cpp Class Methods
// ----------------------------------------------------------------------------
RubyClassHandler::RubyClassHandler()
// ----------------------------------------------------------------------------
{
    //ctor
    // Define the TuioClient class, add some methods for testing

    This = this;

    m_pMacroRunner = new MSW_ProcessMacro();

    // Define Ruby TuioClient class and methods
    m_rubyClass = rb_define_class("TuioClient", rb_cObject);
    rb_define_method(m_rubyClass, "initialize", (ruby_method*)&Initialize, 0);

    //*Testing* routines
    // we call OnTuioData, user should not call it
    //-rb_define_method(m_rubyClass, "OnTuioData", (ruby_method*)&OnTuioData, 1);

    // called by Ruby script .rb
    rb_define_method(m_rubyClass, "Logit", (ruby_method*)&Logit, 1);
    rb_define_method(m_rubyClass, "GetMousePosition", (ruby_method*)&GetMousePosition, 0);
    rb_define_method(m_rubyClass, "SendKeyMacro", (ruby_method*)&SendKeyMacro, 1);
    //rb_define_method(m_rubyClass, "TimerStart", (ruby_method*)&TimerStart,1);
    //rb_define_method(m_rubyClass, "TimerStop", (ruby_method*)&TimerStop,0);
    rb_define_method(m_rubyClass, "GetVersion", (ruby_method*)&GetVersion,0);
    rb_define_method(m_rubyClass, "RemoveObject", (ruby_method*)&RemoveObject,1);
    rb_define_method(m_rubyClass, "WinFreeze", (ruby_method*)&WinFreeze,1);
    rb_define_method(m_rubyClass, "SetFocusSketchUp", (ruby_method*)&SetFocusSketchUp,0);
}
// ----------------------------------------------------------------------------
RubyClassHandler::~RubyClassHandler()
// ----------------------------------------------------------------------------
{
    //dtor
    if (m_pMacroRunner)
        delete (m_pMacroRunner);
    m_pMacroRunner = 0;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::Initialize(VALUE self)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // Ruby class instantiation
    // This function is called when the user issues "TuioClient.new"

    #if defined(LOGGING)
    LOGIT( "RubyClassHandler::Class Initialize self VALUE[%lu]", self);
    #endif

    // Add a finalizer to users TuioClient class that will delete the class object
    // from our array when Ruby garbage collection deletes this class object.
    VALUE mObjectSpace = rb_const_get(rb_cObject, rb_intern(_T("ObjectSpace")));
    VALUE proc = rb_proc_new((ruby_method*)&Finalizer, self);
    rb_funcall(mObjectSpace, rb_intern("define_finalizer"), 2, self, proc);

    // Add this class object to our array of Ruby objects to post events to.
    This->AddRubyObj(self);

    return self;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::RemoveObject(VALUE self, VALUE id)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // Remove the garbage collected object from our array of class objects

    #if defined(LOGGING)
    LOGIT( _T("RemoveObject self[%lu] removing id[%lu]"), self, id);
    #endif
    This->RemoveRubyObj(id);
    return id;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::Finalizer(VALUE self, VALUE id)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // Remove the garbage collected object from our array of class objects

    #if defined(LOGGING)
    LOGIT( _T("Finalizer self[%lu] removing id[%lu]"), self, id);
    #endif
    This->RemoveRubyObj(id);
    return id;
}
// ----------------------------------------------------------------------------
void RubyClassHandler::FreeObject(void* pObj)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // called from user when an object is about to be garbage collected

    VALUE self = (VALUE)pObj;
    if (self); //stifle "unused" warning
    #if defined(LOGGING)
    char* szObjClassName = rb_obj_classname(self);
    LOGIT( _T("RubyClassHandler::FreeObject[%p]self[%lu]name[%s]"), pObj, self, szObjClassName);
    #endif
    //-This->RemoveRubyObj(self);
}
// ----------------------------------------------------------------------------
void RubyClassHandler::MarkObject(void* pObj)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // Called from Ruby to check if an object should be gargage collected
    VALUE self = (VALUE)pObj;
    #if defined(LOGGING)
    char* szObjClassName = rb_obj_classname(self);
    LOGIT( _T("RubyClassHandler::FreeObject[%p]self[%lu]name[%s]"), pObj, self, szObjClassName);
    #endif
    rb_gc_mark(self);
}
// ----------------------------------------------------------------------------
bool RubyClassHandler::MethodExists( const VALUE vObj, const wxString methodName)
// ----------------------------------------------------------------------------
{
    // Call our Ruby protected mode version of MethodExists so we
    // don't get caught up in Ruby exceptions.


    m_vMethodExistsObj = vObj;
    m_sMethodName = methodName;

    int status = 0;
    // protect ourselves from Ruby exception throwing
    VALUE rc = rb_protect ( &rbpMethodExists, vObj, &status);
    if ( status )
    {   // exception raised, discard the object
         ShowRubyError( status );
         This->RemoveRubyObj(vObj);
    }
    return (rc == Qtrue);
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::rbpMethodExists( VALUE self)
// ----------------------------------------------------------------------------
{//static called by rb_protect for MethodExists

    //-if ( rb_const_defined(rb_cObject, rb_intern("TuioReceiver")) )
    if ( rb_respond_to(This->m_vMethodExistsObj, rb_intern(This->m_sMethodName.c_str()) ))
    {
        return Qtrue;
    }
    return Qfalse;
}
// ----------------------------------------------------------------------------
bool RubyClassHandler::Call_Ruby_OnTuioDataMethod( wxString& tuioDataString )
// ----------------------------------------------------------------------------
{
    // Called from our Tuio client data receiver which was called from
    // the TUIO server

    // Stow accessible parms for static methods
    // and do a rb_protect call to Ruby OnTuioData
    // for each Ruby TuioClient object in our array

    VALUE processedKey = 0;
    int status = false;

    // Set parms in this obj so static funcs can read them
    // with a global 'This' pointer
    m_tuioData = tuioDataString;

    // Call each Ruby TuioClient object having the OnTuioData method instantiated
    int count = GetRubyObjCount();
    for (int i=0; i<count; ++i )
    {
        VALUE tuioClientObj = GetRubyObj(i);

        if ( MethodExists(tuioClientObj, _T("OnTuioData")) )
        {
            #if defined(LOGGING)
            //LOGIT( "RubyClassHandler::CallRubyOnTuioData self[%lu]key[%lu],repeat[%lu],flag[%lu]",
            //                        (VALUE)this, m_key, m_repeat, m_flags);
            #endif

            // protect ourselves from Ruby exception throwing
            VALUE rc = rb_protect ( &rbpOnTuioData, tuioClientObj, &status);
            if ( status ) ShowRubyError( status );
            // Did anyone eat the key?
            if ( (not status) && rc) processedKey = true;
        }
    }
    return (bool)processedKey;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::rbpOnTuioData(VALUE tuioClientObj) //Ruby protected
// ----------------------------------------------------------------------------
{//static called from Ruby

    // rb_protect'ed call to OnTuioData

    VALUE rc = 0;

    VALUE tuioData  = rb_str_new2(This->m_tuioData.c_str());

    rc = rb_funcall(tuioClientObj, rb_intern("OnTuioData"), 1, tuioData);
    return rc;
}
////// ----------------------------------------------------------------------------
////VALUE RubyClassHandler::OnTuioData(VALUE self, wxString tuioDataString)
////// ----------------------------------------------------------------------------
////{//static called from Ruby
////
////    // Ruby user should override this method to receive OnTuioData calls
////    // parms: key, repeat, flag
////    #if defined(LOGGING)
////     LOGIT( "RubyClassHandler::OnTuioData self[%lu]Data[%s]",
////                        self, tuioDataString.c_str());
////    #endif
////    return (VALUE)FALSE;    //let others process the key also
////}

// ----------------------------------------------------------------------------
void RubyClassHandler::ShowRubyError(int error)
// ----------------------------------------------------------------------------
{
    // Fetch the status error for rb_protected calls
    // Other non-protected calls may throw an exception to the
    // console/log and terminate the script. eg, wrong parameter count.

    #if not defined(LOGGING)
    return;
    #endif

    if(error == 0)
        return;

    wxString clog = wxEmptyString;
    wxString endl = _T("\n");

    VALUE lasterr = rb_gv_get("$!");

    // class
    VALUE klass = rb_class_path(CLASS_OF(lasterr));
    clog << "class = " << RSTRING(klass)->ptr << endl;

    // message
    VALUE message = rb_obj_as_string(lasterr);
    clog << "message = " << RSTRING(message)->ptr << endl;

    // backtrace
    if(!NIL_P(ruby_errinfo)) {
        //-std::ostringstream o;
        wxString o = "";
        VALUE ary = rb_funcall(
            ruby_errinfo, rb_intern("backtrace"), 0);
        int c;
        for (c=0; c<RARRAY(ary)->len; c++) {
            o << "\tfrom " <<
            clog << "\tfrom " <<
                RSTRING(RARRAY(ary)->ptr[c])->ptr <<
                "\n";
        }
        //-clog << "backtrace = " << o.str() << endl;
        clog << "backtrace = " << o << endl;
    }
    //-throw runtime_error("ruby_error");
    #if defined(LOGGING)
    LOGIT( _T("Ruby Error:[%s]"), clog.c_str());
    #endif
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::Logit(VALUE self, VALUE stringToLog)
// ----------------------------------------------------------------------------
{//Static called from Ruby

    // Called from Ruby
    // write a Ruby string to our testing log

    #if defined(LOGGING)
     //LOGIT( _T("RbuyClassHandler::logit called from Ruby") );
    #endif
    #if defined(LOGGING)
    char* logStr = rb_string_value_ptr(&stringToLog);
    LOGIT( _T("%s"), logStr);
    #endif
    return true;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::GetVersion(VALUE self)
// ----------------------------------------------------------------------------
{//Static called from Ruby

    // Called from Ruby
    // Return a string containing version

    #if defined(LOGGING)
     //LOGIT( _T("RbuyClassHandler::GetVersion called from Ruby") );
    #endif

    wxString version = wxEmptyString;
    if (pVersion)
        version = pVersion->GetVersion();
    VALUE ruby_string = rb_str_new2(version.c_str());
    return ruby_string;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::GetMousePosition(VALUE self)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // return the mouse position to the caller as a Ruby array.
    // eg.,  x, y = GetMousePosition()

    // Called from Ruby
    // return current mouse position x,y

    #if defined(LOGGING)
     //LOGIT( _T("RbuyClassHandler::GetMousePosition called from Ruby") );
    #endif

    VALUE ary = rb_ary_new2(2);

    int x , y;
    ::wxGetMousePosition(&x, &y);
    //FIXME: shouldn't we return client coordinates, not screen coordinates
    rb_ary_store(ary,0,INT2NUM(x));
    rb_ary_store(ary,1,INT2NUM(y));
    return ary;
}
// ----------------------------------------------------------------------------
VALUE  RubyClassHandler::SendKeyMacro(VALUE self, VALUE rMacroString)
// ----------------------------------------------------------------------------
{//static called from Ruby


    // send a Ruby macro string to our macro processor
    // eg., "SendKeyMacro("{LSHIFT DOWN}{MBUTTON DOWN}"

    This->m_macroProcessIsBusy = true;

    VALUE str = StringValue(rMacroString);
    char* p = RSTRING(str)->ptr;
    wxString macroKeys(p);
    #if defined(LOGGING)
    //LOGIT( _T("RubyClassHandler:SendKeyMacro[%s]"), macroKeys.c_str());
    #endif

    // Make sure keys go to main window (not Ruby console or other foreground)
    HWND hRubyConsole = ::FindWindow(_T("#32770"), _T("Ruby Console"));
    if (hRubyConsole) {;}
    ::EnableWindow(g_hWndSketchUp,TRUE);
    ::SetFocus(g_hWndSketchUp);
    ::SetForegroundWindow(g_hWndSketchUp);

    This->m_pMacroRunner->SendMacroString( macroKeys, wxTheApp->GetTopWindow() );
    ::SetFocus(g_hWndSketchUp);
    This->m_macroProcessIsBusy = false;

    return true;
}
//// ----------------------------------------------------------------------------
//VALUE  RubyClassHandler::TimerStart(VALUE selfIn, VALUE dblSecondsIn)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    VALUE rc = 0;
//    // Convert double seconds to milliseconds
//    // Insert time into timer queue, stop timer, update time queue
//    // queue any resulting timer events, re-state timer
//    // Execute timer event queue.
//    double secs = NUM2DBL( dblSecondsIn );
//    LONG millisecs = UINT( secs * 1000 );
//    This->GetHiddenFrame()->TimerQueAdd( (ULONG)selfIn, millisecs );
//    return rc;
//}
//// ----------------------------------------------------------------------------
//VALUE  RubyClassHandler::TimerStop(VALUE selfIn)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Find and stop the timer for this object
//    VALUE rc = 0;
//    rc = This->GetHiddenFrame()->TimerQueRemove( (ULONG)selfIn );
//    return rc?Qtrue:Qfalse;
//}
//// ----------------------------------------------------------------------------
//void RubyClassHandler::Call_Ruby_OnTimerEvent(UINT objectId)
//// ----------------------------------------------------------------------------
//{
//    // Call the Ruby TuioClient object OnTimerEvent matching objectId
//    int count = GetRubyObjCount();
//    for (int i=0; i<count; ++i )
//    {
//        VALUE tuioClientObj = GetRubyObj(i);
//
//        if ( tuioClientObj not_eq objectId)
//            continue;
//
//        if ( MethodExists(tuioClientObj, _T("OnTimerEvent")) )
//        {
//            #if defined(LOGGING)
//            //LOGIT( "RubyClassHandler::CallRuby_OnTimerEvent self[%lu]", (VALUE)this );
//            #endif
//
//            // protect ourselves from Ruby exception throwing
//            int status = false;
//            VALUE rc = rb_protect ( &rbpOnTimerEvent, tuioClientObj, &status);
//            if ( rc ) {;} //unused
//            if ( status ) ShowRubyError( status );
//        }
//    }
//    return ;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::rbpOnTimerEvent(VALUE tuioClientObj) //Ruby protected
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // rb_protect'ed call to OnTimerEvent
//
//    VALUE rc = 0;
//
//    rc = rb_funcall(tuioClientObj, rb_intern("OnTimerEvent"), 0 );
//    return rc;
//}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::WinFreeze(VALUE self, VALUE stateIn)
// ----------------------------------------------------------------------------
{//static called from Ruby

    bool state = NUM2INT(stateIn);
    #if defined(LOGGING)
    LOGIT( _T("ERWinFreeze self[%lu] state[%s]"), self, state?_T("TRUE"):_T("FALSE"));
    #endif

    if (state)
        ::SendMessage(g_hWndSketchUp, WM_SETREDRAW, (WPARAM)false, 0);
    else
        ::SendMessage(g_hWndSketchUp, WM_SETREDRAW, (WPARAM)true, 0);

    return stateIn;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::SetFocusSketchUp(VALUE self)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // Show Ruby Console, but put focus back to SketchUp window

    #if defined(LOGGING)
    LOGIT( _T("ERSetFocusSketchUp self[%lu]"), self);
    #endif

    HWND oldhWnd = ::GetFocus();
    // This wont work, the WM_COMMAND is in the queue. Window is not up yet.
    //unsigned menuId = pHiddenFrame->GetMenuIdFromPath(_T("Window/Ruby Console"));
    //LRESULT lresult = ::SendMessage( g_hWndSketchUp, WM_COMMAND, menuId, 0);
    //HWND rubyConsole = ::FindWindow("#32770", "Ruby Console");

    HWND fghWnd = ::GetForegroundWindow();

    if ( oldhWnd == g_hWndSketchUpView)
        return Qtrue;

    if ( fghWnd == g_hWndSketchUp)
    {
        return Qtrue;
    }

    ::EnableWindow( g_hWndSketchUp, TRUE );
    ::EnableWindow( g_hWndSketchUpView, TRUE );
    ::SetForegroundWindow(g_hWndSketchUp);
    ::SetFocus(g_hWndSketchUpView);

    HWND newhWnd = ::GetFocus();
    HWND newfgWnd = ::GetForegroundWindow();
    //if ( lresult || oldhWnd || newhWnd || rubyConsole ) {;} //shush compiler unused warning

    #if defined(LOGGING)
    LOGIT( _T("ERSetFocusSketchUp OldFocus[%p] NewFocus[%p]] "),
        (void*)oldhWnd, (void*)newhWnd);
    LOGIT( _T("ERSetFocusSketchUp ForeGrnd[%p] SketchUp[%p]]"),
        (void*)fghWnd, (void*)g_hWndSketchUp);
    #endif

    #if defined(LOGGING)
    LOGIT( _T("ERSetFocusSketchUp self[%lu] Focus[%s]"), self,
        ( newhWnd == g_hWndSketchUp )?_T("TRUE":_T("FALSE")));
    #endif

    if ( newhWnd == g_hWndSketchUpView )
        return Qtrue;
    if ( newfgWnd == g_hWndSketchUp )
        return Qtrue;

    return Qfalse;
}
// ----------------------------------------------------------------------------
//  Example script
// ----------------------------------------------------------------------------
//    puts "ClassTest.rb running from " + Dir.getwd
//    require "MultitouchSu.dll"
//    # ----------------------------------------------------------------------------
//    class MyTuioClient < TuioClient
//    # ----------------------------------------------------------------------------
//
//        # ----------------------------------
//        # initialize
//        # ----------------------------------
//        def initialize
//            super();         # <== initialize the Ruby TuioClient extension
//        end
//        # ----------------------------------
//        #     OnTuioData (called by TuioClient when TUIO data is available)
//        # ----------------------------------
//        def OnTuioData( stringOfTuioData )
//            Logit "FROM RUBY OnTuioData: #{stringOfTuioData}"
//            puts stringOfTuioData; #write to SketchUp console
//            return true
//        end
//    end # class MyTuioClient
//
//    # ------------------------------------------------------------------------
//    # The TuioClient extension will see the next statement and make
//    # a global reference to "myTuioClient" object class so it lives past
//    # the termination of this script. Anything outside MyTuioClient class will
//    # be eaten by the Ruby garbage collector.
//    # ------------------------------------------------------------------------
//
//    myTuioClient = MyTuioClient.new();
//
//    #end of script
