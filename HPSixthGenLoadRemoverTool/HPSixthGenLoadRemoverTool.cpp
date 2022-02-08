// HPSixthGenLoadRemoverTool.cpp : Defines the entry point for the application.
//


#include "framework.h"
#include "HPSixthGenLoadRemoverTool.h"
#include "LiveSplitServer.h"
#pragma comment(lib, "ComCtl32.Lib");
#pragma comment (lib, "Psapi.lib");
#pragma comment (lib,"Gdiplus.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#define BITS_PER_PIXEL   32
#define BYTES_PER_PIXEL  (BITS_PER_PIXEL / 8)
#define CANVAS_WIDTH 480
#define CANVAS_HEIGHT 270
#define MAX_LOADSTRING 100
#define DBOUT( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugStringW( os_.str().c_str() );  \
}
#if(_WIN32_WINNT >= 0x0603)
#define PW_RENDERFULLCONTENT    0x00000002
#endif /* _WIN32_WINNT >= 0x0603 */
//using namespace System;
// Global Variables:
cv::Mat hp3 = cv::imread("C:\\Users\\Pauldin\\hp3sixthgen\\hp3sixthgen\\imgs\\loading.png");
HMENU hMenu;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
BOOL fDrawRect;   // TRUE if bitmap rect. is dragged  
HWND hwndButton5, hwndStart, hwndStop, hwndPrint, hwndProcessList, hwndDrawableCanvas, hwndPreviewCanvas, hwndPause, hwndUnpause, hwndPSNR, hwndImage;
LiveSplitServer server;
HWND selHwnd;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;
RECT rcTarget;    // rectangle to receive bitmap 
POINT pt;         // x and y coordinates of cursor 
int convertedX, convertedY, convertedWidth, convertedHeight;
INITCOMMONCONTROLSEX icex;    // Structure for control initialization.
bool isInitialLoad = true;
const UINT valMin = 0;          // The range of values for the Up-Down control.
const UINT valMax = GetSystemMetrics(SM_CXSCREEN);
HRESULT hr;
HBITMAP refBmp;
UINT cyVScroll;              // Height of scroll bar arrow.
HWND hControl; // Handles to the controls.
HWND hwndUpDnEdtBdyX = NULL;
HWND hwndUpDnCtlX = NULL;
HWND hwndUpDnEdtBdyY = NULL;
HWND hwndUpDnCtlY = NULL;
HWND hwndUpDnEdtBdyW = NULL;
HWND hwndUpDnCtlW = NULL;
HWND hwndUpDnEdtBdyH = NULL;
HWND hwndUpDnCtlH = NULL;
cv::Mat cvMat;
   // Handles to the create controls functions.
HWND CreateUpDnBuddy(HWND, int, int);
HWND CreateUpDnCtl(HWND);
RECT selRect;
HANDLE loadRemover;
DWORD loadRemoverId;
BITMAP selBmp;
typedef std::vector<std::pair<HDC, RECT>> device_hdc;
typedef std::map<HMONITOR, device_hdc> device_hdc_map;
device_hdc monitors;
static bool isThreadWorking = false;
char selText[10];
// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
void                MyRegisterDrawCanvasClass(HINSTANCE hInstance);
void                MyRegisterPreviewCanvasClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DrawCanvasProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PrevCanvasProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    UpDownDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void PopulateProcessList();
BOOL CALLBACK enumWindowCallback(HWND, LPARAM);
void CaptureAnImage(HWND, HWND, bool, bool, bool);
DWORD WINAPI LoadRemover(LPVOID);
BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
void AddMenus(HWND);
void OpenFile();
void SaveFile();
void SaveFileAs();
float GetPSNR(HWND);
void LoadRefImage();
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    // TODO: Place code here.
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HPSIXTHGENLOADREMOVERTOOL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    MyRegisterDrawCanvasClass(hInstance);
    MyRegisterPreviewCanvasClass(hInstance);
    server = LiveSplitServer();
    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HPSIXTHGENLOADREMOVERTOOL));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

void MyRegisterDrawCanvasClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex_drawCanvas;

    wcex_drawCanvas.cbSize = sizeof(WNDCLASSEX);

    wcex_drawCanvas.style = CS_HREDRAW | CS_VREDRAW;
    wcex_drawCanvas.lpfnWndProc = DrawCanvasProc;
    wcex_drawCanvas.cbClsExtra = 0;
    wcex_drawCanvas.cbWndExtra = 0;
    wcex_drawCanvas.hInstance = hInstance;
    wcex_drawCanvas.hIcon = NULL;
    wcex_drawCanvas.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex_drawCanvas.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wcex_drawCanvas.lpszMenuName = NULL;
    wcex_drawCanvas.lpszClassName = TEXT("Drawable Canvas");
    wcex_drawCanvas.hIconSm = NULL;
    if (RegisterClassExW(&wcex_drawCanvas) == 0) {
        wchar_t error[40] = { 0 };
        swprintf(error, _countof(error), L"Error Registering Child %d\n", GetLastError());
        MessageBox(NULL, (LPCWSTR) error, TEXT("Error..."), MB_OK | MB_ICONERROR);
    }
}
void MyRegisterPreviewCanvasClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex_drawCanvas;

    wcex_drawCanvas.cbSize = sizeof(WNDCLASSEX);

    wcex_drawCanvas.style = CS_HREDRAW | CS_VREDRAW;
    wcex_drawCanvas.lpfnWndProc = PrevCanvasProc;
    wcex_drawCanvas.cbClsExtra = 0;
    wcex_drawCanvas.cbWndExtra = 0;
    wcex_drawCanvas.hInstance = hInstance;
    wcex_drawCanvas.hIcon = NULL;
    wcex_drawCanvas.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex_drawCanvas.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wcex_drawCanvas.lpszMenuName = NULL;
    wcex_drawCanvas.lpszClassName = TEXT("Preview Canvas");
    wcex_drawCanvas.hIconSm = NULL;
    if (RegisterClassExW(&wcex_drawCanvas) == 0) {
        wchar_t error[40] = { 0 };
        swprintf(error, _countof(error), L"Error Registering Child %d\n", GetLastError());
        MessageBox(NULL, (LPCWSTR)error, TEXT("Error..."), MB_OK | MB_ICONERROR);
    }
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HPSIXTHGENLOADREMOVERTOOL));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HPSIXTHGENLOADREMOVERTOOL);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 990, 540, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}
PAINTSTRUCT ps2;
POINT p;
COLORREF color = RGB(255, 255, 255);
HBRUSH brush = CreateSolidBrush(NULL_BRUSH);
HPEN pen = CreatePen(PS_SOLID, 5, NULL_PEN);
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    
    switch (message)
    {
    case WM_CREATE:
        {
            AddMenus(hWnd);
            hwndPSNR = CreateWindowW(
                L"STATIC",
                TEXT(""),
                WS_VISIBLE | WS_CHILD,
                900,
                150,
                150,
                20,
                hWnd,
                (HMENU)IDC_PSNR,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            hwndPause = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Pause to Livesplit",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                100,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_HWNDBUTTON2,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            hwndUnpause = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Unpause to Livesplit",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                125,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_HWNDBUTTON3,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            hwndStart = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Start",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                25,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_START,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndStop = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Stop",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                50,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_STOP,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndPrint = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Save Preview",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                75,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_PRINT,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            //EnableWindow(hwndStop, false);
            hwndProcessList = CreateWindowW(
                WC_COMBOBOX,
                TEXT(""),
                CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VSCROLL | EM_SETREADONLY | WS_VISIBLE,
                200, 0, 400, 200,
                hWnd,
                NULL,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            hwndButton5 = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Refresh",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                605,         // x position 
                0,         // y position 
                75,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_HWNDBUTTON5,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            PopulateProcessList();
            SendMessage(hwndProcessList, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);
            hwndDrawableCanvas = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                TEXT("Drawable Canvas"),
                TEXT(""),
                WS_CHILDWINDOW | SS_BITMAP | WS_BORDER | WS_VISIBLE,
                5, 200, CANVAS_WIDTH, CANVAS_HEIGHT,
                hWnd,
                NULL,
                GetModuleHandle(NULL),
                NULL);
            hwndPreviewCanvas = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                TEXT("Preview Canvas"),
                L"",
                WS_CHILDWINDOW | SS_BITMAP | WS_BORDER | WS_VISIBLE | SS_NOTIFY,
                490, 200, CANVAS_WIDTH, CANVAS_HEIGHT,
                hWnd,
                (HMENU)ID_PREVIEW,
                GetModuleHandle(NULL),
                NULL);
            hwndImage = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"Static",
                L"Picture Box",
                WS_CHILD | SS_BITMAP | WS_BORDER | WS_VISIBLE,
                705, 5, 50, 50,
                hWnd,
                NULL,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            EnableWindow(hwndStop, false);
            break;
        }
    case WM_COMMAND:
        {
            //DBOUT("Message: " << std::hex << message << "\n");
            int wmId = LOWORD(wParam);
            int hWmID = HIWORD(wParam);
            wchar_t buf[100];
            // Parse the menu selections:
            swprintf(buf, _countof(buf), L"WM_COMMAND %d %d\n", hWmID, CBN_SELCHANGE);
            OutputDebugStringW(buf);

            if (hWmID == CBN_SELCHANGE) {
                OutputDebugStringW(L"CBN_SELCHANGE\n");
                int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL,
                    (WPARAM)0, (LPARAM)0);
                LPWSTR ListItem[256];
                
                (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,
                    (WPARAM)ItemIndex, (LPARAM)ListItem);
                //DBOUT("ListItem " << ListItem << "\n");
                if (wcsstr((wchar_t*)ListItem, L"Screen")) {
                    CaptureAnImage(hwndDrawableCanvas, NULL, false, false, false);
                    selHwnd = NULL;
                }
                else {
                    selHwnd = FindWindowW(NULL, (LPCWSTR)ListItem);
                }
                
                if (selHwnd == NULL) {
                    selRect = RECT{ 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
                    //wchar_t error[40] = { 0 };
                    //swprintf(error, _countof(error), L"Error Registering Child %d\n", GetLastError());
                    //MessageBox(NULL, (LPCWSTR)error, TEXT("Error..."), MB_OK | MB_ICONERROR);
                }
                else {
                    GetClientRect(selHwnd, &selRect);
                }
                isInitialLoad = false;
                SendMessage(hwndDrawableCanvas, WM_PAINT, 0, 0);
                if (!IsRectEmpty(&rcTarget)) {
                    convertedX = (rcTarget.left * (selRect.right - selRect.left) / CANVAS_WIDTH);
                    convertedY = (rcTarget.top * (selRect.bottom - selRect.top) / CANVAS_HEIGHT);
                    convertedWidth = ((selRect.right - selRect.left) * (rcTarget.right - rcTarget.left)) / CANVAS_WIDTH;
                    convertedHeight = ((selRect.bottom - selRect.top) * (rcTarget.bottom - rcTarget.top)) / CANVAS_HEIGHT;
                    SendMessage(hwndUpDnCtlX, UDM_SETPOS, 0, convertedX);
                    SendMessage(hwndUpDnCtlY, UDM_SETPOS, 0, convertedY);
                    SendMessage(hwndUpDnCtlW, UDM_SETPOS, 0, convertedWidth);
                    SendMessage(hwndUpDnCtlH, UDM_SETPOS, 0, convertedHeight);
                }
            }
            if (hWmID == STN_CLICKED && wmId == ID_PREVIEW) {
                OutputDebugStringW(L"ID_PREVIEW\n");
                GetCursorPos(&p);
                ScreenToClient(hwndPreviewCanvas, &p);
                HDC hdcDraw = GetDC(hwndPreviewCanvas);
                color = GetPixel(hdcDraw, p.x, p.y);
                DBOUT(color << "\n");
                brush = CreateSolidBrush(color);
                DBOUT(GetRValue(color) << " " << GetGValue(color) << " " << GetBValue(color) << "\n");
                ReleaseDC(hwndPreviewCanvas, hdcDraw);
                
            }
            switch (wmId)
            {
            case ID_FILE_OPEN:
                OpenFile();
                break;
            case ID_IMAGE_LOAD:
                LoadRefImage();
                break;
            case ID_FILE_SAVE:
                SaveFile();
                break;
            case ID_FILE_SAVE_AS:
                SaveFileAs();
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_HWNDBUTTON2:
                server.send_to_ls("pausegametime\r\n");
                break;
            case ID_HWNDBUTTON3:
                server.send_to_ls("unpausegametime\r\n");
                break;
            case ID_HWNDBUTTON5:
                PopulateProcessList();
                SendMessage(hwndProcessList, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);
                break;
            case ID_START:
                if (!server.open_pipe()) {
                    DBOUT("Couldn't open Livesplit pipe.\n");
                    break;
                }
                if (!IsRectEmpty(&rcTarget)) {
                    isThreadWorking = true;
                    loadRemover = CreateThread(
                        NULL,                   // default security attributes
                        0,                      // use default stack size  
                        LoadRemover,       // thread function name
                        NULL,          // argument to thread function 
                        0,                      // use default creation flags 
                        &loadRemoverId);   // returns the thread identifier 
                    if (loadRemover == NULL)
                    {
                        DBOUT(TEXT("Can't create thread.\n"));
                        break;
                    }
                    EnableWindow(hwndStart, false);
                    EnableWindow(hwndStop, true);
                }
                break;
            case ID_STOP:
                isThreadWorking = false;
                WaitForSingleObject(loadRemover, 0);
                CloseHandle(loadRemover);
                EnableWindow(hwndStop, false);
                EnableWindow(hwndStart, true);
                server.close_pipe();
                break;
            case ID_PRINT:
                CaptureAnImage(selHwnd, (HWND)NULL, false, false, true);
                break;
            }
            
        }
        break;
    case WM_PAINT:
        {
            DBOUT("WM_PAINT Main\n");
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;
        }
    case WM_DESTROY:
        DWORD ret;
        if (GetExitCodeThread(loadRemover, &ret)) {
            if (ret == STILL_ACTIVE) {
                isThreadWorking = false;
                WaitForSingleObject(loadRemover, 0);
                CloseHandle(loadRemover);
            }
        }
        DeleteObject(brush);
        DeleteObject(pen);
        PostQuitMessage(0);
        break;
    case WM_NOTIFY:
    {
        //FIX
        UINT nCode = ((LPNMHDR)lParam)->code;
        switch (nCode) {
        case UDN_DELTAPOS:
            LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
            if (!IsRectEmpty(&rcTarget)) {
                DBOUT("Before: " << rcTarget.left << " " << rcTarget.top << " " << rcTarget.right << " " << rcTarget.bottom << std::endl);
                //convertedX = (rcTarget.left * (selRect.right - selRect.left) / CANVAS_WIDTH);
                //convertedY = (rcTarget.top * (selRect.bottom - selRect.top) / CANVAS_HEIGHT);
                //convertedWidth = ((selRect.right - selRect.left) * (rcTarget.right - rcTarget.left)) / CANVAS_WIDTH;
                //convertedHeight = ((selRect.bottom - selRect.top) * (rcTarget.bottom - rcTarget.top)) / CANVAS_HEIGHT;

                int l = rcTarget.left;
                int t = rcTarget.top;
                int r = rcTarget.right;
                int b = rcTarget.bottom;
                int newPos = lpnmud->iPos + lpnmud->iDelta;
                InvalidateRect(hWnd, NULL, true);
                
                if (lpnmud->hdr.hwndFrom == hwndUpDnCtlX) {
                    convertedX = newPos;
                    l = CANVAS_WIDTH * convertedX / (selRect.right - selRect.left);
                    r = l + (CANVAS_WIDTH * convertedWidth / (selRect.right - selRect.left));
                }
                else if (lpnmud->hdr.hwndFrom == hwndUpDnCtlY) {
                    convertedY = newPos;
                    t = CANVAS_HEIGHT * convertedY / (selRect.bottom - selRect.top);
                    b = t + (CANVAS_HEIGHT * convertedHeight / (selRect.bottom - selRect.top));
                }
                else if (lpnmud->hdr.hwndFrom == hwndUpDnCtlW) {
                    convertedWidth = newPos;
                    r = l + (CANVAS_WIDTH * convertedWidth / (selRect.right - selRect.left));
                }
                else {
                    convertedHeight = newPos;
                    b = t + (CANVAS_HEIGHT * convertedHeight / (selRect.bottom - selRect.top));
                }
                SetRect(&rcTarget, l, t, r, b);
                DBOUT("After: " << rcTarget.left << " " << rcTarget.top << " " << rcTarget.right << " " << rcTarget.bottom << std::endl);
            }
            break;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        if (hwndPreviewCanvas == (HWND)lParam) {
            OutputDebugStringW(L"COLORSTATIC Preview\n");
            return (INT_PTR)NULL;
        }
        else {
            return DefWindowProc(hWnd, message, wParam, lParam);
            
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK DrawCanvasProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;                 // device context (DC) for window  
    RECT rcTmp;              // temporary rectangle  
    PAINTSTRUCT ps;          // paint data for BeginPaint and EndPaint  
    POINT ptClientUL;        // client area upper left corner  
    POINT ptClientLR;        // client area lower right corner  
    static HDC hdcCompat;    // DC for copying bitmap  
    
    static RECT rcClient;    // client-area rectangle  
    
    static HBRUSH hbrBkgnd;  // handle of background-color brush  
    static COLORREF crBkgnd; // color of client-area background  
    static HPEN hpenDot;     // handle of dotted pen 
    int i;
    switch (message)
    {
    case WM_CREATE:
    {
        //DBOUT("WM_CREATE Draw\n");
        // Create a device context (DC) to hold the bitmap.  
        // The bitmap is copied from this DC to the window's DC  
        // whenever it must be drawn.  

        hdc = GetDC(hWnd);
        hdcCompat = CreateCompatibleDC(hdc);

        // Create a brush of the same color as the background  
        // of the client area. The brush is used later to erase  
        // the old bitmap before copying the bitmap into the  
        // target rectangle.  

        crBkgnd = GetBkColor(hdc);
        hbrBkgnd = CreateSolidBrush(COLORREF(NULL_BRUSH));
        ReleaseDC(hWnd, hdc);

        // Create a dotted pen. The pen is used to draw the  
        // bitmap rectangle as the user drags it.  

        //hpenDot = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
        break;
    }
    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);
        if (!isInitialLoad) {
            CaptureAnImage(hwndDrawableCanvas, selHwnd, false, false, false);
            CaptureAnImage(hwndPreviewCanvas, selHwnd, true, false, false);
            if (!IsRectEmpty(&rcTarget) && convertedWidth > 0 && convertedHeight > 0) {
                GetPSNR(selHwnd);
            }
            
        }
        EndPaint(hWnd, &ps);
        ReleaseDC(hWnd, hdc);
        break;
    }
    case WM_LBUTTONDOWN:
        //DBOUT("WM_LBUTTONDOWN\n");
        // Restrict the mouse cursor to the client area. This  
        // ensures that the window receives a matching  
        // WM_LBUTTONUP message.  

        //ClipCursor(&rcClient);

        // Save the coordinates of the mouse cursor.  

        pt.x = (LONG)LOWORD(lParam);
        pt.y = (LONG)HIWORD(lParam);
        fDrawRect = true;
        break;
    case WM_MOUSEMOVE:
        {
        //DBOUT("MOUSEMOVE\n");
            
            // Draw a target rectangle or drag the bitmap rectangle,  
            // depending on the status of the fDragRect flag.  

            if ((wParam && MK_LBUTTON) && fDrawRect)
            {
                //DBOUT("WM_MOUSEMOVE\n");
                // Set the mix mode so that the pen color is the  
                // inverse of the background color. The previous  
                // rectangle can then be erased by drawing  
                // another rectangle on top of it.  

                //SetROP2(hdc, R2_NOTXORPEN);

                // If a previous target rectangle exists, erase  
                // it by drawing another rectangle on top of it.  
                if (!IsRectEmpty(&rcTarget))
                {
                    InvalidateRect(hWnd, NULL, true);
                }


                // Save the coordinates of the target rectangle. Avoid  
                // invalid rectangles by ensuring that the value of  
                // the left coordinate is lesser than the  
                // right coordinate, and that the value of the top 
                // coordinate is lesser than the bottom coordinate. 
                int left, top, right, bottom;
                if ((pt.x < (LONG)LOWORD(lParam)) &&
                    (pt.y > (LONG) HIWORD(lParam)))
                {
                    left = pt.x;
                    top = HIWORD(lParam);
                    right = LOWORD(lParam);
                    bottom = pt.y;
                }
                else if ((pt.x > (LONG)LOWORD(lParam)) &&
                    (pt.y > (LONG)HIWORD(lParam)))
                {
                    left = LOWORD(lParam);
                    top = HIWORD(lParam);
                    right = pt.x;
                    bottom = pt.y;
                }
                else if ((pt.x > (LONG)LOWORD(lParam)) &&
                    (pt.y < (LONG)HIWORD(lParam)))
                {
                    left = LOWORD(lParam);
                    top = pt.y;
                    right = pt.x;
                    bottom = HIWORD(lParam);
                }
                else
                {
                    left = pt.x;
                    top = pt.y;
                    right = LOWORD(lParam);
                    bottom = HIWORD(lParam);
                }
                SetRect(&rcTarget, left, top, right,
                    bottom);
                convertedX = (rcTarget.left * (selRect.right - selRect.left) / CANVAS_WIDTH);
                convertedY = (rcTarget.top * (selRect.bottom - selRect.top) / CANVAS_HEIGHT);
                convertedWidth = ((selRect.right - selRect.left) * (rcTarget.right - rcTarget.left)) / CANVAS_WIDTH;
                convertedHeight = ((selRect.bottom - selRect.top) * (rcTarget.bottom - rcTarget.top)) / CANVAS_HEIGHT;
                SendMessage(hwndUpDnCtlX, UDM_SETPOS, 0, convertedX);
                SendMessage(hwndUpDnCtlY, UDM_SETPOS, 0, convertedY);
                SendMessage(hwndUpDnCtlW, UDM_SETPOS, 0, convertedWidth);
                SendMessage(hwndUpDnCtlH, UDM_SETPOS, 0, convertedHeight);

            }
        }
        break;
    case WM_LBUTTONUP:
        fDrawRect = false;
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        DeleteObject(hbrBkgnd);
        DeleteDC(hdcCompat);
        PostQuitMessage(0);

        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)NULL;
}

LRESULT CALLBACK  PrevCanvasProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:
        //BeginPaint(hwnd, &ps);

        ValidateRect(hwnd, NULL);
        //EndPaint(hwnd, &ps);
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void PopulateProcessList() {
    SendMessageW(hwndProcessList, (UINT)CB_RESETCONTENT, 0, 0);
    monitors.clear();
    // Get the list of process identifiers.
    unsigned int i;
    HDC mHDC = GetDC(NULL);
    SendMessageW(hwndProcessList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"Screen");
    /*if (!EnumDisplayMonitors(mHDC, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors))) {
        ReleaseDC(NULL, mHDC);
        return;
    }*/
    if (!EnumWindows(enumWindowCallback, 0))
    {
        ReleaseDC(NULL, mHDC);
        return;
    }
    ReleaseDC(NULL, mHDC);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData) {
    SendMessageW(hwndProcessList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"Screen");
    MONITORINFOEX mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(hMon, &mi))
    {
        HDC dc = CreateDC(NULL, mi.szDevice, NULL, NULL);
        if (dc)
            (*reinterpret_cast<device_hdc*>(pData)).push_back(std::make_pair(hdc, *lprcMonitor));
    }
    return TRUE;
}

