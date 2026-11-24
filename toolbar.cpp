#include "pch.h"
#include "framework.h"
#include "toolbar.h"
#include "tools.h"

constexpr UINT kMainToolbarId = 10001;

void Toolbar::Create(HWND hWndParent, HINSTANCE hInst)
{
	hToolbar = CreateWindowExW(
		0, TOOLBARCLASSNAMEW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_TOP | CCS_NODIVIDER |
		TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		0, 0, 0, 0, hWndParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(kMainToolbarId)), hInst, nullptr);
	if (!hToolbar) return;

	SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessageW(hToolbar, TB_SETEXTENDEDSTYLE, 0,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
	SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(48, 48));
	SendMessageW(hToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(58, 58));

	const auto tools = ToolRegistry::GetAll();

	TBADDBITMAP tbab = {};
	tbab.hInst = hInst;
	tbab.nID = IDB_TOOLBAR;
	SendMessageW(hToolbar, TB_ADDBITMAP, static_cast<WPARAM>(tools.size()), reinterpret_cast<LPARAM>(&tbab));

	std::vector<TBBUTTON> buttons;
	buttons.reserve(tools.size() + 1);

	for (size_t i = 0; i < tools.size(); ++i)
	{
		if (i == 2)
		{
			TBBUTTON sep = {};
			sep.fsStyle = TBSTYLE_SEP;
			buttons.push_back(sep);
		}

		TBBUTTON btn = {};
		btn.iBitmap = tools[i].bitmapIndex;
		btn.idCommand = static_cast<int>(tools[i].cmdId);
		btn.fsState = TBSTATE_ENABLED;
		btn.fsStyle = BTNS_CHECKGROUP;
		buttons.push_back(btn);
	}

	SendMessageW(hToolbar, TB_ADDBUTTONSW,
		static_cast<WPARAM>(buttons.size()),
		reinterpret_cast<LPARAM>(buttons.data()));
	SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
}

void Toolbar::OnSize(HWND hWndParent)
{
	if (!hToolbar) return;

	RECT rcClient = {};
	RECT rcToolbar = {};
	GetClientRect(hWndParent, &rcClient);
	GetWindowRect(hToolbar, &rcToolbar);

	MoveWindow(
		hToolbar,
		0, 0,
		rcClient.right - rcClient.left,
		rcToolbar.bottom - rcToolbar.top,
		TRUE
	);
}

int Toolbar::GetHeight() const
{
	if (!hToolbar) return 0;

	RECT rc = {};
	GetWindowRect(hToolbar, &rc);
	return rc.bottom - rc.top;
}

void Toolbar::SetActiveTool(UINT cmdId)
{
	if (!hToolbar) return;

	for (const auto& tool : ToolRegistry::GetAll())
	{
		SendMessageW(hToolbar, TB_CHECKBUTTON, tool.cmdId, MAKELPARAM(tool.cmdId == cmdId, 0));
	}
}

bool Toolbar::IsToolId(UINT cmdId) const
{
	return ToolRegistry::IsToolId(cmdId);
}

LRESULT Toolbar::OnNotify(LPARAM lParam)
{
	const auto* pnmh = reinterpret_cast<LPNMHDR>(lParam);
	if (!pnmh) return 0;

	if (pnmh->code == TTN_NEEDTEXTW)
	{
		auto* lpttt = reinterpret_cast<LPTOOLTIPTEXTW>(lParam);
		const UINT id = static_cast<UINT>(lpttt->hdr.idFrom);
		if (const ToolDescriptor* tool = ToolRegistry::FindByCmdId(id))
		{
			wcscpy_s(lpttt->szText, _countof(lpttt->szText), tool->tooltip);
			return 0;
		}
	}

	return 0;
}
