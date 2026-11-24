#include "pch.h"
#include "framework.h"
#include "shape.h"
#include <cmath>

constexpr double PI = 3.14159265358979323846;

static std::wstring XmlEscape(const std::wstring& s)
{
	std::wstring out;
	out.reserve(s.size());
	for (wchar_t ch : s)
	{
		switch (ch)
		{
		case L'&': out += L"&amp;";  break;
		case L'<': out += L"&lt;";   break;
		case L'>': out += L"&gt;";   break;
		case L'"': out += L"&quot;"; break;
		default:   out += ch;
		}
	}
	return out;
}

static void WriteSvgColorAttr(FILE* fout, const WCHAR* attrName, COLORREF color, bool enabled)
{
	if (!fout) return;
	if (!enabled)
	{
		fwprintf(fout, L" %s=\"none\"", attrName);
	}
	else
	{
		fwprintf(fout, L" %s=\"rgb(%d,%d,%d)\"",
			attrName,
			(int)GetRValue(color),
			(int)GetGValue(color),
			(int)GetBValue(color));
	}
}

static void WriteSvgPointsAttr(FILE* fout, const POINT* pts, int count)
{
	if (!fout || !pts || count <= 0) return;
	fwprintf(fout, L" points=\"");
	for (int i = 0; i < count; i++)
	{
		if (i > 0) fwprintf(fout, L" ");
		fwprintf(fout, L"%ld,%ld", pts[i].x, pts[i].y);
	}
	fwprintf(fout, L"\"");
}

// Shape

void Shape::Set(long xstart, long ystart, long xend, long yend)
{
	x1 = xstart;
	y1 = ystart;
	x2 = xend;
	y2 = yend;
}

void Shape::MoveBy(long dx, long dy)
{
	x1 += dx;
	y1 += dy;
	x2 += dx;
	y2 += dy;
}

void Shape::GetBounds(long& l, long& t, long& r, long& b) const
{
	NormalizeCoords(l, t, r, b);
}

bool Shape::HitTest(long sx, long sy, long tolerance) const
{
	long l, t, r, b;
	GetBounds(l, t, r, b);
	return (sx >= l - tolerance && sx <= r + tolerance &&
		sy >= t - tolerance && sy <= b + tolerance);
}

int Shape::GetHandlePoints(POINT* pts) const
{
	switch (GetResizeMode())
	{
	case ResizeMode::Endpoints:
		pts[0] = { x1, y1 };
		pts[1] = { x2, y2 };
		return 2;

	case ResizeMode::Box:
	{
		long l, t, r, b;
		GetBounds(l, t, r, b);
		l -= SELECTION_MARGIN; t -= SELECTION_MARGIN;
		r += SELECTION_MARGIN; b += SELECTION_MARGIN;
		long cx = (l + r) / 2, cy = (t + b) / 2;
		// 0 TL, 1 T, 2 TR, 3 R, 4 BR, 5 B, 6 BL, 7 L
		pts[0] = { l, t };  pts[1] = { cx, t };  pts[2] = { r, t };
		pts[3] = { r, cy }; pts[4] = { r, b };   pts[5] = { cx, b };
		pts[6] = { l, b };  pts[7] = { l, cy };
		return 8;
	}

	default:
		return 0;
	}
}

int Shape::HitHandle(long sx, long sy) const
{
	POINT pts[8];
	int n = GetHandlePoints(pts);
	for (int i = n - 1; i >= 0; i--)
	{
		if (labs(sx - pts[i].x) <= HANDLE_SIZE + 2 && labs(sy - pts[i].y) <= HANDLE_SIZE + 2)
			return i;
	}
	return -1;
}

