// ----------------------------------------------------------------------------
//    Embedding Ruby in C++
//
//     This class embeds the Ruby interpreter in a C++ application
//
//
//    Compilation
//
//      The compilation of the example looks like
//        c++ -o example -I/usr/lib/ruby/1.6/i686-linux -L/usr/lib/ruby/1.6/i686-linux example.cpp rubyeval.cpp -lruby -ldl -lcrypt
//
//
//    Copying
//
//     This extension module is copyrighted free software by Pirmin Kalberer
//     You can redistribute it and/or modify it under the same term as
//     Ruby.
//
//
//    Pirmin Kalberer <pka@sourcepole.ch>
//    http://www.sourcepole.ch/sources/software/c++ruby/
// ----------------------------------------------------------------------------
/* $Id: rubyeval.h,v 1.6 2002/12/13 20:25:11 pi Exp $
 *
 * Copyright (C) 2002 Sourcepole AG.  All rights reserved.
 *
 */
// ----------------------------------------------------------------------------
// Embedded Ruby
// ----------------------------------------------------------------------------
#include "rubyeval.h"

#include <sstream>
#include "ruby.h"
#include "debugging.h"

using namespace std;

// ----------------------------------------------------------------------------
//  Globals/Statics
// ----------------------------------------------------------------------------
RubyEval* RubyEval::_instance = 0;

/** View for Interpreter output */
static ostream& curout = cout;

EXTERN VALUE ruby_errinfo;
EXTERN VALUE rb_defout;

// ----------------------------------------------------------------------------
static VALUE defout_write(VALUE self, VALUE str)
// ----------------------------------------------------------------------------
{
    /** Called by Ruby for writing to STDOUT */
    curout << RubyEval::val2str(str);
    #if defined(LOGGING)
    do{
        wxString outStr = RubyEval::val2str(str);
        // skip logging single newlines
        if ( outStr == _T("\n") )
            break; //{outStr = _T("\\n");}
        LOGIT( _T("[%s]"), outStr.c_str());
    }while(0);
    #endif

    return Qnil;
}
// ----------------------------------------------------------------------------
RubyEval* RubyEval::instance() {
  // ----------------------------------------------------------------------------
  if (!_instance) {
    _instance = new RubyEval();
  }
  return _instance;
}

// ----------------------------------------------------------------------------
RubyEval::RubyEval()
// ----------------------------------------------------------------------------
{
    //ctor

    ruby_init();
    ruby_init_loadpath();
    rb_set_safe_level(0);
    ruby_script("ruby");

    rb_defout = rb_str_new("", 0);
    rb_define_singleton_method(rb_defout, "write", (VALUE(*)(...))defout_write, 1);
}
// ----------------------------------------------------------------------------
RubyEval::~RubyEval()
// ----------------------------------------------------------------------------
{
    //dtor
     _instance = 0;
}
// ----------------------------------------------------------------------------
string RubyEval::val2str(const VALUE rval)
// ----------------------------------------------------------------------------
{
    /** Convert Ruby value to string */
    return STR2CSTR(rb_funcall(rval, rb_intern("to_s"), 0));
}
// ----------------------------------------------------------------------------
string RubyEval::strval2str(const VALUE rval)
// ----------------------------------------------------------------------------
{
    /** Convert Ruby string value to string */
    return string(RSTRING(rval)->ptr, RSTRING(rval)->len);
}
// ----------------------------------------------------------------------------
void RubyEval::run_file(const char* filename, ostream& out)
// ----------------------------------------------------------------------------
{
    /** Run Ruby interpreter with @p filename */
    //ruby_options(int argc, char **argv")
    rb_load_protect(rb_str_new2(filename), 0, &status);
    if (status) {
        exception_print(out);
    }
}

