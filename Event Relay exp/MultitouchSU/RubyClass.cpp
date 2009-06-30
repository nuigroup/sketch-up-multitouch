/***************************************************************
Credits:Event Relay and MultitouchSU
License devs ToBeDone
*/

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
//      OnKeyDown(key, repeatcount, modifierFlags)
//      OnKeyUp  (key, repeatcount, modifierFlags)
//
//      OnLButtonDown( modifierFlags, x, y)
//      OnMButtonDown( modifierFlags, x, y)
//      OnRButtonDown( modifierFlags, x, y)
//
//      OnLButtonUp( modifierFlags, x, y)
//      OnMButtonUp( modifierFlags, x, y)
//      OnRButtonUp( modifierFlags, x, y)

//      OnMenuSelected( menuId, menuLabel)
//
//      A return of true suppresses the key or menu
//      A return of false allows the key or menu
//
// ----------------------------------------------------------------------------
// Calls handled from Ruby
// ----------------------------------------------------------------------------
//      VALUE initialize(VALUE self)
//      VALUE Logit(VALUE self, VALUE stringToLog)
//      VALUE SendKeyMacro(VALUE self, VALUE rMacroString) See notes at bottom
//      VALUE GetMousePosition(VALUE self)
//              returns array of [x, y]
//      VALUE GetKeyInfo(VALUE virtualKeyNum)
//              returns array of ["vk_name", "{key_macro_symbol}"]
// ----------------------------------------------------------------------------
//  Globals
// ----------------------------------------------------------------------------
    // Use this typedef to make the compiler happy when
    // calling rb_define_method()
    typedef VALUE (ruby_method)(...);
    typedef VALUE (ruby_protected_func)(VALUE);

    // Global pointer to this class used in static routines