void Shape::ResizeFrom(const Shape& orig, int handle, long mx, long my)
{
	switch (GetResizeMode())
	{
	case ResizeMode::Endpoints:
		x1 = orig.x1; y1 = orig.y1; x2 = orig.x2; y2 = orig.y2;
		if (handle == 0) { x1 = mx; y1 = my; }
		else if (handle == 1) { x2 = mx; y2 = my; }
		break;

	case ResizeMode::Box:
	{
		long l, t, r, b;
		orig.GetBounds(l, t, r, b);

		const bool mL = (handle == 0 || handle == 6 || handle == 7);
		const bool mR = (handle == 2 || handle == 3 || handle == 4);
		const bool mT = (handle == 0 || handle == 1 || handle == 2);
		const bool mB = (handle == 4 || handle == 5 || handle == 6);

		long nl = l, nt = t, nr = r, nb = b;
		if (mL) nl = mx + SELECTION_MARGIN;
		if (mR) nr = mx - SELECTION_MARGIN;
		if (mT) nt = my + SELECTION_MARGIN;
		if (mB) nb = my - SELECTION_MARGIN;

		if (nr - nl < MIN_SHAPE_SIZE) { if (mL) nl = nr - MIN_SHAPE_SIZE; else nr = nl + MIN_SHAPE_SIZE; }
		if (nb - nt < MIN_SHAPE_SIZE) { if (mT) nt = nb - MIN_SHAPE_SIZE; else nb = nt + MIN_SHAPE_SIZE; }

		const double sx = (r > l) ? (double)(nr - nl) / (r - l) : 1.0;
		const double sy = (b > t) ? (double)(nb - nt) / (b - t) : 1.0;

		x1 = nl + std::lround((orig.x1 - l) * sx);
		y1 = nt + std::lround((orig.y1 - t) * sy);
		x2 = nl + std::lround((orig.x2 - l) * sx);
		y2 = nt + std::lround((orig.y2 - t) * sy);
		break;
	}

	default:
		break;
	}
}

void Shape::SetStyle(COLORREF border, COLORREF fill, bool borderEnabled, bool filled, int width)
{
	borderColor = border;
	fillColor = fill;
	hasBorder = borderEnabled;
	hasFill = filled;
	borderWidth = (width > 0) ? width : 1;
}

HPEN Shape::CreateBorderPen() const
{
	if (rubber)
		return CreatePen(PS_DOT, 1, RGB(0, 0, 0));

	if (!hasBorder)
		return (HPEN)GetStockObject(NULL_PEN);

	return CreatePen(PS_SOLID, borderWidth, borderColor);
}

HBRUSH Shape::CreateFillBrush(bool& createdBrush) const
{
	createdBrush = false;
	if (!hasFill || rubber)
		return (HBRUSH)GetStockObject(NULL_BRUSH);

	createdBrush = true;
	return CreateSolidBrush(fillColor);
}

void Shape::NormalizeCoords(long& l, long& t, long& r, long& b) const
{
	l = (x1 < x2) ? x1 : x2;
	r = (x1 < x2) ? x2 : x1;
	t = (y1 < y2) ? y1 : y2;
	b = (y1 < y2) ? y2 : y1;
}

void Shape::DrawSelection(HDC hdc) const
{
	if (!selected || rubber) return;

	long l, t, r, b;
	GetBounds(l, t, r, b);

	HPEN hSelPen = CreatePen(PS_DASH, 1, RGB(0, 120, 215));
	HPEN hOldPen = (HPEN)SelectObject(hdc, hSelPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, l - SELECTION_MARGIN, t - SELECTION_MARGIN,
		r + SELECTION_MARGIN, b + SELECTION_MARGIN);

	POINT pts[8];
	int n = GetHandlePoints(pts);
	if (n > 0)
	{
		HPEN hHandlePen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
		SelectObject(hdc, hHandlePen);
		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		for (int i = 0; i < n; i++)
			Rectangle(hdc, pts[i].x - HANDLE_SIZE, pts[i].y - HANDLE_SIZE,
				pts[i].x + HANDLE_SIZE + 1, pts[i].y + HANDLE_SIZE + 1);
		SelectObject(hdc, hSelPen);
		DeleteObject(hHandlePen);
	}

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hSelPen);
}

void Shape::Serialize(FILE* fout) const
{
	if (!fout) return;
	fwprintf(fout, L"%s %ld %ld %ld %ld %lu %lu %d %d %d\n",
		GetTypeName(), x1, y1, x2, y2,
		(unsigned long)borderColor, (unsigned long)fillColor,
		hasBorder ? 1 : 0, hasFill ? 1 : 0, borderWidth);
}

