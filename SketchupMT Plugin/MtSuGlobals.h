/***
Spaced out for Comments and developer Credits and license info
*/

#ifndef MTSUGLOBALS_H_INCLUDED
#define MTSUGLOBALS_H_INCLUDED
// ----------------------------------------------------------------------------
//  Globals
// ----------------------------------------------------------------------------
extern  int     g_DllIsInitialized;                 //guard while initializing
extern  char*   g_lpszTitleToFind;                  //Title of window to find
extern  char*   g_lpszClassToFind;                  //Class of View window
extern  HWND    g_hWndSketchUp;                     //handle of SketchUp window
extern  HWND    g_hWndSketchUpView;                 //handle of SketchUp View subwindow
extern  HWND    g_hWndSketchUpStatusBar;            //handle of SketchUp StatusBar
extern  DWORD   g_dwCurrentProcessId;               //PID of SketchUp process
extern  LONG    OldWndProc;                         //Main windoww proc that we subclassed
//enum    {g_enumMtSuUserMsgNum = WM_USER+0x8001};    //MultiTouchSu WM_USER message id
extern  UINT    g_MtSuUserMsgNum;                   //variable of above
struct structMtSuTuioData
{
    int      sessionID;
    int      symbolID;
    float    positionX;
    float    positionY;
    //float    positionZ;
    float    angle;
    //float    angleA;
    //float    angleB;
    //float    angleC;
    //float    motionSpeedX;
    //float    motionSpeedY;
    //float    motionSpeedZ;
    float    motionSpeed;
    //float    rotatonSpeedA;
    //float    rotatonSpeedB;
    //float    rotatonSpeedC;
    float    rotationSpeed;
    float    motionAccel;
    float    rotationAccel;
    wxString packetMsg;

    void clear()
    {
        sessionID   = 0;
        symbolID    = 0;
        positionX   = 0;
        positionY   = 0;
        angle       = 0;
        motionSpeed = 0;
        rotationSpeed = 0;
        motionAccel   = 0;
        rotationAccel = 0;
        packetMsg = wxEmptyString;
    }
};
extern  structMtSuTuioData g_MtSuTuioData;

#define ZEROMEMORY(obj)   ::ZeroMemory(&obj, sizeof(obj))

#endif // MTSUGLOBALS_H_INCLUDED
