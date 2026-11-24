#include "pch.h"
#include "framework.h"
#include "shape.h"
#include <cmath>

constexpr double PI = 3.14159265358979323846;

// Shape

void Shape::Set(long xstart, long ystart, long xend, long yend)
{
	x1 = xstart;
	y1 = ystart;
	x2 = xend;
	y2 = yend;
}

void Shape::SetStyle(COLORREF border, COLORREF fill, bool filled, int width)
{
	borderColor = border;
	fillColor = fill;
	hasFill = filled;
	borderWidth = (width > 0) ? width : 1;
}

HPEN Shape::CreateBorderPen() const
{
	if (rubber)
		return CreatePen(PS_DOT, 1, RGB(0, 0, 0));
	return CreatePen(PS_SOLID, borderWidth, borderColor);
}

void Shape::NormalizeCoords(long& l, long& t, long& r, long& b) const
{
	l = (x1 < x2) ? x1 : x2;
	r = (x1 < x2) ? x2 : x1;
	t = (y1 < y2) ? y1 : y2;
	b = (y1 < y2) ? y2 : y1;
}

void Shape::Serialize(FILE* fout) const
{
	if (!fout) return;
	fwprintf(fout, L"%s %ld %ld %ld %ld %lu %lu %d %d\n",
		GetTypeName(), x1, y1, x2, y2,
		(unsigned long)borderColor, (unsigned long)fillColor,
		hasFill ? 1 : 0, borderWidth);
}

bool Shape::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int filled = 0, width = 1;

	if (swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &filled, &width) == 8)
	{
		borderColor = (COLORREF)bCol;
		fillColor = (COLORREF)fCol;
		hasFill = (filled != 0);
		borderWidth = (width > 0) ? width : 1;
		return true;
	}
	return false;
}

Shape* Shape::CreateByName(const WCHAR* typeName)
{
	if (!typeName) return nullptr;

	if (wcscmp(typeName, L"Node") == 0)          return new Node();
	if (wcscmp(typeName, L"Edge") == 0)          return new Edge();
	if (wcscmp(typeName, L"Line") == 0)          return new Line();
	if (wcscmp(typeName, L"Arrow") == 0)         return new Arrow();
	if (wcscmp(typeName, L"Triangle") == 0)      return new Trngl();
	if (wcscmp(typeName, L"RightTriangle") == 0) return new RightTrngl();
	if (wcscmp(typeName, L"Rectangle") == 0)     return new Rect();
	if (wcscmp(typeName, L"Rhombus") == 0)       return new Romb();
	if (wcscmp(typeName, L"Pentagon") == 0)      return new Pent();
	if (wcscmp(typeName, L"Hexagon") == 0)       return new Hex();
	if (wcscmp(typeName, L"Ellipse") == 0)       return new Elips();
	if (wcscmp(typeName, L"Cube") == 0)          return new Cube();
	if (wcscmp(typeName, L"Cylinder") == 0)      return new Cyl();
	if (wcscmp(typeName, L"Text") == 0)          return new Text();

	return nullptr;
}

// Line, Rect, Elips

void Line::Show(HDC hdc)
{
	HPEN hPen = CreateBorderPen();
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);

	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);
}

void Rect::Show(HDC hdc)
{
	bool useBrush = (hasFill && !rubber);
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = useBrush ? CreateSolidBrush(fillColor) : (HBRUSH)GetStockObject(NULL_BRUSH);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	Rectangle(hdc, x1, y1, x2, y2);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (useBrush) DeleteObject(hBrush);
	DeleteObject(hPen);
}

void Elips::Show(HDC hdc)
{
	bool useBrush = (hasFill && !rubber);
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = useBrush ? CreateSolidBrush(fillColor) : (HBRUSH)GetStockObject(NULL_BRUSH);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	Ellipse(hdc, x1, y1, x2, y2);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (useBrush) DeleteObject(hBrush);
	DeleteObject(hPen);
}

