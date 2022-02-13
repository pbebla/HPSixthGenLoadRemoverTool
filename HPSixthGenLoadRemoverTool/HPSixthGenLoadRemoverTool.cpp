// HPSixthGenLoadRemoverTool.cpp : Defines the entry point for the application.
//


#include "framework.h"
#include "HPSixthGenLoadRemoverTool.h"
#include "LiveSplitServer.h"
#pragma comment(lib, "ComCtl32.Lib")
#pragma comment (lib, "Psapi.lib")
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
// Global Variables:
cv::Mat cvMat, refMat, resized, grayResized, diff;
HMENU hMenu;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
BOOL fDrawRect;   // TRUE if bitmap rect. is dragged  
HWND selHwnd, hwndButton5, hwndStart, hwndStop, hwndPrint, hwndProcessList, hwndDrawableCanvas, hwndPreviewCanvas, hwndPSNR, hwndThreshold, hwndToggle;
HWND savedX, savedY, savedW, savedH;
LiveSplitServer server;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;
RECT selRect, rcTarget;    // rectangle to receive bitmap 
POINT pt;         // x and y coordinates of cursor 
int convertedX, convertedY, convertedWidth, convertedHeight, intSavedX, intSavedY, intSavedW, intSavedH;
float threshold;
INITCOMMONCONTROLSEX icex;    // Structure for control initialization.
bool isInitialLoad, showPreview;
const UINT valMin = 0;          // The range of values for the Up-Down control.
const UINT valMax = GetSystemMetrics(SM_CXVIRTUALSCREEN);
Gdiplus::Bitmap* refBmp;
UINT cyVScroll;              // Height of scroll bar arrow.
HRESULT hr;
typedef struct tagMonData
{
    int current;
    MONITORINFOEXW* info;
} MonData;

typedef struct SaveData {
    float threshold;
    int rectLeft, rectTop, rectRight, rectBottom;
    PWSTR pszFilePath;
} SaveData;

