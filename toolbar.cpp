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

static const ToolItem toolbarItems[] = {
	{ IDM_SELECT,      0,  L"Select" },
	{ IDM_MOVE,        1,  L"Move" },
	{ IDM_NODE,        2,  L"Node" },
	{ IDM_EDGE,        3,  L"Edge" },
	{ IDM_LINE,        4,  L"Line" },
	{ IDM_ARROW,       5,  L"Arrow" },
	{ IDM_TRNGL,       6,  L"Triangle" },
	{ IDM_RIGHT_TRNGL, 7,  L"Right Triangle" },
	{ IDM_RECT,        8,  L"Rectangle" },
	{ IDM_ROMB,        9,  L"Rhombus" },
	{ IDM_PENT,        10, L"Pentagon" },
	{ IDM_HEX,         11, L"Hexagon" },
	{ IDM_ELIPS,       12, L"Ellipse" },
	{ IDM_CUBE,        13, L"Cube" },
	{ IDM_CYL,         14, L"Cylinder" },
	{ IDM_TEXT,        15, L"Text" }
};

static constexpr int toolbarItemsCount = sizeof(toolbarItems) / sizeof(toolbarItems[0]);

void Toolbar::Create(HWND hWndParent, HINSTANCE hInst)
{
	hWndToolbar = CreateWindowExW(
		0, TOOLBARCLASSNAMEW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_TOP | CCS_NODIVIDER |
		TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		0, 0, 0, 0, hWndParent, (HMENU)IDC_MAIN_TOOLBAR, hInst, nullptr);
	if (!hWndToolbar) return;

	SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	SendMessage(hWndToolbar, TB_SETEXTENDEDSTYLE, 0,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
	SendMessage(hWndToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(48, 48));
	SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(58, 58));

	TBADDBITMAP tbab = {};
	tbab.hInst = hInst;
	tbab.nID = IDB_TOOLBAR;
	SendMessage(hWndToolbar, TB_ADDBITMAP, toolbarItemsCount, (LPARAM)&tbab);

	constexpr int totalButtons = toolbarItemsCount + 1; // + 1 сепаратор
	TBBUTTON tbb[totalButtons] = {};
	int n = 0;

	for (int i = 0; i < toolbarItemsCount; i++)
	{
		if (i == 2)
			tbb[n++].fsStyle = TBSTYLE_SEP;

		tbb[n].iBitmap = toolbarItems[i].bitmapIndex;
		tbb[n].idCommand = toolbarItems[i].id;
		tbb[n].fsState = TBSTATE_ENABLED;
		tbb[n].fsStyle = BTNS_CHECKGROUP;
		n++;
	}

	SendMessage(hWndToolbar, TB_ADDBUTTONSW, n, (LPARAM)tbb);
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

	for (int i = 0; i < toolbarItemsCount; i++)
	{
		UINT id = toolbarItems[i].id;
		SendMessage(hWndToolbar, TB_CHECKBUTTON, id, MAKELPARAM(id == cmdId, 0));
	}
}

bool Toolbar::IsToolId(UINT cmdId) const
{
	for (int i = 0; i < toolbarItemsCount; i++)
	{
		if (toolbarItems[i].id == cmdId)
			return true;
	}
	return false;
}

LRESULT Toolbar::OnNotify(LPARAM lParam)
{
	LPNMHDR pnmh = (LPNMHDR)lParam;
	if (!pnmh) return 0;

	if (pnmh->code == TTN_NEEDTEXTW)
	{
		LPTOOLTIPTEXTW lpttt = (LPTOOLTIPTEXTW)lParam;
		UINT id = (UINT)lpttt->hdr.idFrom;

		for (int i = 0; i < toolbarItemsCount; i++)
		{
			if (toolbarItems[i].id == id)
			{
				wcscpy_s(lpttt->szText, _countof(lpttt->szText), toolbarItems[i].tooltip);
				return 0;
			}
		}
	}

	return 0;
}
