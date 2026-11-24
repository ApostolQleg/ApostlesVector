#include "pch.h"
#include "framework.h"
#include "ApostlesVector.h"
#include "editor.h"

constexpr int kMaxLoadString = 100;

HINSTANCE hInst = nullptr;
HWND hMainWnd = nullptr;
WCHAR szTitle[kMaxLoadString] = {};
WCHAR szWindowClass[kMaxLoadString] = {};

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
	InitCommonControlsEx(&icc);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, kMaxLoadString);
	LoadStringW(hInstance, IDC_APOSTLESVECTOR, szWindowClass, kMaxLoadString);
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDC_APOSTLESVECTOR));

	MSG msg = {};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		const bool skipAccel = Editor::GetInstance()->IsTextEditing();
		if (skipAccel || !TranslateAcceleratorW(hMainWnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APOSTLESVECTOR));
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_APOSTLESVECTOR);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	HWND hWnd = CreateWindowW(szWindowClass, szTitle,
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	hMainWnd = hWnd;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		Editor::GetInstance()->OnCreate(hWnd);
		break;
	case WM_SIZE:
		Editor::GetInstance()->OnSize(hWnd);
		break;
	case WM_NOTIFY:
		return Editor::GetInstance()->OnNotify(lParam);
	case WM_ERASEBKGND:
		return 1;
	case WM_HSCROLL:
		Editor::GetInstance()->OnHScroll(hWnd, wParam);
		break;
	case WM_VSCROLL:
		Editor::GetInstance()->OnVScroll(hWnd, wParam);
		break;
	case WM_MOUSEWHEEL:
		Editor::GetInstance()->OnMouseWheel(hWnd, wParam);
		break;
	case WM_LBUTTONDOWN:
		Editor::GetInstance()->OnLBDown(hWnd, lParam);
		break;
	case WM_LBUTTONDBLCLK:
		Editor::GetInstance()->OnLBDblClk(hWnd);
		break;
	case WM_MOUSEMOVE:
		Editor::GetInstance()->OnMouseMove(hWnd, lParam);
		break;
	case WM_LBUTTONUP:
		Editor::GetInstance()->OnLBUp(hWnd, lParam);
		break;
	case WM_PAINT:
		Editor::GetInstance()->OnPaint(hWnd);
		break;
	case WM_INITMENUPOPUP:
		Editor::GetInstance()->OnInitMenuPopup(hWnd);
		break;
	case WM_COMMAND:
	{
		if (Editor::GetInstance()->OnCommand(hWnd, wParam))
			break;

		const int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			SendMessageW(hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_CLOSE:
		if (Editor::GetInstance()->OnClose(hWnd))
			DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