struct BufferPSNR                                     // Optimized CUDA versions
{   // Data allocations are very expensive on CUDA. Use a buffer to solve: allocate once reuse later.
    cv::cuda::GpuMat gI1, gI2, gs, t1, t2;
    cv::cuda::GpuMat buf;
};
struct BufferMSSIM                                     // Optimized CUDA versions
{   // Data allocations are very expensive on CUDA. Use a buffer to solve: allocate once reuse later.
    cv::cuda::GpuMat gI1, gI2, gs, t1, t2;
    cv::cuda::GpuMat I1_2, I2_2, I1_I2;
    std::vector<cv::cuda::GpuMat> vI1, vI2;
    cv::cuda::GpuMat mu1, mu2;
    cv::cuda::GpuMat mu1_2, mu2_2, mu1_mu2;
    cv::cuda::GpuMat sigma1_2, sigma2_2, sigma12;
    cv::cuda::GpuMat t3;
    cv::cuda::GpuMat ssim_map;
    cv::cuda::GpuMat buf;
};
bool hasCuda;
PWSTR filePath, saveFilePath;
WCHAR* selDisplay;
int selDisplayIndex;
HWND hControl, hwndUpDnEdtBdyX, hwndUpDnCtlX, hwndUpDnEdtBdyY, hwndUpDnCtlY, hwndUpDnEdtBdyW, hwndUpDnCtlW, hwndUpDnEdtBdyH, hwndUpDnCtlH; // Handles to the controls.
HANDLE loadRemover;
DWORD loadRemoverId;
MonData monitors;
static bool isThreadWorking = false;
// Handles to the create controls functions.
HWND CreateUpDnBuddy(HWND, int, int);
HWND CreateUpDnCtl(HWND, int, int, const wchar_t*);
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
bool CALLBACK SetFont(HWND child, LPARAM font);
void PopulateProcessList();
BOOL CALLBACK enumWindowCallback(HWND, LPARAM);
void CaptureAnImage(HWND, HWND, bool);
DWORD WINAPI LoadRemover(LPVOID);
BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
void AddMenus(HWND);
void SaveFile();
void SaveFileAs(HWND);
float GetPSNR(HWND);
void LoadFile(HWND, bool);
void mainDraw();
void Resize(int left, int top, int right, int bottom);
void GrabMat(HWND hwnd);
void RenderRefImage(HWND);
void CalculateDim();
void DisplayDiff();
std::wstring GetMatType(const cv::Mat& mat);
std::wstring GetMatDepth(const cv::Mat& mat);
double getPSNR_CUDA_optimized(const cv::Mat&, const cv::Mat&, BufferPSNR&);
float getMSSIM_CUDA_optimized(const cv::Mat& i1, const cv::Mat& i2, BufferMSSIM& b);
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HPSIXTHGENLOADREMOVERTOOL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    MyRegisterDrawCanvasClass(hInstance);
    MyRegisterPreviewCanvasClass(hInstance);
    server = LiveSplitServer();
    isInitialLoad = true;
    showPreview = true;
    cv::cuda::DeviceInfo dInfo;
    hasCuda = (cv::cuda::getCudaEnabledDeviceCount() > 0 && dInfo.isCompatible());
    DBOUT("can use cuda?: " << hasCuda << std::endl);
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HPSIXTHGENLOADREMOVERTOOL));
    MSG msg;
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

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
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

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, 990, 540, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   EnumChildWindows(hWnd, (WNDENUMPROC)SetFont, (LPARAM)GetStockObject(DEFAULT_GUI_FONT));
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

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
                70,
                20,
                hWnd,
                (HMENU)IDC_PSNR,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);
            hwndThreshold = CreateWindowW(
                L"EDIT",
                TEXT(""),
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                900,
                175,
                70,
                20,
                hWnd,
                (HMENU)IDC_PSNR,
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
                L"Show Difference Image",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                75,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_PRINT,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            hwndToggle = CreateWindowW(
                L"BUTTON",  // Predefined class; Unicode assumed 
                L"Disable Preview",      // Button text 
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
                0,         // x position 
                100,         // y position 
                150,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                (HMENU)ID_TOGGLE,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            savedX = CreateWindowW(
                L"STATIC",  // Predefined class; Unicode assumed 
                L"",      // Button text 
                WS_VISIBLE | WS_CHILD,  // Styles 
                0,         // x position 
                125,         // y position 
                100,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                NULL,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            savedY = CreateWindowW(
                L"STATIC",  // Predefined class; Unicode assumed 
                L"",      // Button text 
                WS_VISIBLE | WS_CHILD,  // Styles 
                105,         // x position 
                125,         // y position 
                100,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                NULL,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            savedW = CreateWindowW(
                L"STATIC",  // Predefined class; Unicode assumed 
                L"",      // Button text 
                WS_VISIBLE | WS_CHILD,  // Styles 
                210,         // x position 
                125,         // y position 
                100,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                NULL,       // No menu.
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                NULL);      // Pointer not needed.
            savedH = CreateWindowW(
                L"STATIC",  // Predefined class; Unicode assumed 
                L"",      // Button text 
                WS_VISIBLE | WS_CHILD,  // Styles 
                315,         // x position 
                125,         // y position 
                100,        // Button width
                20,        // Button height
                hWnd,     // Parent window
                NULL,       // No menu.
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
            if (hWmID == CBN_SELCHANGE) {
                int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL,
                    (WPARAM)0, (LPARAM)0);
                selDisplayIndex = ItemIndex;
                LPWSTR ListItem[256];
                (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,
                    (WPARAM)ItemIndex, (LPARAM)ListItem);
                //DBOUT("ListItem " << ListItem << "\n");
                if (wcsstr((wchar_t*)ListItem, L"\\\\.\\DISPLAY")) {
                    selDisplay = monitors.info[ItemIndex].szDevice;
                    CaptureAnImage(hwndDrawableCanvas, NULL, false);
                    selHwnd = NULL;
                }
                else {
                    selHwnd = FindWindowW(NULL, (LPCWSTR)ListItem);
                }
                if (selHwnd == NULL) {
                    selRect = RECT{ 0, 0, monitors.info[ItemIndex].rcMonitor.right - monitors.info[ItemIndex].rcMonitor.left, monitors.info[ItemIndex].rcMonitor.bottom - monitors.info[ItemIndex].rcMonitor.top };
                }
                else {
                    GetClientRect(selHwnd, &selRect);
                }
                isInitialLoad = false;
                SendMessage(hwndDrawableCanvas, WM_PAINT, 0, 0);
                if (!IsRectEmpty(&rcTarget)) {
                    CalculateDim();
                    if (intSavedX != NULL) {
                        int range = 5;
                        if (std::abs(intSavedX - convertedX) <= range && std::abs(intSavedY - convertedY) <= range && std::abs(intSavedW - convertedWidth) <= range && std::abs(intSavedH - convertedHeight) <= range) {
                            convertedX = intSavedX;
                            convertedY = intSavedY;
                            convertedWidth = intSavedW;
                            convertedHeight = intSavedH;
                            SendMessage(hwndUpDnCtlX, UDM_SETPOS, 0, convertedX);
                            SendMessage(hwndUpDnCtlY, UDM_SETPOS, 0, convertedY);
                            SendMessage(hwndUpDnCtlW, UDM_SETPOS, 0, convertedWidth);
                            SendMessage(hwndUpDnCtlH, UDM_SETPOS, 0, convertedHeight);
                        }
                    }
                }
                mainDraw();
            }
            switch (wmId)
            {
            case ID_FILE_OPEN:
                LoadFile(hWnd, false);
                break;
            case ID_IMAGE_LOAD:
                LoadFile(hWnd, true);
                break;
            case ID_FILE_SAVE:
                SaveFile();
                break;
            case ID_FILE_SAVE_AS:
                SaveFileAs(hWnd);
                break;
            case ID_TOGGLE:
                showPreview = !showPreview;
                EnableWindow(hwndPreviewCanvas, showPreview);
                if (showPreview) {
                    SetWindowText(hwndToggle, L"Disable Preview");
                    mainDraw();
                }
                else {
                    SetWindowText(hwndToggle, L"Enable Preview");
                    InvalidateRect(hwndPreviewCanvas, NULL, true);
                }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
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
                    TCHAR buff[MAX_LOADSTRING];
                    GetWindowText(hwndThreshold, buff, MAX_LOADSTRING);
                    threshold = _tstof(buff);
                    
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
                    EnableWindow(hwndThreshold, false);
                }
                break;
            case ID_STOP:
                isThreadWorking = false;
                WaitForSingleObject(loadRemover, 0);
                CloseHandle(loadRemover);
                EnableWindow(hwndStop, false);
                EnableWindow(hwndStart, true);
                EnableWindow(hwndThreshold, true);
                server.close_pipe();
                break;
            case ID_PRINT:
                if (!cvMat.empty() && !diff.empty()) {
                    //cv::imwrite("test.png", diff);
                    cv::imshow("Diff", diff);
                }
                break;
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Gdiplus::Graphics gf(hdc);
            gf.DrawImage(refBmp, 605, 25, 145*CANVAS_WIDTH/CANVAS_HEIGHT, 145);
            EndPaint(hWnd, &ps);
            break;
        }
    case WM_SIZE:
        mainDraw();
        break;
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
    case WM_NOTIFY:
    {
        UINT nCode = ((LPNMHDR)lParam)->code;
        switch (nCode) {
        case UDN_DELTAPOS:
            LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
            if (!IsRectEmpty(&rcTarget)) {
                int l = rcTarget.left;
                int t = rcTarget.top;
                int r = rcTarget.right;
                int b = rcTarget.bottom;
                int newPos = lpnmud->iPos + lpnmud->iDelta;
                InvalidateRect(hWnd, NULL, false);
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
                Resize(l, t, r, b);
            }
            mainDraw();
            break;
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool CALLBACK SetFont(HWND child, LPARAM font) {
    SendMessage(child, WM_SETFONT, font, true);
    return true;
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
        hdc = GetDC(hWnd);
        hdcCompat = CreateCompatibleDC(hdc);
        crBkgnd = GetBkColor(hdc);
        hbrBkgnd = CreateSolidBrush(COLORREF(NULL_BRUSH));
        ReleaseDC(hWnd, hdc);
        break;
    }
    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        ReleaseDC(hWnd, hdc);
        break;
    }
    case WM_LBUTTONDOWN:
        pt.x = (LONG)LOWORD(lParam);
        pt.y = (LONG)HIWORD(lParam);
        fDrawRect = true;
        break;
    case WM_MOUSEMOVE:
        {
            if ((wParam && MK_LBUTTON) && fDrawRect)
            {
                if (!IsRectEmpty(&rcTarget))
                {
                    InvalidateRect(hWnd, NULL, true);
                }
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
                Resize(left, top, right, bottom);
                CalculateDim();
                mainDraw();
            }
        }
        break;
    case WM_LBUTTONUP:
        fDrawRect = false;
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        free(monitors.info);
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
    case WM_ERASEBKGND: return 1;
    case WM_PAINT:
        //BeginPaint(hwnd, &ps);

        ValidateRect(hwnd, NULL);
        //EndPaint(hwnd, &ps);
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

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
    int cMonitors = GetSystemMetrics(SM_CMONITORS);
    monitors = {};
    monitors.current = 0;
    monitors.info = (MONITORINFOEXW*)calloc(cMonitors, sizeof(MONITORINFOEXW));
    unsigned int i;
    HDC mHDC = GetDC(NULL);
    if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc,(LPARAM)(&monitors))) {
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
    MonData* data = (MonData*)pData;
    data->info[data->current].cbSize = sizeof(MONITORINFOEXW);
    if (GetMonitorInfoW(hMon, &(data->info[data->current++]))) {
        SendMessageW(hwndProcessList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)data->info[data->current-1].szDevice);
        return TRUE;
    }
    else {
        return FALSE;
    }
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

void CaptureAnImage(HWND hWnd, HWND hWndSource, bool isDrawing)
{
    HDC hdcScreen;
    HDC hdcWindow;
    HDC hdcMemDC = NULL;
    HBITMAP hbmScreen = NULL;
    BITMAP bmpScreen;
    int x, y, width, height;
    int error = 0;
    BITMAPINFO   bi;
    if (hWndSource == NULL) {
        hdcScreen = CreateDC(selDisplay, selDisplay, NULL, NULL);
    }
    else {
        hdcScreen = GetDC(hWndSource);
    }
    hdcWindow = GetDC(hWnd);
    hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC)
    {
        MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
        goto done;
    }
    RECT rcClient, rcSource;
    GetClientRect(hWnd, &rcClient);
    if (hWndSource != NULL) {
        GetClientRect(hWndSource, &rcSource);
    }
    SetStretchBltMode(hdcWindow, HALFTONE);
    if (isDrawing) {
        x = convertedX;
        y = convertedY;
        width = convertedWidth;
        height = convertedHeight;
    }
    else {
        x = 0;
        y = 0;
        if (hWndSource == NULL) {
            width = monitors.info[selDisplayIndex].rcMonitor.right - monitors.info[selDisplayIndex].rcMonitor.left;
            height = monitors.info[selDisplayIndex].rcMonitor.bottom - monitors.info[selDisplayIndex].rcMonitor.top;
        }
        else {
            width = rcSource.right;
            height = rcSource.bottom;
        }
    }
    if (!isDrawing || (isDrawing && showPreview)) {
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
    }
    
    if (!isDrawing) {
        if (!refMat.empty() && convertedWidth > 0 && convertedHeight > 0){
            cv::resize(refMat, resized, cv::Size(convertedWidth, convertedHeight), cv::INTER_LINEAR);
            cv::cvtColor(resized, grayResized, cv::COLOR_BGRA2GRAY);
            //DBOUT("gresized Mat " << GetMatType(grayResized) << " " << GetMatDepth(grayResized) << std::endl);
        }
        FrameRect(hdcWindow, &rcTarget, CreateSolidBrush(RGB(0,255,0)));
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
    using std::chrono::operator""ms;
    float ret;
    int frames = 0;
    while (isThreadWorking) {
        auto start = std::chrono::steady_clock::now();
        auto duration = start + 3ms;
        if (showPreview) {
            CaptureAnImage(hwndPreviewCanvas, selHwnd, true);
        }
        if (!IsRectEmpty(&rcTarget) && convertedWidth > 0 && convertedHeight > 0) {
            GrabMat(selHwnd);
            ret = GetPSNR(selHwnd);
        }
        else {
            ret = -1.0;
        }
        if (ret >= threshold && !server.get_isLoading()) {
            server.send_to_ls("pausegametime\r\n");
            server.set_isLoading(true);
        }
        else if (ret < threshold && server.get_isLoading()) {
            server.send_to_ls("unpausegametime\r\n");
            server.set_isLoading(false);
        }
        std::this_thread::sleep_until(duration);
        std::chrono::duration<double, std::milli> Elapsed = std::chrono::steady_clock::now() - start;
    }
    return 0;
}

void AddMenus(HWND hWnd) {
    hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hAboutMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, ID_IMAGE_LOAD, L"Load Image");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, L"Open");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE, L"Save");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE_AS, L"Save As");
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, L"Exit");
    AppendMenu(hAboutMenu, MF_STRING, IDM_ABOUT, L"About");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hAboutMenu, L"About");
    SetMenu(hWnd, hMenu);
    cyVScroll = GetSystemMetrics(SM_CYVSCROLL);   
    hwndUpDnEdtBdyX = CreateUpDnBuddy(hWnd, 0, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlX = CreateUpDnCtl(hWnd, 0, 175, L"X");    // Create an Up-Down control.
    hwndUpDnEdtBdyY = CreateUpDnBuddy(hWnd, 105, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlY = CreateUpDnCtl(hWnd, 105, 175, L"Y");    // Create an Up-Down control.
    hwndUpDnEdtBdyW = CreateUpDnBuddy(hWnd, 210, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlW = CreateUpDnCtl(hWnd, 210, 175, L"Width");    // Create an Up-Down control.
    hwndUpDnEdtBdyH = CreateUpDnBuddy(hWnd, 315, 175);  // Create an buddy window (an Edit control).
    hwndUpDnCtlH = CreateUpDnCtl(hWnd, 315, 175, L"Height");    // Create an Up-Down control.
}

void LoadFile(HWND hwnd, bool isRefImg) {
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (SUCCEEDED(hr))
        {
            if (isRefImg) {
                COMDLG_FILTERSPEC rgSpec[] =
                {
                    {L"All supported images.", L"*.jpg;*.jpeg;*.png;*.bmp;*.gif"},
                };
                pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
            }
            else {
                COMDLG_FILTERSPEC rgSpec[] =
                {
                    {L".dat", L"*.dat"},
                };
                pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
            }
            hr = pFileOpen->Show(NULL);
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    if (isRefImg) {
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
                        if (SUCCEEDED(hr))
                        {
                            RenderRefImage(hwnd);
                        }
                    } else {
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &saveFilePath);
                        if (SUCCEEDED(hr))
                        {
                            std::wstring wFilePath;
                            std::wifstream ss(saveFilePath, std::ifstream::in);
                            ss >> threshold;
                            ss >> rcTarget.left;
                            ss >> rcTarget.top;
                            ss >> rcTarget.right;
                            ss >> rcTarget.bottom;
                            ss >> wFilePath;
                            ss >> convertedX;
                            ss >> convertedY;
                            ss >> convertedWidth;
                            ss >> convertedHeight;
                            filePath = (PWSTR) wFilePath.c_str();
                            ss.close();
                            Resize(rcTarget.left, rcTarget.top, rcTarget.right, rcTarget.bottom);
                            RenderRefImage(hwnd);
                            WCHAR buf[40];
                            swprintf(buf, sizeof(buf), L"%f", threshold);
                            SetWindowText(hwndThreshold, std::to_wstring(threshold).c_str());
                            SetWindowText(savedX, std::to_wstring(convertedX).c_str());
                            SetWindowText(savedY, std::to_wstring(convertedY).c_str());
                            SetWindowText(savedW, std::to_wstring(convertedWidth).c_str());
                            SetWindowText(savedH, std::to_wstring(convertedHeight).c_str());
                            intSavedX = convertedX;
                            intSavedY = convertedY;
                            intSavedW = convertedWidth;
                            intSavedH = convertedHeight;
                            mainDraw();
                        }
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
}

void SaveFileAs(HWND hwnd) {
    TCHAR buff[MAX_LOADSTRING];
    GetWindowText(hwndThreshold, buff, MAX_LOADSTRING);
    threshold = _tstof(buff);
    if (IsRectEmpty(&rcTarget) || threshold == NULL || threshold == 0.0 || refMat.empty()) {
        MessageBox(hwnd, L"Need green box, threshold value, and a reference image.", TEXT("Error..."), MB_OK | MB_ICONERROR);
        return;
    }
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileSaveDialog* pFileSaveAs;
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSaveAs));
        if (SUCCEEDED(hr)) {
            COMDLG_FILTERSPEC rgSpec[] =
            {
                {L".dat", L"*.dat;"},
            };
            hr = pFileSaveAs->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
            hr = pFileSaveAs->SetDefaultExtension(L".dat");
            hr = pFileSaveAs->Show(NULL);
            if (SUCCEEDED(hr)) {
                IShellItem* pItem;
                hr = pFileSaveAs->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &saveFilePath);
                    if (SUCCEEDED(hr)) {
                        std::wofstream outData(saveFilePath);
                        if (outData.is_open()) {
                            TCHAR buff[MAX_LOADSTRING];
                            GetWindowText(hwndThreshold, buff, MAX_LOADSTRING);
                            threshold = _tstof(buff);
                            outData << threshold << std::endl;
                            outData << rcTarget.left << std::endl;
                            outData << rcTarget.top << std::endl;
                            outData << rcTarget.right << std::endl;
                            outData << rcTarget.bottom << std::endl;
                            outData << filePath << std::endl;
                            outData << convertedX << std::endl;
                            outData << convertedY << std::endl;
                            outData << convertedWidth << std::endl;
                            outData << convertedHeight << std::endl;
                            outData.close();
                        }
                        SetWindowText(hwndThreshold, std::to_wstring(threshold).c_str());
                        SetWindowText(savedX, std::to_wstring(convertedX).c_str());
                        SetWindowText(savedY, std::to_wstring(convertedY).c_str());
                        SetWindowText(savedW, std::to_wstring(convertedWidth).c_str());
                        SetWindowText(savedH, std::to_wstring(convertedHeight).c_str());
                    }
                    pItem->Release();
                }
            }
            pFileSaveAs->Release();
        }
        CoUninitialize();
    }
}