bool Shape::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int borderFlag = 1, filled = 0, width = 1;

	int parsed = swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d %d",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &borderFlag, &filled, &width);

	if (parsed >= 9)
	{
		borderColor = (COLORREF)bCol;
		fillColor = (COLORREF)fCol;
		hasBorder = (borderFlag != 0);
		hasFill = (filled != 0);
		borderWidth = (width > 0) ? width : 1;
		return true;
	}

	return false;
}

void Shape::SerializeSvg(FILE* fout) const
{
	UNREFERENCED_PARAMETER(fout);
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

bool Line::HitTest(long sx, long sy, long tol) const
{
	double dx = (double)(x2 - x1), dy = (double)(y2 - y1);
	double len2 = dx * dx + dy * dy;
	double t = (len2 > 0.0) ? ((sx - x1) * dx + (sy - y1) * dy) / len2 : 0.0;
	if (t < 0.0) t = 0.0;
	if (t > 1.0) t = 1.0;
	double dist = std::hypot(sx - (x1 + t * dx), sy - (y1 + t * dy));
	return dist <= tol + borderWidth / 2.0;
}

void Line::Show(HDC hdc)
{
	HPEN hPen = CreateBorderPen();
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);

	SelectObject(hdc, hOldPen);
	if (rubber || hasBorder) DeleteObject(hPen);

	DrawSelection(hdc);
}

void Line::SerializeSvg(FILE* fout) const
{
	if (!fout || !hasBorder) return;
	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x1, y1, x2, y2);
	WriteSvgColorAttr(fout, L"stroke", borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
}

void Rect::Show(HDC hdc)
{
	bool createdBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = CreateFillBrush(createdBrush);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	Rectangle(hdc, x1, y1, x2, y2);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (createdBrush) DeleteObject(hBrush);
	if (rubber || hasBorder) DeleteObject(hPen);

	DrawSelection(hdc);
}

void Rect::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\"", l, t, r - l, b - t);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

void Elips::Show(HDC hdc)
{
	bool createdBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = CreateFillBrush(createdBrush);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	Ellipse(hdc, x1, y1, x2, y2);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (createdBrush) DeleteObject(hBrush);
	if (rubber || hasBorder) DeleteObject(hPen);

	DrawSelection(hdc);
}

void Elips::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	double cx = (l + r) / 2.0;
	double cy = (t + b) / 2.0;
	double rx = (r - l) / 2.0;
	double ry = (b - t) / 2.0;

	fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\"", cx, cy, rx, ry);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

// Багатокутники

void Poly::GetBounds(long& l, long& t, long& r, long& b) const
{
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count <= 0)
	{
		NormalizeCoords(l, t, r, b);
		return;
	}

	l = r = pts[0].x;
	t = b = pts[0].y;
	for (int i = 1; i < count; i++)
	{
		if (pts[i].x < l) l = pts[i].x;
		if (pts[i].x > r) r = pts[i].x;
		if (pts[i].y < t) t = pts[i].y;
		if (pts[i].y > b) b = pts[i].y;
	}
}

void Poly::Show(HDC hdc)
{
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count < 3) return;

	bool createdBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = CreateFillBrush(createdBrush);

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	Polygon(hdc, pts, count);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (createdBrush) DeleteObject(hBrush);
	if (rubber || hasBorder) DeleteObject(hPen);

	DrawSelection(hdc);
}

void Poly::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count < 3) return;

	fwprintf(fout, L"  <polygon");
	WriteSvgPointsAttr(fout, pts, count);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

void Trngl::CalcVertices(POINT* pts, int& count) const
{
	count = 3;
	pts[0] = { (x1 + x2) / 2, y1 };
	pts[1] = { x2, y2 };
	pts[2] = { x1, y2 };
}

void RightTrngl::CalcVertices(POINT* pts, int& count) const
{
	count = 3;
	pts[0] = { x1, y1 };
	pts[1] = { x2, y1 };
	pts[2] = { x1, y2 };
}

void Romb::CalcVertices(POINT* pts, int& count) const
{
	long rx = x2 - x1;
	long ry = y2 - y1;
	count = 4;
	pts[0] = { x1, y1 - ry };
	pts[1] = { x1 + rx, y1 };
	pts[2] = { x1, y1 + ry };
	pts[3] = { x1 - rx, y1 };
}

