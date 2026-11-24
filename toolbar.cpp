#include "pch.h"
#include "framework.h"
#include "toolbar.h"

#define IDC_MAIN_TOOLBAR 10001

struct ToolItem
{
	UINT id;
	int bitmapIndex;
	const WCHAR* tooltip;
};

static const ToolItem shapeTools[] = {
	{ IDM_NODE,        0,  L"Node" },
	{ IDM_EDGE,        1,  L"Edge" },
	{ IDM_LINE,        2,  L"Line" },
	{ IDM_ARROW,       3,  L"Arrow" },
	{ IDM_TRNGL,       4,  L"Triangle" },
	{ IDM_RIGHT_TRNGL, 5,  L"Right Triangle" },
	{ IDM_RECT,        6,  L"Rectangle" },
	{ IDM_ROMB,        7,  L"Rhombus" },
	{ IDM_PENT,        8,  L"Pentagon" },
	{ IDM_HEX,         9,  L"Hexagon" },
	{ IDM_ELIPS,       10, L"Ellipse" },
	{ IDM_CUBE,        11, L"Cube" },
	{ IDM_CYL,         12, L"Cylinder" },
	{ IDM_TEXT,        13, L"Text" }
};

static const int shapeToolsCount = sizeof(shapeTools) / sizeof(shapeTools[0]);

void Toolbar::Create(HWND hWndParent, HINSTANCE hInst)
{
	hWndToolbar = CreateWindowExW(
		0,
		TOOLBARCLASSNAMEW,
		nullptr,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS | CCS_TOP | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		0, 0, 0, 0,
		hWndParent,
		(HMENU)IDC_MAIN_TOOLBAR,
		hInst,
		nullptr
	);

	if (!hWndToolbar) return;

	SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	SendMessage(hWndToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(48, 48));
	SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(48, 48));

	TBADDBITMAP tbab = {};
	tbab.hInst = hInst;
	tbab.nID = IDB_TOOLBAR;
	SendMessage(hWndToolbar, TB_ADDBITMAP, shapeToolsCount, (LPARAM)&tbab);

	// 2 індикатори кольору + 1 сепаратор + 14 кнопок фігур = 17 елементів
	const int totalButtons = 3 + shapeToolsCount;
	TBBUTTON tbb[totalButtons] = {};

	// індикатор кольору межі
	tbb[0].iBitmap = I_IMAGENONE;
	tbb[0].idCommand = IDM_BORDER_COLOR;
	tbb[0].fsState = TBSTATE_ENABLED;
	tbb[0].fsStyle = BTNS_BUTTON;

	// індикатор кольору заповнення
	tbb[1].iBitmap = I_IMAGENONE;
	tbb[1].idCommand = IDM_FILL_COLOR;
	tbb[1].fsState = TBSTATE_ENABLED;
	tbb[1].fsStyle = BTNS_BUTTON;

	// розділювач
	tbb[2].fsStyle = TBSTYLE_SEP;

	// кнопки фігур
	for (int i = 0; i < shapeToolsCount; i++)
	{
		tbb[3 + i].iBitmap = shapeTools[i].bitmapIndex;
		tbb[3 + i].idCommand = shapeTools[i].id;
		tbb[3 + i].fsState = TBSTATE_ENABLED;
		tbb[3 + i].fsStyle = BTNS_BUTTON;
	}

	SendMessage(hWndToolbar, TB_ADDBUTTONSW, totalButtons, (LPARAM)&tbb);

	TBBUTTONINFO tbbi = {};
	tbbi.cbSize = sizeof(TBBUTTONINFO);
	tbbi.dwMask = TBIF_SIZE;
	tbbi.cx = 30;
	SendMessage(hWndToolbar, TB_SETBUTTONINFOW, IDM_BORDER_COLOR, (LPARAM)&tbbi);
	SendMessage(hWndToolbar, TB_SETBUTTONINFOW, IDM_FILL_COLOR, (LPARAM)&tbbi);

	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
}

void Toolbar::OnSize(HWND hWndParent)
{
	if (!hWndToolbar) return;

	RECT rcClient = {};
	RECT rcToolbar = {};
	GetClientRect(hWndParent, &rcClient);
	GetWindowRect(hWndToolbar, &rcToolbar);

	MoveWindow(
		hWndToolbar,
		0, 0,
		rcClient.right - rcClient.left,
		rcToolbar.bottom - rcToolbar.top,
		TRUE
	);
}