void SaveFile() {
    if (saveFilePath != NULL) {
        std::wofstream outData(saveFilePath);
        if (outData.is_open()) {
            TCHAR buff[MAX_LOADSTRING];
            GetWindowText(hwndThreshold, buff, MAX_LOADSTRING);
            threshold = _tstof(buff);
            outData << threshold << std::endl;
            outData << rcTarget.left << std::endl;
            outData << rcTarget.top << std::endl;
            outData << rcTarget.right << std::endl;
            outData << rcTarget.bottom << std::endl;
            outData << filePath << std::endl;
            outData << convertedX << std::endl;
            outData << convertedY << std::endl;
            outData << convertedWidth << std::endl;
            outData << convertedHeight << std::endl;
            outData.close();
        }
        SetWindowText(hwndThreshold, std::to_wstring(threshold).c_str());
        SetWindowText(savedX, std::to_wstring(convertedX).c_str());
        SetWindowText(savedY, std::to_wstring(convertedY).c_str());
        SetWindowText(savedW, std::to_wstring(convertedWidth).c_str());
        SetWindowText(savedH, std::to_wstring(convertedHeight).c_str());
    }
}

HWND CreateUpDnBuddy(HWND hwndParent, int x, int y)
{
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

HWND CreateUpDnCtl(HWND hwndParent, int x, int y, const wchar_t* label)
{
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
    CreateWindow(L"Static",
        label,
        WS_VISIBLE | WS_CHILD,
        x, y - 25,
        100, 20,
        hwndParent,
        NULL,
        hInst,
        NULL);
    return (hControl);
}

float GetPSNR(HWND hwnd) {
    float PSNR = 0.0;
    BufferPSNR b;
    BufferMSSIM b2;
    if (!refMat.empty() && convertedWidth > 0 && convertedHeight > 0) {
        cv::Mat srcGray;
        try {
            cv::cvtColor(cvMat, srcGray, cv::COLOR_BGRA2GRAY);
            //DBOUT("gresized Mat " << GetMatType(grayResized) << " " << GetMatDepth(grayResized) << std::endl);
            //DBOUT("srcGray Mat " << GetMatType(srcGray) << " " << GetMatDepth(srcGray) << std::endl);
            if (hasCuda) {
                //PSNR = getMSSIM_CUDA_optimized(srcGray, resized, b2);
                PSNR = getPSNR_CUDA_optimized(srcGray, grayResized, b);
            }
            else {
                PSNR = cv::PSNR(srcGray, grayResized);
            }
            int dec, sign;
            std::wostringstream ss;
            ss << PSNR;
            SetWindowText(hwndPSNR, (LPCWSTR)ss.str().c_str());
            RedrawWindow(hwndPSNR, NULL, NULL, RDW_ERASE);
        }
        catch (std::exception& e) {
            DBOUT(e.what() << "\n");
        }
    }
    return PSNR;
}

void Resize(int left, int top, int right, int bottom) {
    SetRect(&rcTarget, left, top, right, bottom);
    
}

void mainDraw() {
    if (!isInitialLoad) {
        CaptureAnImage(hwndDrawableCanvas, selHwnd, false);
        CaptureAnImage(hwndPreviewCanvas, selHwnd, true);
        if (!IsRectEmpty(&rcTarget) && convertedWidth > 0 && convertedHeight > 0) {
            GrabMat(selHwnd);
            GetPSNR(selHwnd);
            if (hasCuda) {
                //DisplayDiff();
            }
        }
    }
}

void GrabMat(HWND hwnd) {
    HDC hwindowDC, hwindowCompatibleDC;
    HBITMAP hbwindow;
    BITMAPINFOHEADER  bi;
    if (hwnd == NULL) {
        hwindowDC = CreateDC(selDisplay, selDisplay, NULL, NULL);
    }
    else {
        hwindowDC = GetDC(hwnd);
    }
    hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, HALFTONE);
    RECT windowsize;    // get the height and width of the screen
    GetClientRect(hwnd, &windowsize);
    cvMat.create(convertedHeight, convertedWidth, CV_8UC4);
    hbwindow = CreateCompatibleBitmap(hwindowDC, convertedWidth, convertedHeight);
    bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
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
    SelectObject(hwindowCompatibleDC, hbwindow);
    BitBlt(hwindowCompatibleDC, 0, 0, convertedWidth, convertedHeight, hwindowDC, convertedX, convertedY, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, convertedHeight, cvMat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow
    DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);
}