static void CalcRegularPolygon(long cx, long cy, long rx, long ry, int sides, POINT* pts)
{
	double startAngle = -PI / 2.0;
	for (int i = 0; i < sides; i++)
	{
		double angle = startAngle + (2.0 * PI * i) / sides;
		pts[i].x = std::lround((double)cx + (double)rx * std::cos(angle));
		pts[i].y = std::lround((double)cy + (double)ry * std::sin(angle));
	}
}

void Pent::CalcVertices(POINT* pts, int& count) const
{
	long rx = x2 - x1;
	long ry = y2 - y1;
	count = 5;
	CalcRegularPolygon(x1, y1, rx, ry, count, pts);
}

void Hex::CalcVertices(POINT* pts, int& count) const
{
	long rx = x2 - x1;
	long ry = y2 - y1;
	count = 6;
	CalcRegularPolygon(x1, y1, rx, ry, count, pts);
}

// Node, Arrow, Edge, Cube, Cyl

void Node::GetBounds(long& l, long& t, long& r, long& b) const
{
	l = x1 - NODE_RADIUS;
	t = y1 - NODE_RADIUS;
	r = x1 + NODE_RADIUS;
	b = y1 + NODE_RADIUS;
}

void Node::Show(HDC hdc)
{
	const long xold1 = x1, yold1 = y1, xold2 = x2, yold2 = y2;
	const bool oldSel = selected;
	selected = false;

	const long r = NODE_RADIUS;
	Set(xold1 - r, yold1 - r, xold1 + r, yold1 + r);
	Elips::Show(hdc);

	Set(xold1, yold1, xold2, yold2);
	selected = oldSel;
	DrawSelection(hdc);
}

void Node::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%d\"", x1, y1, NODE_RADIUS);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

void Arrow::Show(HDC hdc)
{
	const bool oldSel = selected;
	selected = false;
	Line::Show(hdc);

	double dx = (double)(x2 - x1), dy = (double)(y2 - y1);
	double len = std::hypot(dx, dy);
	if (len >= 1.0 && (hasBorder || rubber))
	{
		const long xold1 = x1, yold1 = y1, xold2 = x2, yold2 = y2;
		double angle = std::atan2(dy, dx);
		double wingLen = 12.0 + borderWidth * 2.0;
		if (wingLen > len * 0.6) wingLen = len * 0.6;
		constexpr double spread = 25.0 * PI / 180.0;

		Set(xold2, yold2,
			xold2 - std::lround(wingLen * std::cos(angle - spread)),
			yold2 - std::lround(wingLen * std::sin(angle - spread)));
		Line::Show(hdc);

		Set(xold2, yold2,
			xold2 - std::lround(wingLen * std::cos(angle + spread)),
			yold2 - std::lround(wingLen * std::sin(angle + spread)));
		Line::Show(hdc);

		Set(xold1, yold1, xold2, yold2);
	}

	selected = oldSel;
	DrawSelection(hdc);
}

void Arrow::SerializeSvg(FILE* fout) const
{
	if (!fout || !hasBorder) return;

	Line::SerializeSvg(fout);

	double dx = (double)(x2 - x1);
	double dy = (double)(y2 - y1);
	double len = std::hypot(dx, dy);
	if (len < 1.0) return;

	double angle = std::atan2(dy, dx);
	double wingLen = 12.0 + borderWidth * 2.0;
	if (wingLen > len * 0.6) wingLen = len * 0.6;
	constexpr double spread = 25.0 * PI / 180.0;

	long wx1 = x2 - std::lround(wingLen * std::cos(angle - spread));
	long wy1 = y2 - std::lround(wingLen * std::sin(angle - spread));
	long wx2 = x2 - std::lround(wingLen * std::cos(angle + spread));
	long wy2 = y2 - std::lround(wingLen * std::sin(angle + spread));

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x2, y2, wx1, wy1);
	WriteSvgColorAttr(fout, L"stroke", borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x2, y2, wx2, wy2);
	WriteSvgColorAttr(fout, L"stroke", borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
}

void Edge::Show(HDC hdc)
{
	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;
	bool oldSel = selected;
	selected = false;

	Line::Show(hdc);

	const long r = EDGE_RADIUS;

	Set(xold1 - r, yold1 - r, xold1 + r, yold1 + r);
	Elips::Show(hdc);

	Set(xold2 - r, yold2 - r, xold2 + r, yold2 + r);
	Elips::Show(hdc);

	Set(xold1, yold1, xold2, yold2);
	selected = oldSel;
	DrawSelection(hdc);
}

