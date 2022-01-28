// HPSixthGenLoadRemoverTool.cpp : Defines the entry point for the application.
//
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "framework.h"
#include "HPSixthGenLoadRemoverTool.h"
#include "LiveSplitServer.h"
#include "DrawablePicture.h"
#pragma comment (lib, "Psapi.lib");
#pragma comment (lib,"Gdiplus.lib")
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
// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hwndText, hwndPort, hwndConnectLivesplit, hwndButton4, hwndButton5, hwndStart, hwndStop, hwndPrint, hwndProcessList, hwndDrawableCanvas, hwndPreviewCanvas;
LiveSplitServer server;
HWND selHwnd;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;
RECT rcTarget;    // rectangle to receive bitmap 
POINT pt;         // x and y coordinates of cursor 
int convertedX, convertedY, convertedWidth, convertedHeight;
RECT selRect;
tesseract::TessBaseAPI tess;
HANDLE loadRemover;
DWORD loadRemoverId;
typedef std::vector<std::pair<HDC, RECT>> device_hdc;
typedef std::map<HMONITOR, device_hdc> device_hdc_map;
device_hdc monitors;
static bool isThreadWorking = false;
// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
void                MyRegisterDrawCanvasClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DrawCanvasProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                DrawWizardCard(HDC, HWND);
VOID                OnPaint(HDC);
void PopulateProcessList();
BOOL CALLBACK enumWindowCallback(HWND, LPARAM);
void InitializeTesseract();
unsigned char* CaptureAnImage(HWND, HWND, bool, bool, bool);
unsigned char* CaptureAnImage(HWND, HDC, RECT, bool, bool, bool);
DWORD WINAPI LoadRemover(LPVOID);
BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);

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
    InitializeTesseract();
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HPSIXTHGENLOADREMOVERTOOL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    MyRegisterDrawCanvasClass(hInstance);
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
      CW_USEDEFAULT, 0, 975, 540, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

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
            hwndText = CreateWindowW(
                TEXT("EDIT"),
                TEXT("VALUE"),
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                50,
                150,
                20,
                hWnd,
                (HMENU)NULL,
                NULL,
                NULL
            );
            hwndPort = CreateWindowW(
                TEXT("EDIT"),
                TEXT("PORT"),
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                75,
                150,
                20,
                hWnd,
                (HMENU)NULL,
                NULL,
                NULL
            );
            //DBOUT("Message: " << std::hex << message << "\n");
            hwndConnectLivesplit = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Connect to Livesplit",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                0,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)111,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndButton4 = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Disconnect Server",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                25,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)114,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndStart = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Start",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                100,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)116,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndStop = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Stop",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                125,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)117,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndPrint = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Save Preview",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                150,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)118,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            EnableWindow(hwndButton4, false);
            EnableWindow(hwndStart, true);
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
                (HMENU)115,       // No menu.
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
                L"Static",
                L"Picture Box",
                WS_CHILDWINDOW | SS_BITMAP | WS_BORDER | WS_VISIBLE,
                490, 200, CANVAS_WIDTH, CANVAS_HEIGHT,
                hWnd,
                NULL,
                GetModuleHandle(NULL),
                NULL);
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
                
                SendMessageW(hwndDrawableCanvas, WM_PAINT, 0, 0);
            }
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_HWNDBUTTON:
                try {
                    server = LiveSplitServer();
                    if (server.ls_connect()) {
                        throw std::exception("Unable to connect to server\n");
                    }
                    EnableWindow(hwndConnectLivesplit, false);
                    EnableWindow(hwndButton4, true);
                    EnableWindow(hwndStart, true);
                }
                catch (std::exception& e) {
                    std::cout << e.what() << std::endl;
                }
                break;
            case ID_HWNDBUTTON2:
                server.send_to_ls("pausegametime\r\n");
                break;
            case ID_HWNDBUTTON3:
                server.send_to_ls("unpausegametime\r\n");
                break;
            case ID_HWNDBUTTON4:
                try {
                    if (server.ls_close()) {
                        throw std::exception("Shutdown failed\n");
                    }
                    EnableWindow(hwndConnectLivesplit, true);
                    EnableWindow(hwndButton4, false);
                }
                catch (std::exception& e) {
                    std::cout << e.what() << std::endl;
                }
                break;
            case ID_HWNDBUTTON5:
                PopulateProcessList();
                SendMessage(hwndProcessList, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);
                break;
            case ID_START:
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
                        //DBOUT(TEXT("CreateThread"));
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
                break;
            case ID_PRINT:
                CaptureAnImage(selHwnd, (HWND)NULL, false, false, true);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }

        }
        break;
    case WM_PAINT:
        {
            //DBOUT("WM_PAINT Main\n");
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            //CaptureAnImage(hWnd);
            //DrawWizardCard(hdc, hWnd);
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
        PostQuitMessage(0);
        break;
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
    static BOOL fDrawRect;   // TRUE if bitmap rect. is dragged  
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
        //DBOUT("WM_PAINT Draw ");
        // Draw the bitmap rectangle and copy the bitmap into  
        // it. The 32-pixel by 32-pixel bitmap is centered  
        // in the rectangle by adding 1 to the left and top  
        // coordinates of the bitmap rectangle, and subtracting 2  
        // from the right and bottom coordinates. 
        
            CaptureAnImage(hwndDrawableCanvas, selHwnd, false, false, false);
            CaptureAnImage(hwndPreviewCanvas, selHwnd, true, false, false);
        hdc = BeginPaint(hWnd, &ps);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        SelectObject(hdc, (HGDIOBJ)CreatePen(PS_SOLID, 5, RGB(0, 255, 0)));
        //DBOUT("Draw Rect\n");
        Rectangle(hdc, rcTarget.left, rcTarget.top,
            rcTarget.right, rcTarget.bottom);
        //FrameRect(hdc, &rcTarget, CreateSolidBrush(RGB(0,0,255)));
        EndPaint(hWnd, &ps);
        ReleaseDC(hWnd, hdc);
        break;
    }
    case WM_MOVE:
    case WM_SIZE:
        break;
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
            
            // Draw a target rectangle or drag the bitmap rectangle,  
            // depending on the status of the fDragRect flag.  

            if ((wParam && MK_LBUTTON) && fDrawRect)
            {
                //DBOUT("WM_MOUSEMOVE\n");
                // Set the mix mode so that the pen color is the  
                // inverse of the background color. The previous  
                // rectangle can then be erased by drawing  
                // another rectangle on top of it.  

                hdc = GetDC(hWnd);
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

                if ((pt.x < (LONG)LOWORD(lParam)) &&
                    (pt.y > (LONG) HIWORD(lParam)))
                {
                    SetRect(&rcTarget, pt.x, HIWORD(lParam),
                        LOWORD(lParam), pt.y);
                }
                else if ((pt.x > (LONG)LOWORD(lParam)) &&
                    (pt.y > (LONG)HIWORD(lParam)))
                {
                    SetRect(&rcTarget, LOWORD(lParam),
                        HIWORD(lParam), pt.x, pt.y);
                }
                else if ((pt.x > (LONG)LOWORD(lParam)) &&
                    (pt.y < (LONG)HIWORD(lParam)))
                {
                    SetRect(&rcTarget, LOWORD(lParam), pt.y,
                        pt.x, HIWORD(lParam));
                }
                else
                {
                    SetRect(&rcTarget, pt.x, pt.y, LOWORD(lParam),
                        HIWORD(lParam));
                }
                ReleaseDC(hWnd, hdc);

            }
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONUP:
        fDrawRect = false;
        CaptureAnImage(hwndPreviewCanvas, selHwnd, true, false, false);
        break;
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
    if (!EnumDisplayMonitors(mHDC, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors))){
        ReleaseDC(NULL, mHDC);
        return;
    }
    if (!EnumWindows(enumWindowCallback, 0))
    {
        ReleaseDC(NULL, mHDC);
        return;
    }
    ReleaseDC(NULL, mHDC);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData) {
    if (typeid(&pData).name() == "int") {
        CaptureAnImage(hwndDrawableCanvas, hdc, *lprcMonitor, false, false, false);
    }
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

unsigned char* CaptureAnImage(HWND hWnd, HWND hWndSource, bool isDrawing, bool isLoadRemoverRunning, bool isPrinting)
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
    HDC hdcMemDC2 = NULL;
    HBITMAP newBitmap = NULL;
    unsigned char* bits = NULL;
    int error = 0;
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
            if (isDrawing) {
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
        error = 1;
        goto done;
    }
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

    BITMAPINFO   bi;
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
    newBitmap = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
    hdcMemDC2 = CreateCompatibleDC(NULL);
    SelectObject(hdcMemDC2, newBitmap);
    BitBlt(hdcMemDC2, 0, 0, convertedWidth, convertedHeight, hdcWindow, convertedX, convertedY, SRCCOPY);

    if (isPrinting) {
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

        WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

        // Unlock and Free the DIB from the heap.
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);

        // Close the handle for the file that was created.
        CloseHandle(hFile);
    }

    DeleteObject(hdcMemDC2);
