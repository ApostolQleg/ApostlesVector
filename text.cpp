#include "pch.h"
#include "framework.h"
#include "text.h"
#include "shape.h"
#include "viewer.h"

constexpr long kMinTextBoxWidth = 180;
constexpr long kMinTextBoxHeight = 60;
constexpr int kMaxTextLength = 1000;

static LRESULT CALLBACK InplaceEditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR idSubclass, DWORD_PTR)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			PostMessageW(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(kCmdTextCancel, 0), 0);
			return 0;
		}
		if (wParam == VK_RETURN && !(GetKeyState(VK_SHIFT) & 0x8000))
		{
			PostMessageW(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(kCmdTextCommit, 0), 0);
			return 0;
		}
		break;

	case WM_CHAR:
		if (wParam == VK_ESCAPE || wParam == L'\n')
			return 0;
		if (wParam == L'\r' && !(GetKeyState(VK_SHIFT) & 0x8000))
			return 0;
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hEdit, InplaceEditProc, idSubclass);
		break;
	}
	return DefSubclassProc(hEdit, msg, wParam, lParam);
}

InplaceTextEditor::~InplaceTextEditor()
{
	if (hFont)
	{
		DeleteObject(hFont);
		hFont = nullptr;
	}
}

void InplaceTextEditor::Begin(HWND hParent, const Viewer& viewer, Text* target,
	long l, long t, long r, long b, int defaultBorderWidth)
{
	End(hParent, true);

	if (r - l < kMinTextBoxWidth)  r = l + kMinTextBoxWidth;
	if (b - t < kMinTextBoxHeight) b = t + kMinTextBoxHeight;

	editTarget = target;
	editSceneRect = { l, t, r, b };

	long cx = 0, cy = 0;
	viewer.SceneToClient(l, t, cx, cy);

	const int bw = target ? target->GetBorderWidth() : defaultBorderWidth;
	hFont = CreateFontW(16 + bw * 4, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

	// У фігурі переноси зберігаються як '\n', а елемент EDIT потребує "\r\n"
	std::wstring initial;
	if (target)
	{
		for (wchar_t ch : target->GetText())
		{
			if (ch == L'\n') initial += L"\r\n";
			else initial += ch;
		}
	}

	auto hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hParent, GWLP_HINSTANCE));
	hEdit = CreateWindowExW(0, L"EDIT", initial.c_str(),
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
		cx, cy, static_cast<int>(r - l), static_cast<int>(b - t),
		hParent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(kInplaceEditCtrlId)), hInst, nullptr);

	if (!hEdit)
	{
		DeleteObject(hFont);
		hFont = nullptr;
		editTarget = nullptr;
		return;
	}

	SendMessageW(hEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
	SendMessageW(hEdit, EM_LIMITTEXT, kMaxTextLength, 0);
	SetWindowSubclass(hEdit, InplaceEditProc, 1, 0);
	SetFocus(hEdit);
	SendMessageW(hEdit, EM_SETSEL, 0, -1);
}

TextEditResult InplaceTextEditor::End(HWND hParent, bool commit)
{
	TextEditResult result = {};
	if (!hEdit) return result;

	HWND hLocal = hEdit;
	hEdit = nullptr; // Захист від повторного входу через EN_KILLFOCUS

	std::wstring cleanText;
	if (commit)
	{
		const int len = GetWindowTextLengthW(hLocal);
		std::wstring raw(len, L'\0');
		if (len > 0)
			GetWindowTextW(hLocal, raw.data(), len + 1);

		cleanText.reserve(raw.size());
		for (wchar_t ch : raw)
		{
			if (ch != L'\r') cleanText += ch;
		}
	}

	DestroyWindow(hLocal);
	if (hFont)
	{
		DeleteObject(hFont);
		hFont = nullptr;
	}

	result.target = editTarget;
	result.sceneRect = editSceneRect;
	editTarget = nullptr;

	if (commit && !cleanText.empty())
	{
		result.committed = true;
		result.text = std::move(cleanText);
	}

	if (hParent && !GetFocus())
		SetFocus(hParent);

	return result;
}