void Edge::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	Line::SerializeSvg(fout);

	const long r = EDGE_RADIUS;
	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", x1, y1, r);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);

	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", x2, y2, r);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

void Cube::Show(HDC hdc)
{
	const long ax = x1, ay = y1;
	const long bx = x2, by = y2;

	long dx = bx - ax;
	long dy = by - ay;
	long absW = (dx < 0) ? -dx : dx;
	long absH = (dy < 0) ? -dy : dy;
	long depth = ((absW < absH) ? absW : absH) / 3;
	long sx = (dx >= 0) ? depth : -depth;
	long sy = (dy >= 0) ? depth : -depth;

	if (hasFill && !rubber)
	{
		POINT hull[6] = {
			{ ax,      ay },
			{ bx - sx, ay },
			{ bx,      ay + sy },
			{ bx,      by },
			{ ax + sx, by },
			{ ax,      by - sy }
		};

		HPEN hFillPen = CreatePen(PS_SOLID, 1, fillColor);
		HBRUSH hFillBrush = CreateSolidBrush(fillColor);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hFillPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFillBrush);

		Polygon(hdc, hull, 6);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hFillBrush);
		DeleteObject(hFillPen);
	}

	if (hasBorder || rubber)
	{
		bool oldFill = hasFill;
		bool oldSel = selected;
		hasFill = false;
		selected = false;

		// Задня грань
		Set(ax + sx, ay + sy, bx, by);
		Rect::Show(hdc);

		// Чотири ребра
		Set(ax, ay, ax + sx, ay + sy);      Line::Show(hdc);
		Set(bx - sx, ay, bx, ay + sy);      Line::Show(hdc);
		Set(bx - sx, by - sy, bx, by);      Line::Show(hdc);
		Set(ax, by - sy, ax + sx, by);      Line::Show(hdc);

		// Передня грань
		Set(ax, ay, bx - sx, by - sy);
		Rect::Show(hdc);

		hasFill = oldFill;
		selected = oldSel;
	}

	Set(ax, ay, bx, by);
	DrawSelection(hdc);
}

void Cube::SerializeSvg(FILE* fout) const
{
	if (!fout) return;

	long dx = x2 - x1;
	long dy = y2 - y1;
	long absW = (dx < 0) ? -dx : dx;
	long absH = (dy < 0) ? -dy : dy;
	long depth = ((absW < absH) ? absW : absH) / 3;
	long sx = (dx >= 0) ? depth : -depth;
	long sy = (dy >= 0) ? depth : -depth;

	if (hasFill)
	{
		POINT hull[6] = {
			{ x1, y1 },
			{ x2 - sx, y1 },
			{ x2, y1 + sy },
			{ x2, y2 },
			{ x1 + sx, y2 },
			{ x1, y2 - sy }
		};
		fwprintf(fout, L"  <polygon");
		WriteSvgPointsAttr(fout, hull, 6);
		fwprintf(fout, L" stroke=\"none\"");
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");
	}

	if (hasBorder)
	{
		long l1 = (x1 + sx < x2) ? (x1 + sx) : x2;
		long r1 = (x1 + sx < x2) ? x2 : (x1 + sx);
		long t1 = (y1 + sy < y2) ? (y1 + sy) : y2;
		long b1 = (y1 + sy < y2) ? y2 : (y1 + sy);

		long l2 = (x1 < x2 - sx) ? x1 : (x2 - sx);
		long r2 = (x1 < x2 - sx) ? (x2 - sx) : x1;
		long t2 = (y1 < y2 - sy) ? y1 : (y2 - sy);
		long b2 = (y1 < y2 - sy) ? (y2 - sy) : y1;

		fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\" fill=\"none\"", l1, t1, r1 - l1, b1 - t1);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\" fill=\"none\"", l2, t2, r2 - l2, b2 - t2);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		const POINT edges[4][2] = {
			{ { x1, y1 },{ x1 + sx, y1 + sy } },
			{ { x2 - sx, y1 },{ x2, y1 + sy } },
			{ { x2 - sx, y2 - sy },{ x2, y2 } },
			{ { x1, y2 - sy },{ x1 + sx, y2 } }
		};
		for (int i = 0; i < 4; i++)
		{
			fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"",
				edges[i][0].x, edges[i][0].y, edges[i][1].x, edges[i][1].y);
			WriteSvgColorAttr(fout, L"stroke", borderColor, true);
			fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
		}
	}
}

