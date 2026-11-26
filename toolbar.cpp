#include "pch.h"
#include "toolbar.h"
#include "toolbar_icons.h"
#include "tools.h"

constexpr UINT kMainToolbarId = 10001;
constexpr int kIconSize = 48;

Toolbar::~Toolbar()
{
	if (hImageList)
	{
		ImageList_Destroy(hImageList);
		hImageList = nullptr;
	}
}

void Toolbar::Create(HWND hWndParent, HINSTANCE hInst)
{
	hToolbar = CreateWindowExW(
		0, TOOLBARCLASSNAMEW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_TOP | CCS_NODIVIDER |
		TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		0, 0, 0, 0, hWndParent,
		reinterpret_cast<HMENU>(static_cast<UINT_PTR>(kMainToolbarId)), hInst, nullptr);
	if (!hToolbar) return;

	SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessageW(hToolbar, TB_SETEXTENDEDSTYLE, 0,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
	SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(kIconSize, kIconSize));
	SendMessageW(hToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(58, 58));

	hImageList = ToolbarIcons::CreateImageList(kIconSize);
	SendMessageW(hToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImageList));

	const auto tools = ToolRegistry::GetAll();
	std::vector<TBBUTTON> buttons;
	buttons.reserve(tools.size() + 3);

	auto addSeparator = [&]() {
		TBBUTTON sep = {};
		sep.fsStyle = TBSTYLE_SEP;
		buttons.push_back(sep);
		};

	for (size_t i = 0; i < tools.size(); ++i)
	{
		if (i == 2 || i == 16 || i == 18)
			addSeparator();

		TBBUTTON btn = {};
		btn.iBitmap = tools[i].bitmapIndex;
		btn.idCommand = static_cast<int>(tools[i].cmdId);
		btn.fsState = (tools[i].kind == ToolbarButtonKind::ModeTool) ? TBSTATE_ENABLED : 0;
		btn.fsStyle = (tools[i].kind == ToolbarButtonKind::ModeTool) ? BTNS_CHECKGROUP : BTNS_BUTTON;
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
	RECT rcClient = {}, rcBar = {};
	GetClientRect(hWndParent, &rcClient);
	GetWindowRect(hToolbar, &rcBar);
	MoveWindow(hToolbar, 0, 0, rcClient.right - rcClient.left, rcBar.bottom - rcBar.top, TRUE);
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
		if (tool.kind == ToolbarButtonKind::ModeTool)
			SendMessageW(hToolbar, TB_CHECKBUTTON, tool.cmdId, MAKELPARAM(tool.cmdId == cmdId, 0));
	}
}

void Toolbar::UpdateActionStates(int selectionCount)
{
	if (!hToolbar) return;
	for (const auto& tool : ToolRegistry::GetAll())
	{
		if (tool.kind == ToolbarButtonKind::SingleAction)
			SendMessageW(hToolbar, TB_ENABLEBUTTON, tool.cmdId, MAKELPARAM(selectionCount == 1, 0));
		else if (tool.kind == ToolbarButtonKind::SelectionAction)
			SendMessageW(hToolbar, TB_ENABLEBUTTON, tool.cmdId, MAKELPARAM(selectionCount >= 1, 0));
	}
}

LRESULT Toolbar::OnNotify(LPARAM lParam) const
{
	const auto* pnmh = reinterpret_cast<LPNMHDR>(lParam);
	if (!pnmh || pnmh->code != TTN_NEEDTEXTW) return 0;

	auto* lpttt = reinterpret_cast<LPTOOLTIPTEXTW>(lParam);
	const UINT id = static_cast<UINT>(lpttt->hdr.idFrom);
	if (const ToolDescriptor* tool = ToolRegistry::FindByCmdId(id))
		wcscpy_s(lpttt->szText, _countof(lpttt->szText), tool->tooltip);
	return 0;
}