// Багатокутники

void Poly::Show(HDC hdc)
{
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count < 3) return;

	bool useBrush = (hasFill && !rubber);
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = useBrush ? CreateSolidBrush(fillColor) : (HBRUSH)GetStockObject(NULL_BRUSH);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	Polygon(hdc, pts, count);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (useBrush) DeleteObject(hBrush);
	DeleteObject(hPen);
}

void Trngl::CalcVertices(POINT* pts, int& count) const
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	count = 3;
	pts[0] = { (l + r) / 2, t };
	pts[1] = { r, b };
	pts[2] = { l, b };
}

void RightTrngl::CalcVertices(POINT* pts, int& count) const
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	count = 3;
	pts[0] = { l, t };
	pts[1] = { r, b };
	pts[2] = { l, b };
}

void Romb::CalcVertices(POINT* pts, int& count) const
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	count = 4;
	pts[0] = { (l + r) / 2, t };
	pts[1] = { r, (t + b) / 2 };
	pts[2] = { (l + r) / 2, b };
	pts[3] = { l, (t + b) / 2 };
}

static void CalcRegularPolygon(long l, long t, long r, long b, int sides, POINT* pts)
{
	double cx = (l + r) / 2.0;
	double cy = (t + b) / 2.0;
	double rx = (r - l) / 2.0;
	double ry = (b - t) / 2.0;
	double startAngle = -PI / 2.0; // верхня вершина спрямована вгору

	for (int i = 0; i < sides; i++)
	{
		double angle = startAngle + (2.0 * PI * i) / sides;
		pts[i].x = std::lround(cx + rx * std::cos(angle));
		pts[i].y = std::lround(cy + ry * std::sin(angle));
	}
}

void Pent::CalcVertices(POINT* pts, int& count) const
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	count = 5;
	CalcRegularPolygon(l, t, r, b, count, pts);
}

void Hex::CalcVertices(POINT* pts, int& count) const
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	count = 6;
	CalcRegularPolygon(l, t, r, b, count, pts);
}

// Node, Arrow, Edge, Cube, Cylinder

void Node::Show(HDC hdc)
{
	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;

	const long r = NODE_RADIUS;
	Set(xold1 - r, yold1 - r, xold1 + r, yold1 + r);
	Elips::Show(hdc);

	Set(xold1, yold1, xold2, yold2);
}

void Arrow::Show(HDC hdc)
{
	Line::Show(hdc);

	double dx = (double)(x2 - x1);
	double dy = (double)(y2 - y1);
	double len = std::hypot(dx, dy);
	if (len < 1.0) return;

	double angle = std::atan2(dy, dx);
	double wingLen = 12.0 + borderWidth * 2.0;
	if (wingLen > len * 0.6) wingLen = len * 0.6;
	constexpr double spread = 25.0 * PI / 180.0;

	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;

	long wx1 = xold2 - std::lround(wingLen * std::cos(angle - spread));
	long wy1 = yold2 - std::lround(wingLen * std::sin(angle - spread));
	Set(xold2, yold2, wx1, wy1);
	Line::Show(hdc);

	long wx2 = xold2 - std::lround(wingLen * std::cos(angle + spread));
	long wy2 = yold2 - std::lround(wingLen * std::sin(angle + spread));
	Set(xold2, yold2, wx2, wy2);
	Line::Show(hdc);

	Set(xold1, yold1, xold2, yold2);
}

void Edge::Show(HDC hdc)
{
	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;

	Line::Show(hdc);

	const long r = EDGE_RADIUS;

	Set(xold1 - r, yold1 - r, xold1 + r, yold1 + r);
	Elips::Show(hdc);

	Set(xold2 - r, yold2 - r, xold2 + r, yold2 + r);
	Elips::Show(hdc);

	Set(xold1, yold1, xold2, yold2);
}