void Cyl::Show(HDC hdc)
{
	long xold1 = x1, yold1 = y1;
	long xold2 = x2, yold2 = y2;

	long l = (x1 < x2) ? x1 : x2;
	long r = (x1 < x2) ? x2 : x1;
	long dy = y2 - y1;
	long absH = (dy < 0) ? -dy : dy;

	long ry = absH / 4;
	if (ry > absH / 2) ry = absH / 2;
	if (ry == 0 && absH > 0) ry = 1;

	long cap1T, cap1B, cap2T, cap2B;
	if (dy >= 0)
	{
		cap1T = y1;      cap1B = y1 + ry;
		cap2T = y2 - ry; cap2B = y2;
	}
	else
	{
		cap1T = y1 - ry; cap1B = y1;
		cap2T = y2;      cap2B = y2 + ry;
	}

	long c1Y = (cap1T + cap1B) / 2;
	long c2Y = (cap2T + cap2B) / 2;
	long bodyT = (c1Y < c2Y) ? c1Y : c2Y;
	long bodyB = (c1Y < c2Y) ? c2Y : c1Y;

	if (hasFill && !rubber)
	{
		HPEN hFillPen = CreatePen(PS_SOLID, 1, fillColor);
		HBRUSH hFillBrush = CreateSolidBrush(fillColor);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hFillPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFillBrush);

		Rectangle(hdc, l, bodyT, r, bodyB + 1);
		Ellipse(hdc, l, cap1T, r, cap1B);
		Ellipse(hdc, l, cap2T, r, cap2B);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hFillBrush);
		DeleteObject(hFillPen);
	}

	if (hasBorder || rubber)
	{
		bool oldFill = hasFill;
		bool oldSel = selected;
		hasFill = false;
		selected = false;

		long rightEdge = (r > l) ? (r - 1) : r;

		Set(l, cap2T, r, cap2B);
		Elips::Show(hdc);

		Set(l, bodyT, l, bodyB);
		Line::Show(hdc);

		Set(rightEdge, bodyT, rightEdge, bodyB);
		Line::Show(hdc);

		Set(l, cap1T, r, cap1B);
		Elips::Show(hdc);

		hasFill = oldFill;
		selected = oldSel;
	}

	Set(xold1, yold1, xold2, yold2);
	DrawSelection(hdc);
}

void Cyl::SerializeSvg(FILE* fout) const
{
	if (!fout) return;

	long l = (x1 < x2) ? x1 : x2;
	long r = (x1 < x2) ? x2 : x1;
	long w = r - l;
	long dy = y2 - y1;
	long absH = (dy < 0) ? -dy : dy;

	long ry = absH / 4;
	if (ry > absH / 2) ry = absH / 2;
	if (ry == 0 && absH > 0) ry = 1;

	long cap1T, cap1B, cap2T, cap2B;
	if (dy >= 0)
	{
		cap1T = y1;      cap1B = y1 + ry;
		cap2T = y2 - ry; cap2B = y2;
	}
	else
	{
		cap1T = y1 - ry; cap1B = y1;
		cap2T = y2;      cap2B = y2 + ry;
	}

	long c1Y = (cap1T + cap1B) / 2;
	long c2Y = (cap2T + cap2B) / 2;
	long bodyT = (c1Y < c2Y) ? c1Y : c2Y;
	long bodyB = (c1Y < c2Y) ? c2Y : c1Y;

	if (hasFill)
	{
		fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\" stroke=\"none\"", l, bodyT, w, bodyB - bodyT);
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" stroke=\"none\"",
			(l + r) / 2.0, (cap1T + cap1B) / 2.0, w / 2.0, (cap1B - cap1T) / 2.0);
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" stroke=\"none\"",
			(l + r) / 2.0, (cap2T + cap2B) / 2.0, w / 2.0, (cap2B - cap2T) / 2.0);
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");
	}

	if (hasBorder)
	{
		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" fill=\"none\"",
			(l + r) / 2.0, (cap2T + cap2B) / 2.0, w / 2.0, (cap2B - cap2T) / 2.0);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", l, bodyT, l, bodyB);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", r, bodyT, r, bodyB);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" fill=\"none\"",
			(l + r) / 2.0, (cap1T + cap1B) / 2.0, w / 2.0, (cap1B - cap1T) / 2.0);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
	}
}