BOOL CALLBACK enumWindowCallback(HWND hwnd, LPARAM lparam) {
    const DWORD TITLE_SIZE = 1024;
    WCHAR windowTitle[TITLE_SIZE];
    DWORD pid = 0;
    GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);
    wchar_t buf[256];

    int length = ::GetWindowTextLength(hwnd);
    if (IsWindowVisible(hwnd) && length > 0) {
        GetWindowThreadProcessId(hwnd, &pid);
        swprintf(buf, _countof(buf), L"%s %d", windowTitle, pid);
        SendMessageW(hwndProcessList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)windowTitle);
    }
    return TRUE;
}

void CaptureAnImage(HWND hWnd, HWND hWndSource, bool isDrawing, bool isLoadRemoverRunning, bool isPrinting)
{
    HDC hdcScreen;
    HDC hdcWindow;
    HDC hdcMemDC = NULL;
    HBITMAP hbmScreen = NULL;
    BITMAP bmpScreen;
    DWORD dwBytesWritten = 0;
    DWORD dwSizeofDIB = 0;
    char* lpbitmap = NULL;
    HANDLE hDIB = NULL;
    DWORD dwBmpSize = 0;
    int x, y, width, height;
    HBITMAP newBitmap = NULL;
    int error = 0;
    BITMAPINFO   bi;
    // Retrieve the handle to a display device context for the client 
    // area of the window.
    if (hWndSource == NULL) {
        hdcScreen = GetDC(NULL);
    }
    else {
        hdcScreen = GetDC(hWndSource);
    }
    
    hdcWindow = GetDC(hWnd);

    // Create a compatible DC, which is used in a BitBlt from the window DC.
    hdcMemDC = CreateCompatibleDC(hdcWindow);


    if (!hdcMemDC)
    {
        MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
        goto done;
    }

    if (!isLoadRemoverRunning && !isPrinting) {
        // Get the client area for size calculation.
        RECT rcClient, rcSource;
        GetClientRect(hWnd, &rcClient);
        if (hWndSource != NULL) {
           GetClientRect(hWndSource, &rcSource);
        }

        // This is the best stretch mode.
        SetStretchBltMode(hdcWindow, HALFTONE);

        if (isDrawing || isLoadRemoverRunning) {
            if (isDrawing && fDrawRect) {
                convertedX = (rcTarget.left * (selRect.right - selRect.left) / CANVAS_WIDTH);
                convertedY = (rcTarget.top * (selRect.bottom - selRect.top) / CANVAS_HEIGHT);
                convertedWidth = ((selRect.right - selRect.left) * (rcTarget.right - rcTarget.left)) / CANVAS_WIDTH;
                convertedHeight = ((selRect.bottom - selRect.top) * (rcTarget.bottom - rcTarget.top)) / CANVAS_HEIGHT;
            }
            x = convertedX;
            y = convertedY;
            width = convertedWidth;
            height = convertedHeight;
        }
        else {
            x = 0;
            y = 0;
            if (hWndSource == NULL) {
                width = GetSystemMetrics(SM_CXSCREEN);
                height = GetSystemMetrics(SM_CYSCREEN);
            }
            else {
                width = rcSource.right;
                height = rcSource.bottom;
            }
        }
        // The source DC is the entire screen, and the destination DC is the current window (HWND).
        if (!StretchBlt(hdcWindow,
            0, 0,
            rcClient.right, rcClient.bottom,
            hdcScreen,
            x, y,
            width,
            height,
            SRCCOPY))
        {
            wchar_t error[40] = { 0 };
            swprintf(error, _countof(error), L"StretchBlt has failed %d\n", GetLastError());
            MessageBox(NULL, (LPCWSTR)error, TEXT("Error..."), MB_OK | MB_ICONERROR);
            goto done;
        }
        if (!isDrawing) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
            SelectObject(hdcWindow, GetStockObject(NULL_BRUSH));
            SelectObject(hdcWindow, (HGDIOBJ)pen);
            Rectangle(hdcWindow, rcTarget.left, rcTarget.top,
                rcTarget.right, rcTarget.bottom);

            //FrameRect(hdc, &rcTarget, CreateSolidBrush(RGB(0,0,255)));
            DeleteObject(pen);
            
        }
        error = 1;
        
    }
    if ((isDrawing && !IsRectEmpty(&rcTarget))|| isPrinting) {
        // Create a compatible bitmap from the Window DC.
        hbmScreen = CreateCompatibleBitmap(hdcWindow, convertedWidth, convertedHeight);
        if (!hbmScreen)
        {
            MessageBox(selHwnd, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
            error = 1;
            goto done;
        }
        // Select the compatible bitmap into the compatible memory DC.
        SelectObject(hdcMemDC, hbmScreen);

        // Bit block transfer into our compatible memory DC.
        if (!BitBlt(hdcMemDC,
            0, 0,
            convertedWidth, convertedHeight,
            hdcWindow,
            convertedX, convertedY,
            SRCCOPY))
        {
            MessageBox(selHwnd, L"BitBlt has failed", L"Failed", MB_OK);
            error = 1;
            goto done;
        }
        // Get the BITMAP from the HBITMAP.
        GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);
        selBmp = bmpScreen;
        //SendMessage(hwndPreviewCanvas, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmScreen);
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = bmpScreen.bmWidth;
        bi.bmiHeader.biHeight = -bmpScreen.bmHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = BITS_PER_PIXEL;
        bi.bmiHeader.biCompression = BI_RGB;
        bi.bmiHeader.biSizeImage = 0;
        bi.bmiHeader.biXPelsPerMeter = 0;
        bi.bmiHeader.biYPelsPerMeter = 0;
        bi.bmiHeader.biClrUsed = 0;
        bi.bmiHeader.biClrImportant = 0;

        dwBmpSize = ((bmpScreen.bmWidth * bi.bmiHeader.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
        
        //ClearBackground(hbmScreen);
        HANDLE hDIB = NULL;
        unsigned char* lpbitmap = NULL;
        HANDLE hFile = NULL;
        DWORD dwSizeofDIB = 0;
        DWORD dwBytesWritten = 0;
        // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
        // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
        // have greater overhead than HeapAlloc.
        hDIB = GlobalAlloc(GHND, dwBmpSize);
        lpbitmap = (unsigned char*)GlobalLock(hDIB);
        BITMAPFILEHEADER   bmfHeader;

        // Gets the "bits" from the bitmap, and copies them into a buffer 
        // that's pointed to by lpbitmap.
        cvMat.create(convertedHeight, convertedWidth, CV_8UC4);
        GetDIBits(hdcWindow, hbmScreen, 0,
            (UINT)bmpScreen.bmHeight,
            cvMat.data,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        GetDIBits(hdcWindow, hbmScreen, 0,
            (UINT)bmpScreen.bmHeight,
            lpbitmap,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        // A file is created, this is where we will save the screen capture.
        hFile = CreateFile(L"captureqwsx.bmp",
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
        // Add the size of the headers to the size of the bitmap to get the total file size.
        dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        // Offset to where the actual bitmap bits start.
        bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

        // Size of the file.
        bmfHeader.bfSize = dwSizeofDIB;

        // bfType must always be BM for Bitmaps.
        bmfHeader.bfType = 0x4D42; // BM.
        if (isPrinting) {
            WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
            WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
            WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);
            cv::imwrite("test.png", cvMat);
        }
        

        // Unlock and Free the DIB from the heap.
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);

        // Close the handle for the file that was created.
        CloseHandle(hFile);
    }
    

done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    if (hWndSource == NULL) {
        ReleaseDC(NULL, hdcScreen);
    }
    else {
        ReleaseDC(hWndSource, hdcScreen);
    }
    ReleaseDC(hWnd, hdcWindow);
}

DWORD WINAPI LoadRemover(LPVOID lpParam)
{
    float ret;
    while (isThreadWorking) {
        ret = GetPSNR(selHwnd);
        if (ret > 17 && !server.get_isLoading()) {
            server.send_to_ls("pausegametime\r\n");
            server.set_isLoading(true);
        }
        else if (ret <= 17 && server.get_isLoading()) {
            server.send_to_ls("unpausegametime\r\n");
            server.set_isLoading(false);
        }
    }
    return 0;
}

void RGBtoHSL(BYTE r, BYTE g, BYTE b, byte (&hsl)[3]) {
    float fR = r / 255.0;
    float fG = g / 255.0;
    float fB = b / 255.0;


    float fCMin = std::min({fR, fG, fB});
    float fCMax = std::max({ fR, fG, fB });


    hsl[2] = 50 * (fCMin + fCMax);

    if (fCMin == fCMax)
    {
        hsl[1] = 0;
        hsl[0] = 0;
        return;

    }
    else if (hsl[2] < 50)
    {
        hsl[1] = 100 * (fCMax - fCMin) / (fCMax + fCMin);
    }
    else
    {
        hsl[1] = 100 * (fCMax - fCMin) / (2.0 - fCMax - fCMin);
    }

    if (fCMax == fR)
    {
        hsl[0] = 60 * (fG - fB) / (fCMax - fCMin);
    }
    if (fCMax == fG)
    {
        hsl[0] = 60 * (fB - fR) / (fCMax - fCMin) + 120;
    }
    if (fCMax == fB)
    {
        hsl[0] = 60 * (fR - fG) / (fCMax - fCMin) + 240;
    }
    if (hsl[0] < 0)
    {
        hsl[0] = hsl[0] + 360;
    }
}

int hue2rgb(int p, int q, int t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1 / 6) return p + (q - p) * 6 * t;
    if (t < 1 / 2) return q;
    if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
    return p;
}

int* HSLtoRGB(int h, int s, int l) {
    int r, g, b;
    int rgb[3];
    if (s == 0) {
        r = g = b = l; // achromatic
    }
    else {
        int q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        int p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }
    rgb[0] = std::round(r * 255);
    rgb[1] = std::round(g * 255);
    rgb[2] = std::round(b * 255);
    return rgb;
}

void AddMenus(HWND hWnd) {
    hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hAboutMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, L"Open");
    AppendMenu(hFileMenu, MF_STRING, ID_IMAGE_LOAD, L"Load Image");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE, L"Save");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE_AS, L"Save As");
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, L"Exit");
    AppendMenu(hAboutMenu, MF_STRING, IDM_ABOUT, L"About");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hAboutMenu, L"About");
    SetMenu(hWnd, hMenu);
    cyVScroll = GetSystemMetrics(SM_CYVSCROLL);   
    hwndUpDnEdtBdyX = CreateUpDnBuddy(hWnd, 0, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlX = CreateUpDnCtl(hWnd);    // Create an Up-Down control.
    hwndUpDnEdtBdyY = CreateUpDnBuddy(hWnd, 105, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlY = CreateUpDnCtl(hWnd);    // Create an Up-Down control.
    hwndUpDnEdtBdyW = CreateUpDnBuddy(hWnd, 210, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlW = CreateUpDnCtl(hWnd);    // Create an Up-Down control.
    hwndUpDnEdtBdyH = CreateUpDnBuddy(hWnd, 315, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlH = CreateUpDnCtl(hWnd);    // Create an Up-Down control.
}

void OpenFile() {
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
}

void LoadRefImage() {
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        Gdiplus::Bitmap::FromFile(pszFilePath);
                        std::wstringstream ss;
                        ss << pszFilePath;
                        std::string str;
                        std::wstring_convert<std::codecvt_utf8<wchar_t>> ccvt;
                        str = ccvt.to_bytes(ss.str());
                        hp3 = cv::imread(str);
                        //refBmp = (HBITMAP) LoadImageW(NULL, str, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
                        //SendMessage(hwndImage, STM_SETIMAGE, 0, (LPARAM)refBmp);
                        MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
}

void SaveFileAs() {

}

void SaveFile() {

}


HWND CreateUpDnBuddy(HWND hwndParent, int x, int y)
{
    icex.dwICC = ICC_STANDARD_CLASSES;    // Set the Initialization Flag value.
    //InitCommonControlsEx(&icex);          // Initialize the Common Controls Library to use 
                                          // Buttons, Edit Controls, Static Controls, Listboxes, 
                                          // Comboboxes, and  Scroll Bars.

    hControl = CreateWindowEx(WS_EX_LEFT | WS_EX_CLIENTEDGE | WS_EX_CONTEXTHELP,    //Extended window styles.
        WC_EDIT,
        NULL,
        WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER    // Window styles.
        | ES_NUMBER | ES_LEFT,                     // Edit control styles.
        x, y,
        100, 25,
        hwndParent,
        NULL,
        hInst,
        NULL);

    return (hControl);
}

HWND CreateUpDnCtl(HWND hwndParent)
{
    icex.dwICC = ICC_UPDOWN_CLASS;    // Set the Initialization Flag value.
    //InitCommonControlsEx(&icex);      // Initialize the Common Controls Library.

    hControl = CreateWindowEx(WS_EX_LEFT | WS_EX_LTRREADING,
        UPDOWN_CLASS,
        NULL,
        WS_CHILDWINDOW | WS_VISIBLE
        | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
        0, 0,
        0, 0,         // Set to zero to automatically size to fit the buddy window.
        hwndParent,
        NULL,
        hInst,
        NULL);

    SendMessage(hControl, UDM_SETRANGE, 0, MAKELPARAM(valMax, valMin));    // Sets the controls direction 
                                                                           // and range.

    return (hControl);
}

float GetPSNR(HWND hwnd) {
    //cv::imwrite("test.png", cvMat);
    float PSNR = 0.0;
    // get handles to a device context (DC)
    HDC hwindowDC = GetDC(hwnd);
    HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    // create mat object
    cvMat.create(convertedHeight, convertedWidth, CV_8UC4);

    // create a bitmap
    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, convertedWidth, convertedHeight);
    BITMAPINFOHEADER  bi;

    // create a bitmap
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = convertedWidth;
    bi.biHeight = -convertedHeight;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);

    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, convertedWidth, convertedHeight, hwindowDC, convertedX, convertedY, convertedWidth, convertedHeight, SRCCOPY);  //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, convertedHeight, cvMat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);            //copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(hwnd, hwindowDC);

    cv::Mat resized, grayResized, srcGray;
    cv::resize(hp3, resized, cv::Size(convertedWidth, convertedHeight), cv::INTER_LINEAR);
    cv::cvtColor(resized, grayResized, cv::COLOR_BGR2GRAY);
    cv::cvtColor(cvMat, srcGray, cv::COLOR_BGR2GRAY);
    try {
        PSNR = cv::PSNR(grayResized, srcGray);
        int dec, sign;
        std::wostringstream ss;
        ss << PSNR;
        SetWindowText(hwndPSNR, (LPCWSTR)ss.str().c_str());
        RedrawWindow(hwndPSNR, NULL, NULL, RDW_ERASE);
    }
    catch (std::exception& e) {
        DBOUT(e.what() << "\n");
    }
    return PSNR;
}