void RenderRefImage(HWND hwnd) {
    InvalidateRect(hwnd, NULL, true);
    refBmp = new Gdiplus::Bitmap(filePath);
    std::wstringstream ss;
    ss << filePath;
    std::string str;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> ccvt;
    str = ccvt.to_bytes(ss.str());
    refMat = cv::imread(str);
}

void CalculateDim() {
    convertedX = (rcTarget.left * (selRect.right - selRect.left) / CANVAS_WIDTH);
    convertedY = (rcTarget.top * (selRect.bottom - selRect.top) / CANVAS_HEIGHT);
    convertedWidth = ((selRect.right - selRect.left) * (rcTarget.right - rcTarget.left)) / CANVAS_WIDTH;
    convertedHeight = ((selRect.bottom - selRect.top) * (rcTarget.bottom - rcTarget.top)) / CANVAS_HEIGHT;
    SendMessage(hwndUpDnCtlX, UDM_SETPOS, 0, convertedX);
    SendMessage(hwndUpDnCtlY, UDM_SETPOS, 0, convertedY);
    SendMessage(hwndUpDnCtlW, UDM_SETPOS, 0, convertedWidth);
    SendMessage(hwndUpDnCtlH, UDM_SETPOS, 0, convertedHeight);
}

double getPSNR_CUDA_optimized(const cv::Mat& I1, const cv::Mat& I2, BufferPSNR& b)
{
    b.gI1.upload(I1);
    b.gI2.upload(I2);
    b.gI1.convertTo(b.t1, CV_32F);
    b.gI2.convertTo(b.t2, CV_32F);
    cv::cuda::absdiff(b.t1.reshape(1), b.t2.reshape(1), b.gs);
    cv::cuda::multiply(b.gs, b.gs, b.gs);
    double sse = cv::cuda::sum(b.gs, b.buf)[0];
    cv::cuda::absdiff(b.gI1, b.gI2, b.gI2);
    diff.create(convertedHeight, convertedWidth, CV_8UC1);
    b.gI2.download(diff);
    if (sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double mse = sse / (double)(I1.channels() * I1.total());
        double psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;
    }
}

