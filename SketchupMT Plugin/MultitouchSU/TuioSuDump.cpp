/*
	TUIO C++ Example - part of the reacTIVision project
	http://reactivision.sourceforge.net/

	Copyright (c) 2005-2009 Martin Kaltenbrunner <mkalten@iua.upf.edu>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <wx/string.h>
#include "TuioSuDump.h"
#include "debugging.h"
#include "MultitouchSu.h"
#include "RubyClass.h"
#include "MtSuGlobals.h"
////extern wxString g_wxStringMsg;
////extern HWND     g_hWndSketchUp;
////extern UINT     g_MtSuUserMsgNum;
////extern structMtSuTuioData g_MtSuTuioData;
// ----------------------------------------------------------------------------
void TuioDump::addTuioObject(TuioObject *tobj)
// ----------------------------------------------------------------------------
{
	//std::cout << "add obj " << tobj->getSymbolID() << " (" << tobj->getSessionID() << ") "<< tobj->getX() << " " << tobj->getY() << " " << tobj->getAngle() << std::endl;
	wxString msg = _T("");
	msg  << _T("add obj ") << tobj->getSymbolID() << _T(" (") << tobj->getSessionID() << _T(") ")<< tobj->getX() << _T(" ") << tobj->getY() << _T(" ") << tobj->getAngle() ; //_T("\n");

	#if defined(LOGGING_TUIO)
	 LOGIT( _T("[%s]"), msg.c_str());
	#endif

    // Post the data to SketchUp to allowing synchronous entry into embeded Ruby
    g_MtSuTuioData.clear();
	g_MtSuTuioData.symbolID     = tobj->getSymbolID();
	g_MtSuTuioData.sessionID    = tobj->getSessionID();
    g_MtSuTuioData.positionX    = tobj->getX();
	g_MtSuTuioData.positionY    = tobj->getY();
    g_MtSuTuioData.angle        = tobj->getAngle() ;
    g_MtSuTuioData.packetMsg    = msg;
    WORD wParam = 0;
    LPARAM lParam = (LPARAM)tobj;
    long lresult = ::SendMessage( g_hWndSketchUp, g_MtSuUserMsgNum, wParam, lParam);
    if (lresult) ; //shush unused warning
}
// ----------------------------------------------------------------------------
void TuioDump::updateTuioObject(TuioObject *tobj)
// ----------------------------------------------------------------------------
{
	//std::cout << "set obj " << tobj->getSymbolID() << " (" << tobj->getSessionID() << ") "<< tobj->getX() << " " << tobj->getY() << " " << tobj->getAngle()
	//			<< " " << tobj->getMotionSpeed() << " " << tobj->getRotationSpeed() << " " << tobj->getMotionAccel() << " " << tobj->getRotationAccel() << std::endl;
	wxString msg = _T("");
	msg << _T("set obj ") << tobj->getSymbolID() << _T(" (") << tobj->getSessionID() << _T(") ")<< tobj->getX() << _T(" ") << tobj->getY() << _T(" ") << tobj->getAngle()
        << _T(" ") << tobj->getMotionSpeed() << _T(" ") << tobj->getRotationSpeed() << _T(" ") << tobj->getMotionAccel() << _T(" ") << tobj->getRotationAccel() ; //_T("\n");

        #if defined(LOGGING_TUIO)
         LOGIT( _T("[%s]"), msg.c_str());
        #endif

    // Post the data to SketchUp to allowing synchronous entry into embeded Ruby
    g_MtSuTuioData.clear();

	g_MtSuTuioData.symbolID     = tobj->getSymbolID();
	g_MtSuTuioData.sessionID    = tobj->getSessionID();
    g_MtSuTuioData.positionX    = tobj->getX();
	g_MtSuTuioData.positionY    = tobj->getY();
    g_MtSuTuioData.angle        = tobj->getAngle() ;

	g_MtSuTuioData.motionSpeed   = tobj->getMotionSpeed();
	g_MtSuTuioData.rotationSpeed = tobj->getRotationSpeed();
	g_MtSuTuioData.motionAccel   = tobj->getMotionAccel();
	g_MtSuTuioData.rotationAccel = tobj->getRotationAccel();

    g_MtSuTuioData.packetMsg    = msg;
    WORD wParam = 0;
    LPARAM lParam = (LPARAM)tobj;

    long lresult = ::SendMessage( g_hWndSketchUp, g_MtSuUserMsgNum, wParam, lParam);
    if (lresult) ;
}
// ----------------------------------------------------------------------------
void TuioDump::removeTuioObject(TuioObject *tobj)
// ----------------------------------------------------------------------------
{
	//std::cout << "del obj " << tobj->getSymbolID() << " (" << tobj->getSessionID() << ")" << std::endl;
	wxString msg = _T("");
	msg << _T("del obj ") << tobj->getSymbolID() << _T(" (") << tobj->getSessionID() << _T(")") ; //_T("\n");

        #if defined(LOGGING_TUIO)
         LOGIT( _T("[%s]"), msg.c_str());
        #endif

    // Post the data to SketchUp to allowing synchronous entry into embeded Ruby
    g_MtSuTuioData.clear();

	g_MtSuTuioData.symbolID     = tobj->getSymbolID();
	g_MtSuTuioData.sessionID    = tobj->getSessionID();

    g_MtSuTuioData.packetMsg = msg;
    WORD wParam = 0;
    LPARAM lParam = (LPARAM)tobj;

    long lresult = ::SendMessage( g_hWndSketchUp, g_MtSuUserMsgNum, wParam, lParam);
    if (lresult) ;//shush unused compiler warning msg
}
// ----------------------------------------------------------------------------
void TuioDump::addTuioCursor(TuioCursor *tcur)
// ----------------------------------------------------------------------------
{
	//std::cout << "add cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY() << std::endl;
	wxString msg = _T("");
	msg << _T("add cur ") << tcur->getCursorID() << _T(" (") <<  tcur->getSessionID() << _T(") ") << tcur->getX() << _T(" ") << tcur->getY() ; //_T("\n");

    #if defined(LOGGING_TUIO)
     LOGIT( _T("[%s]"), msg.c_str());
    #endif
    // Post the data to SketchUp to allowing synchronous entry into embeded Ruby
    g_MtSuTuioData.clear();

	g_MtSuTuioData.symbolID     = tcur->getCursorID();
	g_MtSuTuioData.sessionID    = tcur->getSessionID();
    g_MtSuTuioData.positionX    = tcur->getX();
	g_MtSuTuioData.positionY    = tcur->getY();

    g_MtSuTuioData.packetMsg = msg;
    WORD wParam = 0;
    LPARAM lParam = (LPARAM)tcur;
    long lresult = ::SendMessage( g_hWndSketchUp, g_MtSuUserMsgNum, wParam, lParam);
    if (lresult) ;
}
// ----------------------------------------------------------------------------
void TuioDump::updateTuioCursor(TuioCursor *tcur)
// ----------------------------------------------------------------------------
{
	//std::cout << "set cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY()
	//			<< " " << tcur->getMotionSpeed() << " " << tcur->getMotionAccel() << " " << std::endl;
	wxString msg = _T("");
    msg << _T("set cur ") << tcur->getCursorID() << _T(" (") <<  tcur->getSessionID() << _T(") ") << tcur->getX() << _T(" ") << tcur->getY()
        << _T(" ") << tcur->getMotionSpeed() << _T(" ") << tcur->getMotionAccel() << _T(" ") ; //_T("\n");
    #if defined(LOGGING_TUIO)
     LOGIT( _T("[%s]"), msg.c_str());
    #endif
    // Post the data to SketchUp to allowing synchronous entry into embeded Ruby
    g_MtSuTuioData.clear();

	g_MtSuTuioData.symbolID     = tcur->getCursorID();
	g_MtSuTuioData.sessionID    = tcur->getSessionID();
    g_MtSuTuioData.positionX    = tcur->getX();
	g_MtSuTuioData.positionY    = tcur->getY();
	g_MtSuTuioData.motionSpeed  = tcur->getMotionSpeed();;
	g_MtSuTuioData.motionAccel  = tcur->getMotionAccel();

    g_MtSuTuioData.packetMsg = msg;
    WORD wParam = 0;
    LPARAM lParam = (LPARAM)tcur;

    long lresult = ::SendMessage( g_hWndSketchUp, g_MtSuUserMsgNum, wParam, lParam);
    if (lresult) ;
}
// ----------------------------------------------------------------------------
void TuioDump::removeTuioCursor(TuioCursor *tcur)
// ----------------------------------------------------------------------------
{
   	//std::cout << "del cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ")" << std::endl;
	wxString msg = _T("");
	msg << _T("del cur ") << tcur->getCursorID() << _T(" (") <<  tcur->getSessionID() << _T(")") ; //_T("\n");

    #if defined(LOGGING_TUIO)
     LOGIT( _T("[%s]"), msg.c_str());
    #endif

    // Post the data to SketchUp to allowing synchronous entry into embeded Ruby
    g_MtSuTuioData.clear();

	g_MtSuTuioData.symbolID     = tcur->getCursorID();
	g_MtSuTuioData.sessionID    = tcur->getSessionID();

    g_MtSuTuioData.packetMsg = msg;
    WORD wParam = 0;
    LPARAM lParam = (LPARAM)tcur;

    long lresult = ::SendMessage( g_hWndSketchUp, g_MtSuUserMsgNum, wParam, lParam);
    if (lresult) ;
}
// ----------------------------------------------------------------------------
void  TuioDump::refresh(TuioTime frameTime)
// ----------------------------------------------------------------------------
{
    //std::cout << "refresh " << frameTime.getTotalMilliseconds() << std::endl;
	//wxString msg = _T("");
	//msg << _T("refresh ") << frameTime.getTotalMilliseconds() ; //_T("\n");
    //#if defined(LOGGING)
    //LOGIT( _T("[%s]"), msg.c_str());
    //#endif
    //if( MultitouchSUApp::pRubyClassHandler )
    //    MultitouchSUApp::pRubyClassHandler->Call_Ruby_OnTuioDataMethod(msg);
    //g_MtSuTuioData.packetMsg
}

//// ----------------------------------------------------------------------------
//int TuioDump::main(int argc, char* argv[])
//// ----------------------------------------------------------------------------
//{
//	if( argc >= 2 && strcmp( argv[1], "-h" ) == 0 ){
//        	std::cout << "usage: TuioDump [port]\n";
//        	return 0;
//	}
//
//	int port = 3333;
//	if( argc >= 2 ) port = atoi( argv[1] );
//
//	TuioDump dump;
//	TuioClient client(port);
//	client.addTuioListener(&dump);
//	client.addTuioListener(this);
//	client.connect(true); // blocking I/O
//
//	return 0;
//}