// ----------------------------------------------------------------------------
void RubyEval::exception_print(ostream& errout)
// ----------------------------------------------------------------------------
{
    /**  Get Ruby error/exception info an print it */
    //Adapted from eruby_main by Shugo Maeda <shugo@modruby.net>
    VALUE errat;
    VALUE eclass;
    VALUE einfo;
    wxString erroutStr = wxEmptyString;

    if (NIL_P(ruby_errinfo)) return;

    errat = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
    if (!NIL_P(errat))
    {
        VALUE mesg = RARRAY(errat)->ptr[0];

        if (NIL_P(mesg)) {
            ID last_func = rb_frame_last_func();
            #if defined(LOGGING)
            errout << rb_id2name(last_func);
            erroutStr = rb_id2name(last_func);
            LOGIT( _T("errout[%s]"), erroutStr.c_str());
            #endif
        } else {
            errout << strval2str(mesg);
            #if defined(LOGGING)
            erroutStr = strval2str(mesg);
            LOGIT( _T("errout[%s]"), erroutStr.c_str());
            #endif
        }
    }

    eclass = CLASS_OF(ruby_errinfo);
    einfo = rb_obj_as_string(ruby_errinfo);
    if (eclass == rb_eRuntimeError && RSTRING(einfo)->len == 0)
    {
        errout << ": unhandled exception";
        #if defined(LOGGING)
        LOGIT( _T("errout[%s]"), _T(": unhandled exception\n"));
        #endif
    } else {
        VALUE epath;

        epath = rb_class_path(eclass);
        if (RSTRING(einfo)->len == 0)
        {
            errout << ": ";
            errout << strval2str(epath);
            errout << "\n";
            #if defined(LOGGING)
            LOGIT( _T("errout[: %s]"),strval2str(epath).c_str());
            #endif
        }
        else
        {
            char *tail  = 0;
            int len = RSTRING(einfo)->len;

            if (RSTRING(epath)->ptr[0] == '#') epath = 0;
            if ((tail = strchr(RSTRING(einfo)->ptr, '\n')) != NULL) {
                len = tail - RSTRING(einfo)->ptr;
                tail++;   /* skip newline */
            }
            errout << ": ";
            errout << string(RSTRING(einfo)->ptr, len);
            erroutStr = ": " + string(RSTRING(einfo)->ptr, len);
            if (epath)
            {
                errout << " (";
                errout << strval2str(epath);
                errout << ")\n";
                erroutStr << " (" << strval2str(epath) << ")";
            }
            if (tail) {
                errout << string(tail, RSTRING(einfo)->len - len - 1);
                errout << "\n";
                erroutStr = erroutStr + string(tail, RSTRING(einfo)->len - len - 1);
            }
            if ( not erroutStr.IsEmpty() )
            {
                #if defined(LOGGING)
                LOGIT( _T("errout[%s]"), erroutStr.c_str());
                #endif
            }
        }//else
    }//else

    if (!NIL_P(errat))
    {
        int i;
        struct RArray *ep = RARRAY(errat);
        erroutStr = wxEmptyString;

        #define TRACE_MAX (TRACE_HEAD+TRACE_TAIL+5)
        #define TRACE_HEAD 8
        #define TRACE_TAIL 5

        rb_ary_pop(errat);
        ep = RARRAY(errat);
        for (i=1; i<ep->len; i++)
        {
            if (TYPE(ep->ptr[i]) == T_STRING)
            {
                errout << "        from ";
                errout << strval2str(ep->ptr[i]);
                errout << "\n";
                erroutStr = _T("        from ");
                erroutStr << strval2str(ep->ptr[i]);
            }
            if (i == TRACE_HEAD && ep->len > TRACE_MAX)
            {
                errout << "         ... " << (ep->len - TRACE_HEAD - TRACE_TAIL)
                    << " levels...\n";
                erroutStr = "         ... ";
                erroutStr << (ep->len - TRACE_HEAD - TRACE_TAIL)
                    << " levels...\n";
                i = ep->len - TRACE_TAIL;
            }
        }//for
        if ( not erroutStr.IsEmpty() )
        {
            #if defined(LOGGING)
            LOGIT( _T("errout[%s]"), erroutStr.c_str());
            #endif
        }
    }//if
    errout.flush();
    // Need pointer to EventRealyTestFrame -> m_pLogging->Flush();
    Debugging::pLog->Flush();
}
// ----------------------------------------------------------------------------
VALUE RubyEval::eval(const char* code)
// ----------------------------------------------------------------------------
{
    /** Evaluate code string and print erros & results */
    return rb_eval_string_protect(code, &status);
}
// ----------------------------------------------------------------------------
VALUE RubyEval::eval(const char* code, ostream& errout)
// ----------------------------------------------------------------------------
{
    /** Evaluate code string and print erros & results */
    VALUE ret = rb_eval_string_protect(code, &status);
    if (status) {
        exception_print(errout);
    }
    return ret;
}
// ----------------------------------------------------------------------------
bool RubyEval::evalOk()
// ----------------------------------------------------------------------------
{
    /** Last evaluation was successful */
    return status == 0;
}
// ----------------------------------------------------------------------------
string RubyEval::exception_info()
// ----------------------------------------------------------------------------
{
    /** Get exception message as string */
    stringstream errstr;
    exception_print(errstr);
    errstr << ends;
    return errstr.str();
}
// ----------------------------------------------------------------------------
//  Example
// ----------------------------------------------------------------------------
//// Emacs: -*-compile-command: "c++ -o example -I/usr/lib/ruby/1.6/i686-linux -L/usr/lib/ruby/1.6/i686-linux example.cpp rubyeval.cpp -lruby -ldl -lcrypt" -*-
//
//#include "rubyeval.h"
//#include "ruby.h"
//
//int main( int argc, char *argv[] ) {
//
//  RubyEval& ruby = *RubyEval::instance();
//
//  ruby.eval("puts 'hello ruby'");
//
//  assert( NUM2INT( ruby.eval("1+1") ) == 2 );
//
//  assert(RubyEval::val2str(ruby.eval("'Regexp'.gsub(/x/, 'X')")) == "RegeXp");
//
//  return 0;
//}
