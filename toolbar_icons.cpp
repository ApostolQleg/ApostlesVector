#include "pch.h"
#include "toolbar_icons.h"
#include "tools.h"
#include "shape.h"

constexpr COLORREF kMaskColor = RGB(255, 0, 255);
constexpr COLORREF kDarkColor = RGB(35, 35, 35);
constexpr COLORREF kAccentColor = RGB(0, 120, 215);
constexpr COLORREF kFillColor = RGB(210, 225, 240);

static void DrawShapeIcon(HDC hdc, UINT cmdId)
{
	auto shape = ToolRegistry::CreateShapeByCmdId(cmdId);
	if (!shape) return;

	switch (cmdId)
	{
	case IDM_NODE:  shape->SetCoords(24, 24, 24, 24); break;
	case IDM_EDGE:  shape->SetCoords(13, 35, 35, 13); break;
	case IDM_LINE:
	case IDM_ARROW: shape->SetCoords(9, 39, 39, 9);   break;
	case IDM_RECT:
	case IDM_ELIPS: shape->SetCoords(7, 12, 41, 36);  break;
	case IDM_CUBE:	shape->SetCoords(8, 40, 40, 8);   break;
	case IDM_CYL:   shape->SetCoords(12, 7, 36, 41);  break;
	default:        shape->SetCoords(8, 8, 40, 40);   break;
	}

	ShapeStyle style = {};
	style.borderColor = kDarkColor;
	style.fillColor = kFillColor;
	style.hasBorder = true;
	style.hasFill = true;
	style.borderWidth = 2;
	shape->SetStyle(style);

	shape->Show(hdc);
}

static void DrawServiceIcon(HDC hdc, UINT cmdId, int size)
{
	HPEN hDarkPen = CreatePen(PS_SOLID, 2, kDarkColor);
	HPEN hAccentPen = CreatePen(PS_SOLID, 2, kAccentColor);
	HPEN hDashPen = CreatePen(PS_DASH, 1, kAccentColor);
	HBRUSH hFillBrush = CreateSolidBrush(kFillColor);
	HBRUSH hAccentBrush = CreateSolidBrush(kAccentColor);

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hDarkPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hFillBrush));

	switch (cmdId)
	{
	case IDM_SELECT:
	{
		const POINT cursor[7] = {
			{ 14, 7 },{ 14, 37 },{ 21, 30 },{ 27, 41 },{ 31, 39 },{ 25, 28 },{ 34, 28 }
		};
		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		Polygon(hdc, cursor, 7);
		break;
	}
	case IDM_SELECT_AREA:
	{
		SetBkMode(hdc, TRANSPARENT);
		SelectObject(hdc, hDashPen);
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, 9, 10, 39, 38);

		SelectObject(hdc, hAccentPen);
		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		const POINT corners[4] = { { 9, 10 },{ 39, 10 },{ 9, 38 },{ 39, 38 } };
		for (const auto& pt : corners)
			Rectangle(hdc, pt.x - 3, pt.y - 3, pt.x + 4, pt.y + 4);
		break;
	}
	case IDM_TEXT:
	{
		HFONT hFont = CreateFontW(34, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
		HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, kDarkColor);
		RECT rc = { 0, 0, size, size };
		DrawTextW(hdc, L"Text", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SelectObject(hdc, hOldFont);
		DeleteObject(hFont);
		break;
	}
	case IDM_FLIP_H:
	{
		const POINT leftTri[3] = { { 20, 12 },{ 20, 36 },{ 8, 36 } };
		const POINT rightTri[3] = { { 28, 12 },{ 28, 36 },{ 40, 36 } };
		Polygon(hdc, leftTri, 3);
		SelectObject(hdc, hAccentBrush);
		Polygon(hdc, rightTri, 3);
		SelectObject(hdc, hAccentPen);
		MoveToEx(hdc, 24, 6, nullptr); LineTo(hdc, 24, 42);
		break;
	}
	case IDM_FLIP_V:
	{
		const POINT topTri[3] = { { 12, 20 },{ 36, 20 },{ 36, 8 } };
		const POINT botTri[3] = { { 12, 28 },{ 36, 28 },{ 36, 40 } };
		Polygon(hdc, topTri, 3);
		SelectObject(hdc, hAccentBrush);
		Polygon(hdc, botTri, 3);
		SelectObject(hdc, hAccentPen);
		MoveToEx(hdc, 6, 24, nullptr); LineTo(hdc, 42, 24);
		break;
	}
	case IDM_Z_FRONT:
		Rectangle(hdc, 8, 8, 28, 28);
		Rectangle(hdc, 20, 20, 40, 40);
		SelectObject(hdc, hAccentBrush);
		Rectangle(hdc, 14, 14, 34, 34);
		break;

	case IDM_Z_UP:
		Rectangle(hdc, 16, 18, 38, 38);
		SelectObject(hdc, hAccentBrush);
		Rectangle(hdc, 10, 10, 30, 30);
		break;

	case IDM_Z_DOWN:
		SelectObject(hdc, hAccentBrush);
		Rectangle(hdc, 16, 18, 38, 38);
		SelectObject(hdc, hFillBrush);
		Rectangle(hdc, 10, 10, 30, 30);
		break;

	case IDM_Z_BACK:
		SelectObject(hdc, hAccentBrush);
		Rectangle(hdc, 14, 14, 34, 34);
		SelectObject(hdc, hFillBrush);
		Rectangle(hdc, 8, 8, 28, 28);
		Rectangle(hdc, 20, 20, 40, 40);
		break;
	}

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hAccentBrush);
	DeleteObject(hFillBrush);
	DeleteObject(hDashPen);
	DeleteObject(hAccentPen);
	DeleteObject(hDarkPen);
}

static HBITMAP CreateIconBitmap(HDC screenDC, const ToolDescriptor& tool, int size)
{
	HDC memDC = CreateCompatibleDC(screenDC);
	HBITMAP hBmp = CreateCompatibleBitmap(screenDC, size, size);
	HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(memDC, hBmp));

	HBRUSH hMaskBrush = CreateSolidBrush(kMaskColor);
	RECT bg = { 0, 0, size, size };
	FillRect(memDC, &bg, hMaskBrush);
	DeleteObject(hMaskBrush);

	if (tool.createShape && tool.cmdId != IDM_TEXT)
		DrawShapeIcon(memDC, tool.cmdId);
	else
		DrawServiceIcon(memDC, tool.cmdId, size);

	SelectObject(memDC, hOldBmp);
	DeleteDC(memDC);
	return hBmp;
}

HIMAGELIST ToolbarIcons::CreateImageList(int iconSize)
{
	const auto tools = ToolRegistry::GetAll();
	HIMAGELIST hList = ImageList_Create(
		iconSize, iconSize, ILC_COLOR32 | ILC_MASK, static_cast<int>(tools.size()), 0);
	if (!hList) return nullptr;

	HDC screenDC = GetDC(nullptr);
	for (const auto& tool : tools)
	{
		HBITMAP hBmp = CreateIconBitmap(screenDC, tool, iconSize);
		ImageList_AddMasked(hList, hBmp, kMaskColor);
		DeleteObject(hBmp);
	}
	ReleaseDC(nullptr, screenDC);
	return hList;
}