int Toolbar::GetHeight() const
{
	if (!hWndToolbar) return 0;

	RECT rc = {};
	GetWindowRect(hWndToolbar, &rc);
	return rc.bottom - rc.top;
}

void Toolbar::SetActiveTool(UINT cmdId)
{
	if (!hWndToolbar) return;

	for (int i = 0; i < shapeToolsCount; i++)
	{
		UINT id = shapeTools[i].id;
		SendMessage(hWndToolbar, TB_PRESSBUTTON, id, (id == cmdId) ? TRUE : FALSE);
	}
}

void Toolbar::UpdateColors(COLORREF border, COLORREF fill, bool filled)
{
	borderColor = border;
	fillColor = fill;
	hasFill = filled;

	if (hWndToolbar)
	{
		InvalidateRect(hWndToolbar, nullptr, TRUE);
	}
}

bool Toolbar::IsToolId(UINT cmdId) const
{
	for (int i = 0; i < shapeToolsCount; i++)
	{
		if (shapeTools[i].id == cmdId)
			return true;
	}
	return false;
}

void Toolbar::DrawColorIndicator(LPNMTBCUSTOMDRAW lpcd) const
{
	HDC hdc = lpcd->nmcd.hdc;
	RECT rc = lpcd->nmcd.rc;
	InflateRect(&rc, -6, -6);

	if (rc.right <= rc.left || rc.bottom <= rc.top) return;

	UINT id = (UINT)lpcd->nmcd.dwItemSpec;
	bool isNoFill = (id == IDM_FILL_COLOR && !hasFill);
	COLORREF squareColor = (id == IDM_BORDER_COLOR) ? borderColor : (hasFill ? fillColor : RGB(255, 255, 255));

	HPEN hFramePen = CreatePen(PS_SOLID, 1, RGB(90, 90, 90));
	HBRUSH hFillBrush = CreateSolidBrush(squareColor);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hFramePen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFillBrush);

	Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

	if (isNoFill)
	{
		HPEN hSlashPen = CreatePen(PS_SOLID, 2, RGB(210, 30, 30));
		SelectObject(hdc, hSlashPen);
		MoveToEx(hdc, rc.left + 2, rc.bottom - 3, nullptr);
		LineTo(hdc, rc.right - 2, rc.top + 1);
		SelectObject(hdc, hFramePen);
		DeleteObject(hSlashPen);
	}

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hFillBrush);
	DeleteObject(hFramePen);
}

LRESULT Toolbar::OnNotify(LPARAM lParam)
{
	LPNMHDR pnmh = (LPNMHDR)lParam;
	if (!pnmh) return 0;

	if (pnmh->code == TTN_NEEDTEXTW)
	{
		LPTOOLTIPTEXTW lpttt = (LPTOOLTIPTEXTW)lParam;
		UINT id = (UINT)lpttt->hdr.idFrom;

		if (id == IDM_BORDER_COLOR)
		{
			lstrcpyW(lpttt->szText, L"Border Color");
			return 0;
		}
		if (id == IDM_FILL_COLOR)
		{
			lstrcpyW(lpttt->szText, L"Fill Color");
			return 0;
		}

		for (int i = 0; i < shapeToolsCount; i++)
		{
			if (shapeTools[i].id == id)
			{
				lstrcpyW(lpttt->szText, shapeTools[i].tooltip);
				return 0;
			}
		}
	}
	else if (pnmh->code == NM_CUSTOMDRAW && pnmh->hwndFrom == hWndToolbar)
	{
		LPNMTBCUSTOMDRAW lpcd = (LPNMTBCUSTOMDRAW)lParam;
		switch (lpcd->nmcd.dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:
			if (lpcd->nmcd.dwItemSpec == IDM_BORDER_COLOR || lpcd->nmcd.dwItemSpec == IDM_FILL_COLOR)
				return CDRF_NOTIFYPOSTPAINT;
			return CDRF_DODEFAULT;

		case CDDS_ITEMPOSTPAINT:
			DrawColorIndicator(lpcd);
			return CDRF_DODEFAULT;
		}
	}

	return 0;
}