void DisplayDiff() {
    BITMAPINFO  bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
    bi.bmiHeader.biWidth = diff.cols;
    bi.bmiHeader.biHeight = -diff.rows;  //this is the line that makes it draw upside down or not
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;
    HDC hdc = GetDC(hwndPreviewCanvas);
    UINT cColors = GetDIBColorTable(hdc, 0, 256, bi.bmiColors);
    for (UINT iColor = 0; iColor < cColors; iColor++) {
        BYTE b = (BYTE)((30 * bi.bmiColors[iColor].rgbRed +
            59 * bi.bmiColors[iColor].rgbGreen +
            11 * bi.bmiColors[iColor].rgbBlue) / 100);
        bi.bmiColors[iColor].rgbRed = b;
        bi.bmiColors[iColor].rgbGreen = b;
        bi.bmiColors[iColor].rgbBlue = b;
    }
    StretchDIBits(hdc, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 0, 0, diff.cols, diff.rows, diff.data, &bi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwndPreviewCanvas, hdc);
}

float getMSSIM_CUDA_optimized(const cv::Mat& i1, const cv::Mat& i2, BufferMSSIM& b)
{
    const float C1 = 6.5025f, C2 = 58.5225f;
    /***************************** INITS **********************************/
    b.gI1.upload(i1);
    b.gI2.upload(i2);
    cv::cuda::Stream stream;
    b.gI1.convertTo(b.t1, CV_32F, stream);
    b.gI2.convertTo(b.t2, CV_32F, stream);
    cv::cuda::split(b.t1, b.vI1, stream);
    cv::cuda::split(b.t2, b.vI2, stream);
    cv::Scalar mssim;
    cv::Ptr<cv::cuda::Filter> gauss = cv::cuda::createGaussianFilter(b.vI1[0].type(), -1, cv::Size(11, 11), 1.5);
    float ssim = 0.0;
    for (int i = 0; i < b.gI1.channels(); ++i)
    {
        cv::cuda::multiply(b.vI2[i], b.vI2[i], b.I2_2, 1, -1, stream);        // I2^2
        cv::cuda::multiply(b.vI1[i], b.vI1[i], b.I1_2, 1, -1, stream);        // I1^2
        cv::cuda::multiply(b.vI1[i], b.vI2[i], b.I1_I2, 1, -1, stream);       // I1 * I2
        gauss->apply(b.vI1[i], b.mu1, stream);
        gauss->apply(b.vI2[i], b.mu2, stream);
        cv::cuda::multiply(b.mu1, b.mu1, b.mu1_2, 1, -1, stream);
        cv::cuda::multiply(b.mu2, b.mu2, b.mu2_2, 1, -1, stream);
        cv::cuda::multiply(b.mu1, b.mu2, b.mu1_mu2, 1, -1, stream);
        gauss->apply(b.I1_2, b.sigma1_2, stream);
        cv::cuda::subtract(b.sigma1_2, b.mu1_2, b.sigma1_2, cv::cuda::GpuMat(), -1, stream);
        //b.sigma1_2 -= b.mu1_2;  - This would result in an extra data transfer operation
        gauss->apply(b.I2_2, b.sigma2_2, stream);
        cv::cuda::subtract(b.sigma2_2, b.mu2_2, b.sigma2_2, cv::cuda::GpuMat(), -1, stream);
        //b.sigma2_2 -= b.mu2_2;
        gauss->apply(b.I1_I2, b.sigma12, stream);
        cv::cuda::subtract(b.sigma12, b.mu1_mu2, b.sigma12, cv::cuda::GpuMat(), -1, stream);
        //b.sigma12 -= b.mu1_mu2;
        //here too it would be an extra data transfer due to call of operator*(Scalar, Mat)
        cv::cuda::multiply(b.mu1_mu2, 2, b.t1, 1, -1, stream); //b.t1 = 2 * b.mu1_mu2 + C1;
        cv::cuda::add(b.t1, C1, b.t1, cv::cuda::GpuMat(), -1, stream);
        cv::cuda::multiply(b.sigma12, 2, b.t2, 1, -1, stream); //b.t2 = 2 * b.sigma12 + C2;
        cv::cuda::add(b.t2, C2, b.t2, cv::cuda::GpuMat(), -12, stream);
        cv::cuda::multiply(b.t1, b.t2, b.t3, 1, -1, stream);     // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))
        cv::cuda::add(b.mu1_2, b.mu2_2, b.t1, cv::cuda::GpuMat(), -1, stream);
        cv::cuda::add(b.t1, C1, b.t1, cv::cuda::GpuMat(), -1, stream);
        cv::cuda::add(b.sigma1_2, b.sigma2_2, b.t2, cv::cuda::GpuMat(), -1, stream);
        cv::cuda::add(b.t2, C2, b.t2, cv::cuda::GpuMat(), -1, stream);
        cv::cuda::multiply(b.t1, b.t2, b.t1, 1, -1, stream);     // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))
        cv::cuda::divide(b.t3, b.t1, b.ssim_map, 1, -1, stream);      // ssim_map =  t3./t1;
        stream.waitForCompletion();
        cv::Scalar s = cv::cuda::sum(b.ssim_map, b.buf);
        mssim.val[i] = s.val[0] / (b.ssim_map.rows * b.ssim_map.cols);
        ssim += mssim.val[i];
    }
    cv::cuda::absdiff(b.gI1, b.gI2, b.gI1);
    diff.create(convertedHeight, convertedWidth, CV_8UC4);
    b.gI1.download(diff);
    return (ssim / b.gI1.channels());
}