// ----------------------------------------------------------------------------
namespace
// ----------------------------------------------------------------------------
{
    // Pointer to this class object. Used by static methods to
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
    // Define the EventRelay class, add some methods for testing

    This = this;

    m_pMacroRunner = new MSW_ProcessMacro();

    // Define Ruby EventRelay class and methods
    m_rubyClass = rb_define_class("EventRelay", rb_cObject);
    rb_define_method(m_rubyClass, "initialize", (ruby_method*)&Initialize, 0);

    //*Testing* routines
    // we call OnKeyDown, user should not call it
    //rb_define_method(m_rubyClass, "OnKeyDown", (ruby_method*)&OnKeyDown, 3);
    // we call OnKeyUp, user should not call it
    //rb_define_method(m_rubyClass, "OnKeyUp", (ruby_method*)&OnKeyUp, 3);

    //*Testing* routines
    // The OnxButtonDown/Up methods don't have to be defined here.
    // The user defines them in their EventRelay class. These are here for
    // convenience testing only.
    // we call OnRButtonDown, user must not call it (these here for testing only)
    //rb_define_method(m_rubyClass, "OnRButtonDown", (ruby_method*)&OnRButtonDown, 3);
    // we call OnRButtonUp, user must not call it (here for testing only)
    //rb_define_method(m_rubyClass, "OnRButtonUp", (ruby_method*)&OnRButtonUp, 3);

    // called by script .rb
    rb_define_method(m_rubyClass, "Logit", (ruby_method*)&Logit, 1);
    rb_define_method(m_rubyClass, "GetMousePosition", (ruby_method*)&GetMousePosition, 0);
    rb_define_method(m_rubyClass, "SendKeyMacro", (ruby_method*)&SendKeyMacro, 1);
    rb_define_method(m_rubyClass, "GetKeyInfo", (ruby_method*)&GetKeyInfo,2);
    //rb_define_method(m_rubyClass, "ERTimerStart", (ruby_method*)&ERTimerStart,1);
    //rb_define_method(m_rubyClass, "ERTimerStop", (ruby_method*)&ERTimerStop,0);
    rb_define_method(m_rubyClass, "ERGetVersion", (ruby_method*)&ERGetVersion,0);
    rb_define_method(m_rubyClass, "ERRemoveObject", (ruby_method*)&ERRemoveObject,1);
    //rb_define_method(m_rubyClass, "ERGetMenuID", (ruby_method*)&ERGetMenuID,1);
    rb_define_method(m_rubyClass, "ERWinFreeze", (ruby_method*)&ERWinFreeze,1);
    rb_define_method(m_rubyClass, "ERSetFocusSketchUp", (ruby_method*)&ERSetFocusSketchUp,0);
    //rb_define_method(m_rubyClass, "ERSendMenuAction", (ruby_method*)&ERSendMenuAction,1);
    //rb_define_method(m_rubyClass, "ERCmdKeyConfig", (ruby_method*)&ERCmdKeyConfig,0);
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
    // This function is called when the user issues "EventRelay.new"

    #if defined(LOGGING)
    LOGIT( "RubyClassHandler::Class Initialize self VALUE[%lu]", self);
    #endif

    // Add a finalizer to users EventRelay class that will delete the class object
    // from our array when Ruby garbage collection deletes this class object.
    VALUE mObjectSpace = rb_const_get(rb_cObject, rb_intern(_T("ObjectSpace")));
    VALUE proc = rb_proc_new((ruby_method*)&Finalizer, self);
    rb_funcall(mObjectSpace, rb_intern("define_finalizer"), 2, self, proc);

    // Add this class object to our array of Ruby objects to post events to.
    This->AddRubyObj(self);

    return self;
}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::ERRemoveObject(VALUE self, VALUE id)
// ----------------------------------------------------------------------------
{//static called from Ruby

    // Remove the garbage collected object from our array of class objects

    #if defined(LOGGING)
    LOGIT( _T("ERRemoveObject self[%lu] removing id[%lu]"), self, id);
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

    // I tried to use this routine to free objects garbage collected
    // by Ruby. But it was being entered even when the user assigned
    // the EventRelay object to a global. So I abandoned the idea.

    // This is being entered as if our object is being freed,
    // Though it never gets freed. Even if we registered a global variable
    // in Initialize() that contains a reference to the EventRelay object
    // this routine gets called.
    // Why this is entered, only Ruby knows.

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

    // I tried to mark the object to get garbage collection to leave it
    // alone, but it didn't work.

    //This routine is never called because we no longer register it
    // in parameter 2 of:
    // "VALUE m_holder = Data_Wrap_Struct(rb_cObject, 0, &FreeObject, (VALUE*)self);"
    // call. Even if we did, it never gets called anyway. Ruby only knows why.

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

    //-if ( rb_const_defined(rb_cObject, rb_intern("EventRelay")) )
    if ( rb_respond_to(This->m_vMethodExistsObj, rb_intern(This->m_sMethodName.c_str()) ))
    {
        return Qtrue;
    }
    return Qfalse;
}
//// ----------------------------------------------------------------------------
//bool RubyClassHandler::Call_Ruby_OnKeyDownMethod( UINT key, UINT repeat, UINT flag)
//// ----------------------------------------------------------------------------
//{
//    // Called from our HiddenWindow event handler which was called from
//    // the Dll Keyboard hook
//
//    // Stow accessible parms for static methods
//    // and do a rb_protect call to Ruby OnKeyDown
//    // for each Ruby EventRelay object in our array
//
//    VALUE processedKey = 0;
//    int status = false;
//
//    // Set parms in this obj so static funcs can read them
//    // with a global 'This' pointer
//    m_key = key;
//    m_repeat = repeat;
//    m_flags = flag;
//
//    // Call each Ruby EventRelay object having the OnKeyDown method instantiated
//    int count = GetRubyObjCount();
//    for (int i=0; i<count; ++i )
//    {
//        VALUE eventRelayObj = GetRubyObj(i);
//
//        //-if (not rb_const_defined(rb_cObject, rb_intern("EventRelay")) )
//        //-{
//        //-    RemoveRubyObj( eventRelayObj );
//        //-    return false;
//        //-}
//
//        //-if ( rb_respond_to(eventRelayObj,rb_intern("OnKeyDown") )) //crash on gc if not global
//        if ( MethodExists(eventRelayObj, _T("OnKeyDown")) )
//        {
//            #if defined(LOGGING)
//            //LOGIT( "RubyClassHandler::CallRubyOnKeyDown self[%lu]key[%lu],repeat[%lu],flag[%lu]",
//            //                        (VALUE)this, m_key, m_repeat, m_flags);
//            #endif
//
//            // protect ourselves from Ruby exception throwing
//            VALUE rc = rb_protect ( &rbpOnKeyDown, eventRelayObj, &status);
//            if ( status ) ShowRubyError( status );
//            // Did anyone eat the key?
//            if ( (not status) && rc) processedKey = true;
//        }
//    }
//    return (bool)processedKey;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::rbpOnKeyDown(VALUE eventRelayObj) //Ruby protected
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // rb_protect'ed call to OnKeyDown
//
//    VALUE rc = 0;
//
//    VALUE key    = INT2FIX(This->m_key);
//    VALUE repeat = INT2FIX(This->m_repeat);
//    VALUE flags  = INT2FIX(This->m_flags);
//
//    rc = rb_funcall(eventRelayObj, rb_intern("OnKeyDown"), 3,
//                                key, repeat, flags);
//    return rc;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::OnKeyDown(VALUE self, VALUE key, VALUE repeat, VALUE flag)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Ruby user should override this method to receive OnKeyDown calls
//    // parms: key, repeat, flag
//    #if defined(LOGGING)
//     LOGIT( "RubyClassHandler::OnKeyDown self[%lu]key[%lu],repeat[%lu],flag[%lu]",
//                        self, key, repeat, flag);
//    #endif
//    return (VALUE)FALSE;    //let others process the key also
//}
//// ----------------------------------------------------------------------------
//bool RubyClassHandler::Call_Ruby_OnKeyUpMethod( UINT key, UINT repeat, UINT flag)
//// ----------------------------------------------------------------------------
//{
//    // Called from our HiddenWindow event handler which was called
//    // from the Dll Keyboard hook
//
//    // Stow accessible parms for static methods
//    // and do a rb_protect call to Ruby OnKeyUp
//    // for each Ruby EventRelay object in our array
//
//    VALUE processedKey = 0;
//    int status = false;
//
//    // Set parms in this obj so static funcs can read them
//    // with a global 'this' pointer
//    m_key = key;
//    m_repeat = repeat;
//    m_flags = flag;
//
//    // Call each Ruby EventRelay object having the OnKeyUp method instantiated
//    int count = GetRubyObjCount();
//    for (int i=0; i<count; ++i )
//    {
//        VALUE eventRelayObj = GetRubyObj(i);
//        if ( MethodExists(eventRelayObj, _T("OnKeyUp")) )
//        {
//            #if defined(LOGGING)
//            //LOGIT( "RubyClassHandler::CallRubyOnKeyUp self[%lu]key[%lu],repeat[%lu],flag[%lu]",
//            //                        (VALUE)this, m_key, m_repeat, m_flags);
//            #endif
//
//            // protect ourselves from Ruby exception throwing
//            VALUE rc = rb_protect ( &rbpOnKeyUp, eventRelayObj, &status);
//            if ( status ) ShowRubyError( status );
//            // Did anyone eat the key?
//            if ( (not status) && rc) processedKey = true;
//        }
//    }
//    return (bool)processedKey;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::rbpOnKeyUp(VALUE eventRelayObj) //Ruby protected
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // rb_protect'ed call to OnKeyUp
//
//    VALUE rc = 0;
//    VALUE key    = INT2FIX(This->m_key);
//    VALUE repeat = INT2FIX(This->m_repeat);
//    VALUE flags  = INT2FIX(This->m_flags);
//
//    rc = rb_funcall(eventRelayObj, rb_intern("OnKeyUp"), 3,
//                            key, repeat, flags);
//    return rc;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::OnKeyUp(VALUE self, VALUE key, VALUE repeat, VALUE flag)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Test function
//    // Ruby user should override this method to receive OnKeyUp calls
//    // parms: key, repeat, flag
//    #if defined(LOGGING)
//    LOGIT( "RubyClassHandler::OnKeyUp self[%lu]key[%lu],repeat[%lu],flag[%lu]",
//                        self, key, repeat, flag);
//    #endif
//    return (VALUE)FALSE;    //let others process the key also
//}
//// ----------------------------------------------------------------------------
//bool RubyClassHandler::Call_Ruby_OnMouseEventMethod( UINT mouseEventType, UINT flags, int x, int y)
//// ----------------------------------------------------------------------------
//{
//    // Called from the HiddenWindow event which was called from
//    // the Dll mouse hook
//
//    // Stow parms for so they're accessible from static methods
//    // and do a rb_protect call to Ruby OnRButtonDown
//    // for each Ruby EventRelay object in our array
//
//    VALUE processedKey = 0;
//    int status = false;
//    UINT m_MouseEventType = mouseEventType;
//
//    switch (m_MouseEventType)
//    {
//        case m_typeRMouseDown:
//            m_charMouseEventType = _T("OnRButtonDown");
//            break;
//        case m_typeLMouseDown:
//            m_charMouseEventType = _T("OnLButtonDown");
//            break;
//        case m_typeMMouseDown:
//            m_charMouseEventType = _T("OnMButtonDown");
//            break;
//        case m_typeRMouseUp:
//            m_charMouseEventType = _T("OnRButtonUp");
//            break;
//        case m_typeLMouseUp:
//            m_charMouseEventType = _T("OnLButtonUp");
//            break;
//        case m_typeMMouseUp:
//            m_charMouseEventType = _T("OnMButtonUp");
//            break;
//        default: return false;
//    }
//    // Set parms in this obj so static funcs can read them
//    // with a global 'this' pointer
//    m_x = x;
//    m_y = y;
//    m_flags = flags;
//
//    // Call each Ruby EventRelay object having the On{R|L|M}Button{Down|Up} method instantiated
//    int count = GetRubyObjCount();
//    for (int i=0; i<count; ++i )
//    {
//        VALUE eventRelayObj = GetRubyObj(i);
//        //-if ( rb_respond_to(eventRelayObj,rb_intern(m_charMouseEventType) ))
//        if ( MethodExists(eventRelayObj, m_charMouseEventType) )
//        {
//            #if defined(LOGGING)
//            LOGIT( "RubyClassHandler::Call_Ruby_OnMouseEventMethod type[%s] x[%ld], y[%ld],flags[%lu]",
//                                    m_charMouseEventType.c_str(), m_x, m_y, m_flags);
//            #endif
//
//            // protect ourselves from Ruby exception throwing
//            VALUE rc = rb_protect ( &rbpOnMouseMethod, eventRelayObj, &status);
//            if ( status ) ShowRubyError( status );
//            // Did anyone eat the key?
//            if ( (not status) && rc) processedKey = true;
//        }
//    }
//    return (bool)processedKey;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::rbpOnMouseMethod(VALUE eventRelayObj) //Ruby protected
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // rb_protect'ed call to OnMouse Right/Left/Middle Up/Down
//
//    VALUE rc = 0;
//    wxString mouseMethod = This->m_charMouseEventType;
//    VALUE flags = INT2FIX(This->m_flags);
//    VALUE x     = INT2FIX(This->m_x);
//    VALUE y     = INT2FIX(This->m_y);
//
//    rc = rb_funcall(eventRelayObj, rb_intern(mouseMethod.c_str()), 3, flags, x, y);
//    return rc;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::OnRButtonDown(VALUE self, VALUE flags, VALUE x, VALUE y )
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Test function
//    // Ruby user should override this method to receive OnRButtonDown calls
//    // parms: flags, x, y
//    #if defined(LOGGING)
//    LOGIT( "RubyClassHandler::OnRButtonDown self[%lu] flags[%lu],x[%lu], y[%lu]",
//                        self, flags, x, y);
//    #endif
//    return (VALUE)FALSE;    //let others process the key also
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::OnRButtonUp(VALUE self, VALUE flags, VALUE x, VALUE y )
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Test function
//    // Ruby user should override this method to receive OnRButtonUp calls
//    // parms: flags, x, y
//    #if defined(LOGGING)
//    LOGIT( "RubyClassHandler::OnRButtonUp self[%lu] flags[%lu],x[%lu], y[%lu]",
//                        self, flags, x, y);
//    #endif
//    return (VALUE)FALSE;    //let others process the key also
//}
//// ----------------------------------------------------------------------------
//bool RubyClassHandler::Call_Ruby_OnMenuSelected( const UINT menuId, const wxString menuLabel)
//// ----------------------------------------------------------------------------
//{
//    // Called from our HiddenWindow event handler which was called from
//    // the Dll Keyboard hook
//
//    // Stow accessible parms for static methods
//    // and do a rb_protect call to Ruby OnMenuSelected
//    // for each Ruby EventRelay object in our array
//
//    VALUE processedKey = 0;
//    int status = false;
//
//    // Set parms in this obj so static funcs can read them
//    // with a global 'This' pointer
//    m_MenuId = menuId;
//    m_MenuLabel = menuLabel;
//
//    // Call each Ruby EventRelay object having the OnMenuSelected method instantiated
//    int count = GetRubyObjCount();
//    for (int i=0; i<count; ++i )
//    {
//        VALUE eventRelayObj = GetRubyObj(i);
//        //-if ( rb_const_defined(rb_cObject, rb_intern("EventRelay")) )
//
//        //-if ( rb_respond_to(eventRelayObj,rb_intern("OnMenuSelected") )) //crash on gc if not global
//        if ( MethodExists(eventRelayObj, _T("OnMenuSelected")) )
//        {
//            #if defined(LOGGING)
//            //LOGIT( "RubyClassHandler::CallRuby_OnMenuSelected self[%lu]menuId[%d],MenuLabel[%s]",
//            //                        (VALUE)this, m_MenuId, m_MenuLabel.c_str());
//            #endif
//
//            // protect ourselves from Ruby exception throwing
//            VALUE rc = rb_protect ( &rbpOnMenuSelected, eventRelayObj, &status);
//            if ( status ) ShowRubyError( status );
//
//            // Did anyone eat the menu?
//            if ( (not status) && rc) processedKey = true;
//        }
//    }
//    return (bool)processedKey;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::rbpOnMenuSelected(VALUE eventRelayObj) //Ruby protected
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // rb_protect'ed call to OnMenuSelected
//
//    VALUE rc = 0;
//
//    VALUE menuId    = UINT2NUM(This->m_MenuId);
//    VALUE menuLabel = rb_str_new2(This->m_MenuLabel.c_str());
//
//    rc = rb_funcall(eventRelayObj, rb_intern("OnMenuSelected"),2,
//                                menuId, menuLabel);
//    return rc;
//}

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
VALUE RubyClassHandler::ERGetVersion(VALUE self)
// ----------------------------------------------------------------------------
{//Static called from Ruby

    // Called from Ruby
    // Return a string containing version

    #if defined(LOGGING)
     //LOGIT( _T("RbuyClassHandler::ERGetVersion called from Ruby") );
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
// ----------------------------------------------------------------------------
VALUE  RubyClassHandler::GetKeyInfo(VALUE selfIn, VALUE vk_keyIn, VALUE vk_flagsIn)
// ----------------------------------------------------------------------------
{//static called from Ruby

        // return array of ["virtual key name", "{macor key symbol}"]

    unsigned const rubyVkKey = NUM2UINT(vk_keyIn);
    unsigned const rubyVkFlags = NUM2UINT(vk_flagsIn);
    if (rubyVkFlags); //unused

    #if defined(LOGGING)
     //LOGIT( _T("RubyClassHandler::GetKeyInfo called from Ruby[%u]"), rubyVkKey );
    #endif

    //Doc:
    // int GetKeyNameText( LONG lParam, LPTSTR lpString, int nSize );
    // lParam:
    //    16—23
    //        Scan code.
    //    24
    //        Extended-key flag. Distinguishes some keys on an enhanced keyboard.
    //    25
    //        "Don't care" bit. The application calling this function sets this bit
    //        to indicate that the function should not distinguish between left and
    //        right CTRL and SHIFT keys, for example.

    long lParam = 0;
    UINT scanCode = This->GetScanCodeFromVirtualKey( rubyVkKey );
    lParam = scanCode << 16;
    long bit24 = 1 << 24;
    lParam |= bit24; //try for extended key
    char mnemonic[128] = {0};
    int len = ::GetKeyNameText( lParam, mnemonic, sizeof(mnemonic) );
    wxString vkMnemonic( mnemonic );
    // No extended key, try for non-extended key
    if ( 0 == len )
    {
        lParam = scanCode<<16;
        len = ::GetKeyNameText( lParam, mnemonic, sizeof(mnemonic) );
        vkMnemonic = mnemonic;
    }
    if ( 0 == len ) vkMnemonic = _T("UNKNOWN");
        else vkMnemonic.MakeUpper();

    //Initialize data with dummy values
    This->m_VkString = _T("UNKNOWN");

    // Transate the virtual key to a string
    if (len)
        This->m_VkString = wxString::Format( _T("VK_%s"), vkMnemonic.c_str());
    else
        if ( (rubyVkKey > 31) && (rubyVkKey < 97) ) //between space && backtick
            This->m_VkString = wxString::Format(  _T("VK_%c"), rubyVkKey );

    // Translate equivalent KeySym macro to a string
    This->m_KeyMacroSymbol = This->m_pMacroRunner->GetKeySymFromVKey(rubyVkKey);

    // If KeySym macro, surround with braces
    if (not This->m_KeyMacroSymbol.IsEmpty())
        This->m_KeyMacroSymbol = _T("{") + This->m_KeyMacroSymbol +  _T("}");

    // if simply a letter or number, say so
    if (This->m_KeyMacroSymbol.IsEmpty())
        if ( (rubyVkKey > 31) && (rubyVkKey < 97) ) //between space && backtick
            This->m_KeyMacroSymbol = wxString::Format(  _T("%c"), rubyVkKey );
        else //unknown symbol or key
            This->m_KeyMacroSymbol = _T("{UNKNOWN}");

    // Translate to Ruby type VALUE
    VALUE vVk_string = rb_str_new2(This->m_VkString.c_str());
    VALUE vKeyMacroSymbol = rb_str_new2(This->m_KeyMacroSymbol.c_str());

    // Create Ruby array and insert strings
    VALUE vAry = rb_ary_new2(2);
    rb_ary_store(vAry,0,vVk_string);
    rb_ary_store(vAry,1,vKeyMacroSymbol);

    return vAry;
}
//// ----------------------------------------------------------------------------
//VALUE  RubyClassHandler::ERTimerStart(VALUE selfIn, VALUE dblSecondsIn)
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
//VALUE  RubyClassHandler::ERTimerStop(VALUE selfIn)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Find and stop the timer for this object
//    VALUE rc = 0;
//    rc = This->GetHiddenFrame()->TimerQueRemove( (ULONG)selfIn );
//    return rc?Qtrue:Qfalse;
//}
//// ----------------------------------------------------------------------------
//void RubyClassHandler::Call_Ruby_OnERTimerEvent(UINT objectId)
//// ----------------------------------------------------------------------------
//{
//    // Call the Ruby EventRelay object OnERTimerEvent matching objectId
//    int count = GetRubyObjCount();
//    for (int i=0; i<count; ++i )
//    {
//        VALUE eventRelayObj = GetRubyObj(i);
//
//        if ( eventRelayObj not_eq objectId)
//            continue;
//
//        if ( MethodExists(eventRelayObj, _T("OnERTimerEvent")) )
//        {
//            #if defined(LOGGING)
//            //LOGIT( "RubyClassHandler::CallRuby_OnERTimerEvent self[%lu]", (VALUE)this );
//            #endif
//
//            // protect ourselves from Ruby exception throwing
//            int status = false;
//            VALUE rc = rb_protect ( &rbpOnERTimerEvent, eventRelayObj, &status);
//            if ( rc ) {;} //unused
//            if ( status ) ShowRubyError( status );
//        }
//    }
//    return ;
//}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::rbpOnERTimerEvent(VALUE eventRelayObj) //Ruby protected
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // rb_protect'ed call to OnERTimerEvent
//
//    VALUE rc = 0;
//
//    rc = rb_funcall(eventRelayObj, rb_intern("OnERTimerEvent"), 0 );
//    return rc;
//}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::ERWinFreeze(VALUE self, VALUE stateIn)
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
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::ERGetMenuID(VALUE self, VALUE menuPathIn)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    VALUE str = StringValue(menuPathIn);
//    char* p = RSTRING(str)->ptr;
//    wxString menuPath(p);
//
//    #if defined(LOGGING)
//    LOGIT( _T("ERGetMenuID self[%lu] path[%s]"), self, menuPath.c_str());
//    #endif
//
//    unsigned id = pHiddenFrame->GetMenuIdFromPath(menuPath);
//    return INT2NUM(id);
//}
// ----------------------------------------------------------------------------
VALUE RubyClassHandler::ERSetFocusSketchUp(VALUE self)
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
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::ERSendMenuAction(VALUE self, VALUE menuPathIn)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Flash SketchUp Edit menu to build correct menu, and queue action
//    // for later execution.
//
//    VALUE str = StringValue(menuPathIn);
//    char* p = RSTRING(str)->ptr;
//    wxString menuPath(p);
//    #if defined(LOGGING)
//    LOGIT( _T("ERSendMenuAction self[%lu] menu[%s]"), self, menuPath.c_str());
//    #endif
//
//    //SendKeyMacro(_T("!e{ALT}"));
//    pHiddenFrame->SendMenuActionAdd(menuPath);
//    return Qtrue;
//}
////// ----------------------------------------------------------------------------
////VALUE RubyClassHandler::ERSendAction(VALUE self, VALUE idIn)
////// ----------------------------------------------------------------------------
////{//static called from Ruby
////
////    // Flash SketchUp Edit menu to build correct menu, and queue action
////    // for later execution.
////
////    unsigned menuID = NUM2INT( idIn );
////    #if defined(LOGGING)
////    LOGIT( _T("ERSendAction self[%lu] menuID[%d]"), self, menuID);
////    #endif
////
////    //SendKeyMacro(_T("!e{ALT}"));
////    This->m_pMacroRunner->SendMacroString( _T("!e{ALT}"), wxTheApp->GetTopWindow() );
////    pHiddenFrame->SendActionAdd(menuID);
////    return idIn;
////}
//// ----------------------------------------------------------------------------
//VALUE RubyClassHandler::ERCmdKeyConfig(VALUE self)
//// ----------------------------------------------------------------------------
//{//static called from Ruby
//
//    // Show KeyBinder Command define dialog
//
//    #if defined(LOGGING)
//    LOGIT( _T("ERCmdKeyConfig self[%lu]"), self);
//    #endif
//
//    int count = pHiddenFrame->KeyBinderConfig();
//    return INT2FIX(count);
//}
// ----------------------------------------------------------------------------
//  Utility functions
// ----------------------------------------------------------------------------
UINT RubyClassHandler::GetCharFromVirtualKey( UINT vKeyIn)
// ----------------------------------------------------------------------------
{
    return ::MapVirtualKey( vKeyIn, MAPVK_VK_TO_CHAR);
}
// ----------------------------------------------------------------------------
UINT RubyClassHandler::GetScanCodeFromVirtualKey( UINT vKeyIn)
// ----------------------------------------------------------------------------
{
    return ::MapVirtualKey( vKeyIn, MAPVK_VK_TO_VSC);
}
// ----------------------------------------------------------------------------
//  Example script
// ----------------------------------------------------------------------------
//    puts "ClassTest.rb running from " + Dir.getwd
//    require "EventRelay.dll"
//    # ----------------------------------------------------------------------------
//    class MyEventRelay < EventRelay
//    # ----------------------------------------------------------------------------
//        VK_F1 = 112 # F1 key
//
//        # ----------------------------------
//        # initialize
//        # ----------------------------------
//        def initialize
//            @handDolly = false
//            super         # <== initialize the EventRelay extension
//        end
//        # ----------------------------------
//        #	OnKeyDown
//        # ----------------------------------
//        def OnKeyDown(key,repeat,flag)
//            # puts "rb file overrode OnKeyDown"
//            Logit "FROM RUBY OnKeyDown key:#{key} repeat:#{repeat} flags:#{flag}"
//
//            # Kill the F1 key
//            if (key == VK_F1) #Kill Help key
//                return true
//            end
//            return false
//        end
//        # ----------------------------------
//        # OnKeyUp
//        # ----------------------------------
//        def OnKeyUp(key,repeat,flag)
//            # puts "rb file overrode OnKeyDown"
//            #Logit "FROM RUBY OnKeyUp key:#{key} repeat:#{repeat} flags:#{flag}"
//
//            # F1 killed by OnKeyDown. The following is unnecessary for F1
//            #if (key == VK_F1) #Kill Help key
//            #	return true
//            #end
//            return false
//        end
//        # ----------------------------------
//        #	OnLButtonDown
//        # ----------------------------------
//    #    def OnLButtonDown(flags, x, y)
//    #        return false
//    #    end
//        # ----------------------------------
//        # OnLButtonUp
//        # ----------------------------------
//    #    def OnLButtonUp(flags, x, y)
//    #        return false
//    #    end
//        # ----------------------------------
//        # OnRButtonUp
//        # ----------------------------------
//        def OnRButtonUp(flags, x, y)
//            Logit "FROMRUBY: OnRButtonUp flags:#{flags} x:#{x} y:#{y}"
//
//            # If we turned on panning, release it
//            if ( @handDolly )
//                @handDolly = false
//                SendKeyMacro("{MBUTTON UP}{LSHIFT UP}")
//                return true
//            end
//            return false
//        end #OnRButtonUp
//        # ----------------------------------
//        # OnRButtonDown
//        # ----------------------------------
//        def OnRButtonDown(flags, x, y)
//            Logit "FROMRUBY: OnRButtonDown flags:#{flags} x:#{x} y:#{y}"
//
//            # Treat a dragged right mouse button like a hand dolly (pan tool)
//            x1, y1 = GetMousePosition()
//            sleep(0.1)
//            x2, y2 = GetMousePosition()
//            if ( ((x2-x1).abs > 2) or ((y2-y1).abs > 2) )
//                Logit "FROMRUBY Mouse DRAGGING"
//                SendKeyMacro( "{LSHIFT DOWN}{MBUTTON DOWN}" )
//                @handDolly = true
//                return true
//            end
//            return false
//        end #OnRButtonDown
//        # ----------------------------------
//        #	Menus
//        # ----------------------------------
//        def OnMenuSelected(menuId, menuLabel)
//            Logit "FROMRUBY: OnMenuSelected MenuId:#{menuId} label:#{menuLabel}"
//
//            # Kill the F1 key Help menu. It's too close to my ESC key
//            #if (menuId == 57670) #Kill Help key
//            #	return true
//            #end
//            return false
//        end #OnMenuSelected
//    end # class MyEventRelay
//
//    # ------------------------------------------------------------------------
//    # The EventRelay extension will see the next statement and make
//    # a global reference to "myEventRelay" object class so it lives past
//    # the termination of this script. Anything outside MyEventRelay class will
//    # be eaten by the Ruby garbage collector.
//    # ------------------------------------------------------------------------
//
//    anEventRelay = MyEventRelay.new
//
//    #end of script
// ----------------------------------------------------------------------------
//  Key Symbols used by SendKeyMacro()
// ----------------------------------------------------------------------------
//    // Named Macro Table
//
//    The special chars !, ^, and + are used to represent Alt, Ctrl, and Shift
//    So they have to be escapted in a macro as "{!}", "{^}", and "{+}" .
//
//    The brace set '{' and '}' are used to delineate special keys. To use them
//    as ordinary characters in a macro, they must be surrounded by braces themselves.
//    IE., {{} and {}} correctly represent the left and right brace as characters.
//
//    "{ALT}"         "{BACKSPACE"     "{BS}"          "{DEL}"
//    "{DELETE}"      "{DOWN}"         "{END}"         "{ENTER}"
//    "{ESC}"         "{ESCAPE}"       "{F1}"          "{F2}"
//    "{F3}"          "{F4}"           "{F5}"          "{F6}"
//    "{F7}"          "{F8}"           "{F9}"          "{F10}"
//    "{F11}"         "{F12}"          "{HOME}"        "{INS}"
//    "{INSERT}"      "{LEFT}"         "{PGDN}"        "{PGUP}"
//    "{RIGHT}"       "{SPACE}"        "{TAB}"         "{UP}"
//    "{PRINTSCREEN}" "{LWIN}"         "{RWIN}"        "{SCROLLLOCK}"
//    "{NUMLOCK}"     "{CTRLBREAK}"    "{PAUSE}"       "{CAPSLOCK}"
//    "{NUMPAD0}"     "{NUMPAD1}"      "{NUMPAD2}"     "{NUMPAD3}"
//    "{NUMPAD4}"     "{NUMPAD5}"      "{NUMPAD6}"     "{NUMPAD7}"
//    "{NUMPAD8}"     "{NUMPAD9}"      "{NUMPADMULT}"  "{NUMPADADD}"
//    "{NUMPADSUB}"   "{NUMPADDOT}"    "{NUMPADDIV}"   "{MENU}"
//    "{LCTRL}"       "{RCTRL}"        "{LALT}"        "{RALT}"
//    "{LSHIFT}"      "{RSHIFT}"       "{SLEEP}"       "{NUMPADENTER}"
//    "{CTRLDOWN}"    "{CTRLUP}" 	   "{ALTDOWN}"     "{ALTUP}"
//    "{SHIFTDOWN}"   "{SHIFTUP}"      "{ASC}"         "{WAIT}"
//    "{LBUTTON}"     "{MBUTTON}"      "{RBUTTON}"
//    "{SHIFT}"       "{MENU}"         "{CONTROL}"
//    "{!}"           "{^}"            "{+}"           "{(}" "{}}"
//
//    Examples:
//    SendKeyMacro("!WR") is equivalent to Alt-Window-RubyConsole
//    SendKeyMacro("+ruby..for +gods sake, +turn around{!}") is equivalent to
//            "Ruby..for Gods sake, Turn around!"
//    SendKeyMacro( "{LSHIFT DOWN}{MBUTTON DOWN}" ) invokes panning
//    SendKeyMacro( "{MBUTTON UP}{LSHIFT UP}" ) releases panning
//    SendKeyMacro ("{^M}") turns on copy
//
//    Other macros, like {ALTDOWN}, depress the keys but do not release them
//    until an opposite macro, like {ALTUP}, appears in the key macro stream.
//    These "paired" macros can cause havoc if not entered as matching pairs.
//
//    Eg., {CTRLDOWN}{RIGHT}{RIGHT}{CTRLUP} moves the cursor to the right
//         while holding the Ctrl key depressed.
//
//    Key macros which cause movement may contain a numeric parameter.
//    Eg, {RIGHT 4} will move the cursor 4 positions to the right.
//
//       {WAIT 5} will pause for 5 milliseconds.
//       {ASC 0191} will push the char represented by ascii 191 into the keyboard.
//       {ENTER 2} will push 2 Return keys into the keyboard buffer.
//       {UP}{DOWN}{RIGHT}{LEFT} are others.
//
//    Special keys
//      !, ^, +, represent the keystrokes Alt, Ctrl, and Shift respectively.
//      They provide a modifier for the character key that follows it.
//      When the macro processor sees a "!c", it interprets it as an Alt-c;
//      a "+v" as a Shift-v; a "^t" as a Ctrl-t. The processor depresses the
//      modifier key, depresses the char key, releases the char key, then releases
//      the modifier key.
//
// ----------------------------------------------------------------------------
