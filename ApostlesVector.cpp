#include "pch.h"
#include "ApostlesVector.h"
#include "editor.h"

constexpr int kMaxLoadString = 100;

static HINSTANCE gInstance = nullptr;
static HWND gMainWnd = nullptr;
static WCHAR gTitle[kMaxLoadString] = {};
static WCHAR gWindowClass[kMaxLoadString] = {};
static std::unique_ptr<Editor> gEditor;

static ATOM RegisterMainWindowClass(HINSTANCE hInstance);
static BOOL CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
	InitCommonControlsEx(&icc);

	LoadStringW(hInstance, IDS_APP_TITLE, gTitle, kMaxLoadString);
	LoadStringW(hInstance, IDC_APOSTLESVECTOR, gWindowClass, kMaxLoadString);
	RegisterMainWindowClass(hInstance);

	gEditor = std::make_unique<Editor>();

	if (!CreateMainWindow(hInstance, nCmdShow))
		return FALSE;

	int argc = 0;
	if (LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc))
	{
		if (argc >= 2 && argv[1][0] != L'\0')
			gEditor->OpenFile(gMainWnd, argv[1]);
		LocalFree(argv);
	}

	HACCEL hAccelTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDC_APOSTLESVECTOR));

	MSG msg = {};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		const bool skipAccel = gEditor && gEditor->IsTextEditing();
		if (skipAccel || !TranslateAcceleratorW(gMainWnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	gEditor.reset();
	return static_cast<int>(msg.wParam);
}

static ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = MainWndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APOSTLESVECTOR));
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_APOSTLESVECTOR);
	wcex.lpszClassName = gWindowClass;
	wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_SMALL));
	return RegisterClassExW(&wcex);
}

static BOOL CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
	gInstance = hInstance;
	HWND hWnd = CreateWindowW(gWindowClass, gTitle,
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) return FALSE;

	gMainWnd = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		if (gEditor) gEditor->OnCreate(hWnd);
		return 0;
	case WM_SIZE:
		if (gEditor) gEditor->OnSize(hWnd);
		return 0;
	case WM_NOTIFY:
		return gEditor ? gEditor->OnNotify(lParam) : 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_HSCROLL:
		if (gEditor) gEditor->OnHScroll(hWnd, wParam);
		return 0;
	case WM_VSCROLL:
		if (gEditor) gEditor->OnVScroll(hWnd, wParam);
		return 0;
	case WM_MOUSEWHEEL:
		if (gEditor) gEditor->OnMouseWheel(hWnd, wParam);
		return 0;
	case WM_LBUTTONDOWN:
		if (gEditor) gEditor->OnLBDown(hWnd, lParam);
		return 0;
	case WM_LBUTTONDBLCLK:
		if (gEditor) gEditor->OnLBDblClk(hWnd);
		return 0;
	case WM_MOUSEMOVE:
		if (gEditor) gEditor->OnMouseMove(hWnd, lParam);
		return 0;
	case WM_LBUTTONUP:
		if (gEditor) gEditor->OnLBUp(hWnd, lParam);
		return 0;
	case WM_PAINT:
		if (gEditor) gEditor->OnPaint(hWnd);
		return 0;
	case WM_INITMENUPOPUP:
		if (gEditor) gEditor->OnInitMenuPopup(hWnd);
		return 0;
	case WM_COMMAND:
	{
		if (gEditor && gEditor->OnCommand(hWnd, wParam))
			return 0;

		switch (LOWORD(wParam))
		{
		case IDM_ABOUT:
			DialogBoxW(gInstance, MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, AboutDialogProc);
			return 0;
		case IDM_EXIT:
			SendMessageW(hWnd, WM_CLOSE, 0, 0);
			return 0;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	case WM_CLOSE:
		if (!gEditor || gEditor->OnClose(hWnd))
			DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
}

static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}