void Cube::Show(HDC hdc)
{
	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;

	long l, t, r, b;
	NormalizeCoords(l, t, r, b);

	long w = r - l;
	long h = b - t;
	long d = ((w < h) ? w : h) / 3;

	// задній квадратик
	Set(l + d, t, r, b - d);
	Rect::Show(hdc);

	// лінії між квадратиками
	Set(l, t + d, l + d, t);     Line::Show(hdc);
	Set(r - d, t + d, r, t);     Line::Show(hdc);
	Set(l, b, l + d, b - d);     Line::Show(hdc);
	Set(r - d, b, r, b - d);     Line::Show(hdc);

	// передній квадратик
	Set(l, t + d, r - d, b);
	Rect::Show(hdc);


	Set(xold1, yold1, xold2, yold2);
}

void Cyl::Show(HDC hdc)
{
	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;

	long l, t, r, b;
	NormalizeCoords(l, t, r, b);

	long h = b - t;
	long ry = h / 4;
	if (ry < 4 && h >= 8) ry = 4;

	// нижній кружок
	Set(l, b - ry, r, b);
	Elips::Show(hdc);

	// лінії між кружками
	Set(l, t + ry / 2, l, b - ry / 2);
	Line::Show(hdc);
	
	// верхній кружок
	Set(l, t, r, t + ry);
	Elips::Show(hdc);


	Set(r, t + ry / 2, r, b - ry / 2);
	Line::Show(hdc);

	Set(xold1, yold1, xold2, yold2);
}

// Текст

void Text::Show(HDC hdc)
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);

	if (rubber)
	{
		// під час розтягування мишею показує пунктирну рамку області в якій текст
		HPEN hPen = CreateBorderPen();
		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

		Rectangle(hdc, l, t, r, b);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		return;
	}

	if (text.empty()) return;

	int fontHeight = 16 + borderWidth * 4;
	HFONT hFont = CreateFontW(
		fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
	);

	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	COLORREF oldTextColor = SetTextColor(hdc, borderColor);
	int oldBkMode = SetBkMode(hdc, hasFill ? OPAQUE : TRANSPARENT);
	COLORREF oldBkColor = hasFill ? SetBkColor(hdc, fillColor) : 0;

	if (r - l < 10 || b - t < 10)
	{
		TextOutW(hdc, l, t, text.c_str(), (int)text.length());
	}
	else
	{
		RECT rc = { l, t, r, b };
		DrawTextW(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
	}

	if (hasFill) SetBkColor(hdc, oldBkColor);
	SetBkMode(hdc, oldBkMode);
	SetTextColor(hdc, oldTextColor);
	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

void Text::Serialize(FILE* fout) const
{
	if (!fout) return;

	// перетворює переноси рядків (\n міняє на \\n), щоб один об'єкт завжди займав один рядок у файлі
	std::wstring escaped;
	escaped.reserve(text.size());
	for (wchar_t ch : text)
	{
		if (ch == L'\n') escaped += L"\\n";
		else if (ch != L'\r') escaped += ch;
	}

	fwprintf(fout, L"%s %ld %ld %ld %ld %lu %lu %d %d %s\n",
		GetTypeName(), x1, y1, x2, y2,
		(unsigned long)borderColor, (unsigned long)fillColor,
		hasFill ? 1 : 0, borderWidth, escaped.c_str());
}

bool Text::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int filled = 0, width = 1;
	int offset = 0;

	if (swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d%n",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &filled, &width, &offset) < 8)
	{
		return false;
	}

	borderColor = (COLORREF)bCol;
	fillColor = (COLORREF)fCol;
	hasFill = (filled != 0);
	borderWidth = (width > 0) ? width : 1;

	const WCHAR* pText = params + offset;
	if (*pText == L' ') pText++;

	text.clear();
	while (*pText && *pText != L'\r' && *pText != L'\n')
	{
		if (*pText == L'\\' && *(pText + 1) == L'n')
		{
			text += L'\n';
			pText += 2;
		}
		else
		{
			text += *pText;
			pText++;
		}
	}
	return true;
}