std::wstring GetMatDepth(const cv::Mat& mat)
{
    const int depth = mat.depth();

    switch (depth)
    {
    case CV_8U:  return L"CV_8U";
    case CV_8S:  return L"CV_8S";
    case CV_16U: return L"CV_16U";
    case CV_16S: return L"CV_16S";
    case CV_32S: return L"CV_32S";
    case CV_32F: return L"CV_32F";
    case CV_64F: return L"CV_64F";
    default:
        return L"Invalid depth type of matrix!";
    }
}

std::wstring GetMatType(const cv::Mat& mat)
{
    const int mtype = mat.type();

    switch (mtype)
    {
    case CV_8UC1:  return L"CV_8UC1";
    case CV_8UC2:  return L"CV_8UC2";
    case CV_8UC3:  return L"CV_8UC3";
    case CV_8UC4:  return L"CV_8UC4";

    case CV_8SC1:  return L"CV_8SC1";
    case CV_8SC2:  return L"CV_8SC2";
    case CV_8SC3:  return L"CV_8SC3";
    case CV_8SC4:  return L"CV_8SC4";

    case CV_16UC1: return L"CV_16UC1";
    case CV_16UC2: return L"CV_16UC2";
    case CV_16UC3: return L"CV_16UC3";
    case CV_16UC4: return L"CV_16UC4";

    case CV_16SC1: return L"CV_16SC1";
    case CV_16SC2: return L"CV_16SC2";
    case CV_16SC3: return L"CV_16SC3";
    case CV_16SC4: return L"CV_16SC4";

    case CV_32SC1: return L"CV_32SC1";
    case CV_32SC2: return L"CV_32SC2";
    case CV_32SC3: return L"CV_32SC3";
    case CV_32SC4: return L"CV_32SC4";

    case CV_32FC1: return L"CV_32FC1";
    case CV_32FC2: return L"CV_32FC2";
    case CV_32FC3: return L"CV_32FC3";
    case CV_32FC4: return L"CV_32FC4";

    case CV_64FC1: return L"CV_64FC1";
    case CV_64FC2: return L"CV_64FC2";
    case CV_64FC3: return L"CV_64FC3";
    case CV_64FC4: return L"CV_64FC4";

    default:
        return L"Invalid type of matrix!";
    }
}