done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(hWndSource, hdcScreen);
    ReleaseDC(hWnd, hdcWindow);
    if (error) {
        return NULL;
    }
    else {
        return bits;
    }
}

unsigned char* CaptureAnImage(HWND hWnd, HDC hdcScreen, RECT rcSource, bool isDrawing, bool isLoadRemoverRunning, bool isPrinting)
{
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
    HDC hdcMemDC2 = NULL;
    HBITMAP newBitmap = NULL;
    unsigned char* bits = NULL;
    int error = 0;
    // Retrieve the handle to a display device context for the client 
    // area of the window. 
    //hdcScreen = GetDC(hWndSource);
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
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        //GetClientRect(hMon, &rcSource);

        // This is the best stretch mode.
        SetStretchBltMode(hdcWindow, HALFTONE);

        if (isDrawing || isLoadRemoverRunning) {
            if (isDrawing) {
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
            width = rcSource.right;
            height = rcSource.bottom;
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
            MessageBox(hWnd, L"StretchBlt has failed", L"Failed", MB_OK);
            goto done;
        }
        error = 1;
        goto done;
    }
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

    BITMAPINFO   bi;
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
    newBitmap = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
    hdcMemDC2 = CreateCompatibleDC(NULL);
    SelectObject(hdcMemDC2, newBitmap);
    BitBlt(hdcMemDC2, 0, 0, convertedWidth, convertedHeight, hdcWindow, convertedX, convertedY, SRCCOPY);

    if (isPrinting) {
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

        WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

        // Unlock and Free the DIB from the heap.
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);

        // Close the handle for the file that was created.
        CloseHandle(hFile);
    }

    DeleteObject(hdcMemDC2);