// Текст

void Text::Show(HDC hdc)
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);

	if (rubber)
	{
		HPEN hPen = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
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
	COLORREF oldTextColor = SetTextColor(hdc, hasBorder ? borderColor : RGB(0, 0, 0));
	int oldBkMode = SetBkMode(hdc, hasFill ? OPAQUE : TRANSPARENT);
	COLORREF oldBkColor = hasFill ? SetBkColor(hdc, fillColor) : 0;

	RECT rc = { l, t, r, b };
	UINT fmt = DT_LEFT | DT_TOP | DT_NOCLIP;
	if (r - l >= 10) fmt |= DT_WORDBREAK;
	DrawTextW(hdc, text.c_str(), -1, &rc, fmt);

	if (hasFill) SetBkColor(hdc, oldBkColor);
	SetBkMode(hdc, oldBkMode);
	SetTextColor(hdc, oldTextColor);
	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);

	DrawSelection(hdc);
}

void Text::Serialize(FILE* fout) const
{
	if (!fout) return;

	std::wstring escaped;
	escaped.reserve(text.size());
	for (wchar_t ch : text)
	{
		if (ch == L'\\') escaped += L"\\\\";
		else if (ch == L'\n') escaped += L"\\n";
		else if (ch != L'\r') escaped += ch;
	}

	fwprintf(fout, L"%s %ld %ld %ld %ld %lu %lu %d %d %d %s\n",
		GetTypeName(), x1, y1, x2, y2,
		(unsigned long)borderColor, (unsigned long)fillColor,
		hasBorder ? 1 : 0, hasFill ? 1 : 0, borderWidth, escaped.c_str());
}

bool Text::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int borderFlag = 1, filled = 0, width = 1;
	int offset = 0;

	if (swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d %d%n",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &borderFlag, &filled, &width, &offset) < 9)
	{
		return false;
	}

	borderColor = (COLORREF)bCol;
	fillColor = (COLORREF)fCol;
	hasBorder = (borderFlag != 0);
	hasFill = (filled != 0);
	borderWidth = (width > 0) ? width : 1;

	const WCHAR* pText = params + offset;
	if (*pText == L' ') pText++;

	text.clear();
	while (*pText && *pText != L'\r' && *pText != L'\n')
	{
		if (*pText == L'\\')
		{
			pText++;
			if (*pText == L'\\')
			{
				text += L'\\';
				pText++;
			}
			else if (*pText == L'n')
			{
				text += L'\n';
				pText++;
			}
			else if (*pText)
			{
				text += *pText;
				pText++;
			}
		}
		else
		{
			text += *pText;
			pText++;
		}
	}
	return true;
}

void Text::SerializeSvg(FILE* fout) const
{
	if (!fout || text.empty()) return;

	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	int fontSize = 16 + borderWidth * 4;

	fwprintf(fout, L"  <text x=\"%ld\" y=\"%ld\" font-family=\"Segoe UI, sans-serif\" font-size=\"%d\" dominant-baseline=\"hanging\"",
		l, t, fontSize);
	WriteSvgColorAttr(fout, L"fill", hasBorder ? borderColor : RGB(0, 0, 0), true);
	fwprintf(fout, L">");

	size_t start = 0;
	bool first = true;
	while (true)
	{
		size_t pos = text.find(L'\n', start);
		std::wstring line = text.substr(start, (pos == std::wstring::npos) ? std::wstring::npos : pos - start);
		fwprintf(fout, L"<tspan x=\"%ld\" dy=\"%s\">%s</tspan>", l, first ? L"0" : L"1.25em", XmlEscape(line).c_str());
		first = false;
		if (pos == std::wstring::npos) break;
		start = pos + 1;
	}
	fwprintf(fout, L"</text>\n");
}