done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    //DeleteDC(hdcScreen);
    ReleaseDC(hWnd, hdcWindow);
    if (error) {
        return NULL;
    }
    else {
        return bits;
    }
}

//TODO
void InitializeTesseract() {

    if (tess.Init("./tessdata", "eng"))
    {
        DBOUT("OCRTesseract: Could not initialize tesseract." << std::endl);
    }
    else {
        DBOUT("OCRTesseract: Initialized tesseract." << std::endl);
    }

    // setup
    tess.SetPageSegMode(tesseract::PageSegMode::PSM_AUTO);
}

DWORD WINAPI LoadRemover(LPVOID lpParam)
{
    char* out_str = NULL;
    unsigned char* bits = NULL;
    while (isThreadWorking) {
        if ((bits = CaptureAnImage(selHwnd, (HWND)NULL, false, true, false)) == NULL) {
            break;
        }
        tess.SetImage(bits, convertedWidth, convertedHeight, BYTES_PER_PIXEL, convertedWidth * BYTES_PER_PIXEL);
        out_str = tess.GetUTF8Text();
        DBOUT(bits << " " << out_str << std::endl);
        if (strstr(out_str, "Loading") && !server.get_isLoading()) {
            server.set_isLoading(true);
            server.send_to_ls("pausegametime\r\n");
        }
        if (strstr(out_str, "Loading")==NULL && server.get_isLoading()) {
            server.set_isLoading(false);
            server.send_to_ls("unpausegametime\r\n");
        }
        memset(bits, 0, sizeof(bits));
    }
    tess.Clear();
    return 0;
}

/*int CaptureAnImage(HWND hWnd, HWND hWndSource)
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

    // Retrieve the handle to a display device context for the client 
    // area of the window. 
    hdcScreen = GetDC(hWndSource);
    hdcWindow = GetDC(hWnd);

    // Create a compatible DC, which is used in a BitBlt from the window DC.
    hdcMemDC = CreateCompatibleDC(hdcWindow);


    if (!hdcMemDC)
    {
        MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
        goto done;
    }
    //PrintWindow(hWndSource, hdcWindow, PW_CLIENTONLY);

    // Get the client area for size calculation.
    RECT rcClient, rcSource;
    GetClientRect(hWnd, &rcClient);
    GetClientRect(hWndSource, &rcSource);

    // This is the best stretch mode.
    SetStretchBltMode(hdcWindow, HALFTONE);

    // The source DC is the entire screen, and the destination DC is the current window (HWND).
    if (!StretchBlt(hdcWindow,
        0, 0,
        rcClient.right, rcClient.bottom,
        hdcScreen,
        0, 0,
        rcSource.right,
        rcSource.bottom,
        SRCCOPY))
    {
        MessageBox(hWnd, L"StretchBlt has failed", L"Failed", MB_OK);
        goto done;
    }

done:
    DeleteObject(hbmScreen);
    ReleaseDC(hWndSource, hdcScreen);
    ReleaseDC(hWnd, hdcWindow);

    return 0;
}

int DrawPreview(HWND hWnd, HWND hWndSource) {
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
    convertedX = (rcTarget.left * (selRect.right - selRect.left) / CANVAS_WIDTH);
    convertedY = (rcTarget.top * (selRect.bottom - selRect.top) / CANVAS_HEIGHT);
    convertedWidth = ((selRect.right - selRect.left) * (rcTarget.right - rcTarget.left)) / CANVAS_WIDTH;
    convertedHeight = ((selRect.bottom - selRect.top) * (rcTarget.bottom - rcTarget.top)) / CANVAS_HEIGHT;
    // Retrieve the handle to a display device context for the client 
    // area of the window. 
    hdcScreen = GetDC(hWndSource);
    hdcWindow = GetDC(hWnd);

    //PrintWindow(hWndSource, hdcWindow, PW_CLIENTONLY);

    // Get the client area for size calculation.
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    // This is the best stretch mode.
    SetStretchBltMode(hdcWindow, HALFTONE);

    // The source DC is the entire screen, and the destination DC is the current window (HWND).
    if (!StretchBlt(hdcWindow,
        0, 0,
        rcClient.right, rcClient.bottom,
        hdcScreen,
        convertedX, convertedY,
        convertedWidth,
        convertedHeight,
        SRCCOPY))
    {
        MessageBox(hWnd, L"StretchBlt has failed", L"Failed", MB_OK);
        goto done;
    }

done:
    DeleteObject(hbmScreen);
    ReleaseDC(hWndSource, hdcScreen);
    ReleaseDC(hWnd, hdcWindow);

    return 0;
}

unsigned char* CapturePreview(int& width, int& height, bool isPrinting) {
    HDC hdcSource;
    HDC hdcMemDC = NULL;
    HDC hdcMemDC2 = NULL;
    HBITMAP hbmScreen = NULL;
    HBITMAP newBitmap = NULL;
    BITMAP bmpScreen;
    DWORD dwBmpSize = 0;
    unsigned char* bits = NULL;
    hdcSource = GetDC(selHwnd);
    hdcMemDC = CreateCompatibleDC(hdcSource);
    int error = 0;
    if (!hdcMemDC)
    {
        MessageBox(selHwnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
        error = 1;
        goto done;
    }
    // Create a compatible bitmap from the Window DC.
    hbmScreen = CreateCompatibleBitmap(hdcSource, convertedWidth, convertedHeight);
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
        hdcSource,
        convertedX, convertedY,
        SRCCOPY))
    {
        MessageBox(selHwnd, L"BitBlt has failed", L"Failed", MB_OK);
        error = 1;
        goto done;
    }
    // Get the BITMAP from the HBITMAP.
    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);
    width = bmpScreen.bmWidth;
    height = bmpScreen.bmHeight;

    BITMAPINFO   bi;
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
    newBitmap = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
    hdcMemDC2 = CreateCompatibleDC(NULL);
    SelectObject(hdcMemDC2, newBitmap);
    BitBlt(hdcMemDC2, 0, 0, convertedWidth, convertedHeight, hdcSource, convertedX, convertedY, SRCCOPY);

    if (isPrinting) {
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
        GetDIBits(hdcSource, hbmScreen, 0,
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

        WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

        // Unlock and Free the DIB from the heap.
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);

        // Close the handle for the file that was created.
        CloseHandle(hFile);
    }

done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC2);
    DeleteObject(hdcMemDC);
    ReleaseDC(selHwnd, hdcSource);
    if (error) {
        return NULL;
    }
    else {
        return bits;
    }

}*/