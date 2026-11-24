#include "pch.h"
#include "framework.h"
#include "shape.h"
#include <cmath>

constexpr double kPi = 3.14159265358979323846;

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
			static_cast<int>(GetRValue(color)),
			static_cast<int>(GetGValue(color)),
			static_cast<int>(GetBValue(color)));
	}
}

static void WriteSvgPointsAttr(FILE* fout, const POINT* pts, int count)
{
	if (!fout || !pts || count <= 0) return;
	fwprintf(fout, L" points=\"");
	for (int i = 0; i < count; ++i)
	{
		if (i > 0) fwprintf(fout, L" ");
		fwprintf(fout, L"%ld,%ld", pts[i].x, pts[i].y);
	}
	fwprintf(fout, L"\"");
}

// Shape

void Shape::Set(long xStart, long yStart, long xEnd, long yEnd)
{
	x1 = xStart;
	y1 = yStart;
	x2 = xEnd;
	y2 = yEnd;
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
		l -= kSelectionMargin;
		t -= kSelectionMargin;
		r += kSelectionMargin;
		b += kSelectionMargin;
		const long cx = (l + r) / 2;
		const long cy = (t + b) / 2;

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
	POINT pts[8] = {};
	const int n = GetHandlePoints(pts);
	for (int i = n - 1; i >= 0; --i)
	{
		if (labs(sx - pts[i].x) <= kHandleSize + 2 && labs(sy - pts[i].y) <= kHandleSize + 2)
			return i;
	}
	return -1;
}

void Shape::ResizeFrom(const Shape& orig, int handle, long mx, long my)
{
	switch (GetResizeMode())
	{
	case ResizeMode::Endpoints:
		x1 = orig.x1;
		y1 = orig.y1;
		x2 = orig.x2;
		y2 = orig.y2;
		if (handle == 0) { x1 = mx; y1 = my; }
		else if (handle == 1) { x2 = mx; y2 = my; }
		break;

	case ResizeMode::Box:
	{
		long l, t, r, b;
		orig.GetBounds(l, t, r, b);

		const bool moveLeft = (handle == 0 || handle == 6 || handle == 7);
		const bool moveRight = (handle == 2 || handle == 3 || handle == 4);
		const bool moveTop = (handle == 0 || handle == 1 || handle == 2);
		const bool moveBottom = (handle == 4 || handle == 5 || handle == 6);

		long nl = l, nt = t, nr = r, nb = b;
		if (moveLeft)   nl = mx + kSelectionMargin;
		if (moveRight)  nr = mx - kSelectionMargin;
		if (moveTop)    nt = my + kSelectionMargin;
		if (moveBottom) nb = my - kSelectionMargin;

		if (nr - nl < kMinShapeSize)
		{
			if (moveLeft) nl = nr - kMinShapeSize;
			else nr = nl + kMinShapeSize;
		}
		if (nb - nt < kMinShapeSize)
		{
			if (moveTop) nt = nb - kMinShapeSize;
			else nb = nt + kMinShapeSize;
		}

		const double sx = (r > l) ? static_cast<double>(nr - nl) / (r - l) : 1.0;
		const double sy = (b > t) ? static_cast<double>(nb - nt) / (b - t) : 1.0;

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
		return static_cast<HPEN>(GetStockObject(NULL_PEN));

	return CreatePen(PS_SOLID, borderWidth, borderColor);
}

HBRUSH Shape::CreateFillBrush(bool& createdBrush) const
{
	createdBrush = false;
	if (!hasFill || rubber)
		return static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

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

void Shape::DrawLinePrimitive(HDC hdc, long ax, long ay, long bx, long by) const
{
	HPEN hPen = CreateBorderPen();
	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));

	MoveToEx(hdc, ax, ay, nullptr);
	LineTo(hdc, bx, by);

	SelectObject(hdc, hOldPen);
	if (rubber || hasBorder) DeleteObject(hPen);
}

void Shape::DrawRectPrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const
{
	bool createdBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = allowFill
		? CreateFillBrush(createdBrush)
		: static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

	Rectangle(hdc, ax, ay, bx, by);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (createdBrush) DeleteObject(hBrush);
	if (rubber || hasBorder) DeleteObject(hPen);
}

void Shape::DrawEllipsePrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const
{
	bool createdBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = allowFill
		? CreateFillBrush(createdBrush)
		: static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

	Ellipse(hdc, ax, ay, bx, by);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (createdBrush) DeleteObject(hBrush);
	if (rubber || hasBorder) DeleteObject(hPen);
}

void Shape::DrawSelection(HDC hdc) const
{
	if (!selected || rubber) return;

	long l, t, r, b;
	GetBounds(l, t, r, b);

	HPEN hSelPen = CreatePen(PS_DASH, 1, RGB(0, 120, 215));
	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hSelPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

	Rectangle(hdc, l - kSelectionMargin, t - kSelectionMargin,
		r + kSelectionMargin, b + kSelectionMargin);

	POINT pts[8] = {};
	const int n = GetHandlePoints(pts);
	if (n > 0)
	{
		HPEN hHandlePen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
		SelectObject(hdc, hHandlePen);
		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		for (int i = 0; i < n; ++i)
		{
			Rectangle(hdc, pts[i].x - kHandleSize, pts[i].y - kHandleSize,
				pts[i].x + kHandleSize + 1, pts[i].y + kHandleSize + 1);
		}
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
		static_cast<unsigned long>(borderColor),
		static_cast<unsigned long>(fillColor),
		hasBorder ? 1 : 0, hasFill ? 1 : 0, borderWidth);
}

bool Shape::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int borderFlag = 1, filled = 0, width = 1;

	const int parsed = swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d %d",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &borderFlag, &filled, &width);

	if (parsed >= 9)
	{
		borderColor = static_cast<COLORREF>(bCol);
		fillColor = static_cast<COLORREF>(fCol);
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

// Базові примітиви

bool Line::HitTest(long sx, long sy, long tolerance) const
{
	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len2 = dx * dx + dy * dy;
	double t = (len2 > 0.0) ? ((sx - x1) * dx + (sy - y1) * dy) / len2 : 0.0;
	if (t < 0.0) t = 0.0;
	if (t > 1.0) t = 1.0;
	const double dist = std::hypot(sx - (x1 + t * dx), sy - (y1 + t * dy));
	return dist <= tolerance + borderWidth / 2.0;
}

void Line::Show(HDC hdc) const
{
	DrawLinePrimitive(hdc, x1, y1, x2, y2);
	DrawSelection(hdc);
}

void Line::SerializeSvg(FILE* fout) const
{
	if (!fout || !hasBorder) return;
	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x1, y1, x2, y2);
	WriteSvgColorAttr(fout, L"stroke", borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
}

void Rect::Show(HDC hdc) const
{
	DrawRectPrimitive(hdc, x1, y1, x2, y2, true);
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

void Elips::Show(HDC hdc) const
{
	DrawEllipsePrimitive(hdc, x1, y1, x2, y2, true);
	DrawSelection(hdc);
}

void Elips::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	const double cx = (l + r) / 2.0;
	const double cy = (t + b) / 2.0;
	const double rx = (r - l) / 2.0;
	const double ry = (b - t) / 2.0;

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
	for (int i = 1; i < count; ++i)
	{
		if (pts[i].x < l) l = pts[i].x;
		if (pts[i].x > r) r = pts[i].x;
		if (pts[i].y < t) t = pts[i].y;
		if (pts[i].y > b) b = pts[i].y;
	}
}

void Poly::Show(HDC hdc) const
{
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count < 3) return;

	bool createdBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = CreateFillBrush(createdBrush);

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

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
	const long rx = x2 - x1;
	const long ry = y2 - y1;
	count = 4;
	pts[0] = { x1, y1 - ry };
	pts[1] = { x1 + rx, y1 };
	pts[2] = { x1, y1 + ry };
	pts[3] = { x1 - rx, y1 };
}

static void CalcRegularPolygon(long cx, long cy, long rx, long ry, int sides, POINT* pts)
{
	constexpr double startAngle = -kPi / 2.0;
	for (int i = 0; i < sides; ++i)
	{
		const double angle = startAngle + (2.0 * kPi * i) / sides;
		pts[i].x = std::lround(static_cast<double>(cx) + static_cast<double>(rx) * std::cos(angle));
		pts[i].y = std::lround(static_cast<double>(cy) + static_cast<double>(ry) * std::sin(angle));
	}
}

void Pent::CalcVertices(POINT* pts, int& count) const
{
	count = 5;
	CalcRegularPolygon(x1, y1, x2 - x1, y2 - y1, count, pts);
}

void Hex::CalcVertices(POINT* pts, int& count) const
{
	count = 6;
	CalcRegularPolygon(x1, y1, x2 - x1, y2 - y1, count, pts);
}

// Складні фігури

void Node::GetBounds(long& l, long& t, long& r, long& b) const
{
	l = x1 - kNodeRadius;
	t = y1 - kNodeRadius;
	r = x1 + kNodeRadius;
	b = y1 + kNodeRadius;
}

void Node::Show(HDC hdc) const
{
	const long r = kNodeRadius;
	DrawEllipsePrimitive(hdc, x1 - r, y1 - r, x1 + r, y1 + r, true);
	DrawSelection(hdc);
}

void Node::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", x1, y1, kNodeRadius);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

void Arrow::Show(HDC hdc) const
{
	DrawLinePrimitive(hdc, x1, y1, x2, y2);

	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len = std::hypot(dx, dy);
	if (len >= 1.0 && (hasBorder || rubber))
	{
		const double angle = std::atan2(dy, dx);
		double wingLen = 12.0 + borderWidth * 2.0;
		if (wingLen > len * 0.6) wingLen = len * 0.6;
		constexpr double spread = 25.0 * kPi / 180.0;

		DrawLinePrimitive(hdc, x2, y2,
			x2 - std::lround(wingLen * std::cos(angle - spread)),
			y2 - std::lround(wingLen * std::sin(angle - spread)));

		DrawLinePrimitive(hdc, x2, y2,
			x2 - std::lround(wingLen * std::cos(angle + spread)),
			y2 - std::lround(wingLen * std::sin(angle + spread)));
	}

	DrawSelection(hdc);
}

void Arrow::SerializeSvg(FILE* fout) const
{
	if (!fout || !hasBorder) return;

	Line::SerializeSvg(fout);

	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len = std::hypot(dx, dy);
	if (len < 1.0) return;

	const double angle = std::atan2(dy, dx);
	double wingLen = 12.0 + borderWidth * 2.0;
	if (wingLen > len * 0.6) wingLen = len * 0.6;
	constexpr double spread = 25.0 * kPi / 180.0;

	const long wx1 = x2 - std::lround(wingLen * std::cos(angle - spread));
	const long wy1 = y2 - std::lround(wingLen * std::sin(angle - spread));
	const long wx2 = x2 - std::lround(wingLen * std::cos(angle + spread));
	const long wy2 = y2 - std::lround(wingLen * std::sin(angle + spread));

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x2, y2, wx1, wy1);
	WriteSvgColorAttr(fout, L"stroke", borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x2, y2, wx2, wy2);
	WriteSvgColorAttr(fout, L"stroke", borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
}

void Edge::Show(HDC hdc) const
{
	DrawLinePrimitive(hdc, x1, y1, x2, y2);

	const long r = kEdgeRadius;
	DrawEllipsePrimitive(hdc, x1 - r, y1 - r, x1 + r, y1 + r, true);
	DrawEllipsePrimitive(hdc, x2 - r, y2 - r, x2 + r, y2 + r, true);

	DrawSelection(hdc);
}

void Edge::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	Line::SerializeSvg(fout);

	const long r = kEdgeRadius;
	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", x1, y1, r);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);

	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", x2, y2, r);
	WriteSvgColorAttr(fout, L"stroke", borderColor, hasBorder);
	WriteSvgColorAttr(fout, L"fill", fillColor, hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", hasBorder ? borderWidth : 0);
}

Cube::CubeLayout Cube::CalcLayout() const
{
	const long dx = x2 - x1;
	const long dy = y2 - y1;
	const long absW = (dx < 0) ? -dx : dx;
	const long absH = (dy < 0) ? -dy : dy;
	const long depth = ((absW < absH) ? absW : absH) / 3;
	const long sx = (dx >= 0) ? depth : -depth;
	const long sy = (dy >= 0) ? depth : -depth;

	CubeLayout layout = {};
	layout.hull[0] = { x1, y1 };
	layout.hull[1] = { x2 - sx, y1 };
	layout.hull[2] = { x2, y1 + sy };
	layout.hull[3] = { x2, y2 };
	layout.hull[4] = { x1 + sx, y2 };
	layout.hull[5] = { x1, y2 - sy };

	layout.backRect = { x1 + sx, y1 + sy, x2, y2 };
	layout.frontRect = { x1, y1, x2 - sx, y2 - sy };

	layout.edges[0][0] = { x1, y1 };           layout.edges[0][1] = { x1 + sx, y1 + sy };
	layout.edges[1][0] = { x2 - sx, y1 };      layout.edges[1][1] = { x2, y1 + sy };
	layout.edges[2][0] = { x2 - sx, y2 - sy }; layout.edges[2][1] = { x2, y2 };
	layout.edges[3][0] = { x1, y2 - sy };      layout.edges[3][1] = { x1 + sx, y2 };

	return layout;
}

void Cube::Show(HDC hdc) const
{
	const CubeLayout layout = CalcLayout();

	if (hasFill && !rubber)
	{
		HPEN hFillPen = CreatePen(PS_SOLID, 1, fillColor);
		HBRUSH hFillBrush = CreateSolidBrush(fillColor);
		HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hFillPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hFillBrush));

		Polygon(hdc, layout.hull, 6);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hFillBrush);
		DeleteObject(hFillPen);
	}

	if (hasBorder || rubber)
	{
		DrawRectPrimitive(hdc, layout.backRect.left, layout.backRect.top,
			layout.backRect.right, layout.backRect.bottom, false);

		for (const auto& edge : layout.edges)
			DrawLinePrimitive(hdc, edge[0].x, edge[0].y, edge[1].x, edge[1].y);

		DrawRectPrimitive(hdc, layout.frontRect.left, layout.frontRect.top,
			layout.frontRect.right, layout.frontRect.bottom, false);
	}

	DrawSelection(hdc);
}

void Cube::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	const CubeLayout layout = CalcLayout();

	if (hasFill)
	{
		fwprintf(fout, L"  <polygon");
		WriteSvgPointsAttr(fout, layout.hull, 6);
		fwprintf(fout, L" stroke=\"none\"");
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");
	}

	if (hasBorder)
	{
		auto writeSvgRect = [&](const RECT& rc) {
			const long l = (rc.left < rc.right) ? rc.left : rc.right;
			const long r = (rc.left < rc.right) ? rc.right : rc.left;
			const long t = (rc.top < rc.bottom) ? rc.top : rc.bottom;
			const long b = (rc.top < rc.bottom) ? rc.bottom : rc.top;

			fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\" fill=\"none\"",
				l, t, r - l, b - t);
			WriteSvgColorAttr(fout, L"stroke", borderColor, true);
			fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
			};

		writeSvgRect(layout.backRect);
		writeSvgRect(layout.frontRect);

		for (const auto& edge : layout.edges)
		{
			fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"",
				edge[0].x, edge[0].y, edge[1].x, edge[1].y);
			WriteSvgColorAttr(fout, L"stroke", borderColor, true);
			fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
		}
	}
}

Cyl::CylLayout Cyl::CalcLayout() const
{
	CylLayout g = {};
	g.left = (x1 < x2) ? x1 : x2;
	g.right = (x1 < x2) ? x2 : x1;

	const long dy = y2 - y1;
	const long absH = (dy < 0) ? -dy : dy;

	long ry = absH / 4;
	if (ry > absH / 2) ry = absH / 2;
	if (ry == 0 && absH > 0) ry = 1;

	if (dy >= 0)
	{
		g.cap1Top = y1;      g.cap1Bottom = y1 + ry;
		g.cap2Top = y2 - ry; g.cap2Bottom = y2;
	}
	else
	{
		g.cap1Top = y1 - ry; g.cap1Bottom = y1;
		g.cap2Top = y2;      g.cap2Bottom = y2 + ry;
	}

	const long c1Y = (g.cap1Top + g.cap1Bottom) / 2;
	const long c2Y = (g.cap2Top + g.cap2Bottom) / 2;
	g.bodyTop = (c1Y < c2Y) ? c1Y : c2Y;
	g.bodyBottom = (c1Y < c2Y) ? c2Y : c1Y;

	return g;
}

void Cyl::Show(HDC hdc) const
{
	const CylLayout g = CalcLayout();

	if (hasFill && !rubber)
	{
		HPEN hFillPen = CreatePen(PS_SOLID, 1, fillColor);
		HBRUSH hFillBrush = CreateSolidBrush(fillColor);
		HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hFillPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hFillBrush));

		Rectangle(hdc, g.left, g.bodyTop, g.right, g.bodyBottom + 1);
		Ellipse(hdc, g.left, g.cap1Top, g.right, g.cap1Bottom);
		Ellipse(hdc, g.left, g.cap2Top, g.right, g.cap2Bottom);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hFillBrush);
		DeleteObject(hFillPen);
	}

	if (hasBorder || rubber)
	{
		const long rightEdge = (g.right > g.left) ? (g.right - 1) : g.right;

		DrawEllipsePrimitive(hdc, g.left, g.cap2Top, g.right, g.cap2Bottom, false);
		DrawLinePrimitive(hdc, g.left, g.bodyTop, g.left, g.bodyBottom);
		DrawLinePrimitive(hdc, rightEdge, g.bodyTop, rightEdge, g.bodyBottom);
		DrawEllipsePrimitive(hdc, g.left, g.cap1Top, g.right, g.cap1Bottom, false);
	}

	DrawSelection(hdc);
}

void Cyl::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	const CylLayout g = CalcLayout();
	const long width = g.right - g.left;
	const double cx = (g.left + g.right) / 2.0;
	const double rx = width / 2.0;

	if (hasFill)
	{
		fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\" stroke=\"none\"",
			g.left, g.bodyTop, width, g.bodyBottom - g.bodyTop);
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" stroke=\"none\"",
			cx, (g.cap1Top + g.cap1Bottom) / 2.0, rx, (g.cap1Bottom - g.cap1Top) / 2.0);
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" stroke=\"none\"",
			cx, (g.cap2Top + g.cap2Bottom) / 2.0, rx, (g.cap2Bottom - g.cap2Top) / 2.0);
		WriteSvgColorAttr(fout, L"fill", fillColor, true);
		fwprintf(fout, L" />\n");
	}

	if (hasBorder)
	{
		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" fill=\"none\"",
			cx, (g.cap2Top + g.cap2Bottom) / 2.0, rx, (g.cap2Bottom - g.cap2Top) / 2.0);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"",
			g.left, g.bodyTop, g.left, g.bodyBottom);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"",
			g.right, g.bodyTop, g.right, g.bodyBottom);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" fill=\"none\"",
			cx, (g.cap1Top + g.cap1Bottom) / 2.0, rx, (g.cap1Bottom - g.cap1Top) / 2.0);
		WriteSvgColorAttr(fout, L"stroke", borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", borderWidth);
	}
}

// Текст

void Text::Show(HDC hdc) const
{
	long l, t, r, b;
	NormalizeCoords(l, t, r, b);

	if (rubber)
	{
		DrawRectPrimitive(hdc, l, t, r, b, false);
		return;
	}

	if (text.empty()) return;

	const int fontHeight = 16 + borderWidth * 4;
	HFONT hFont = CreateFontW(
		fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
	);

	HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
	const COLORREF oldTextColor = SetTextColor(hdc, hasBorder ? borderColor : RGB(0, 0, 0));
	const int oldBkMode = SetBkMode(hdc, hasFill ? OPAQUE : TRANSPARENT);
	const COLORREF oldBkColor = hasFill ? SetBkColor(hdc, fillColor) : 0;

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
		static_cast<unsigned long>(borderColor),
		static_cast<unsigned long>(fillColor),
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

	borderColor = static_cast<COLORREF>(bCol);
	fillColor = static_cast<COLORREF>(fCol);
	hasBorder = (borderFlag != 0);
	hasFill = (filled != 0);
	borderWidth = (width > 0) ? width : 1;

	const WCHAR* pText = params + offset;
	if (*pText == L' ') ++pText;

	text.clear();
	while (*pText && *pText != L'\r' && *pText != L'\n')
	{
		if (*pText == L'\\')
		{
			++pText;
			if (*pText == L'\\')
			{
				text += L'\\';
				++pText;
			}
			else if (*pText == L'n')
			{
				text += L'\n';
				++pText;
			}
			else if (*pText)
			{
				text += *pText;
				++pText;
			}
		}
		else
		{
			text += *pText;
			++pText;
		}
	}
	return true;
}

void Text::SerializeSvg(FILE* fout) const
{
	if (!fout || text.empty()) return;

	long l, t, r, b;
	NormalizeCoords(l, t, r, b);
	const int fontSize = 16 + borderWidth * 4;

	fwprintf(fout, L"  <text x=\"%ld\" y=\"%ld\" font-family=\"Segoe UI, sans-serif\" font-size=\"%d\" dominant-baseline=\"hanging\"",
		l, t, fontSize);
	WriteSvgColorAttr(fout, L"fill", hasBorder ? borderColor : RGB(0, 0, 0), true);
	fwprintf(fout, L">");

	size_t start = 0;
	bool first = true;
	while (true)
	{
		const size_t pos = text.find(L'\n', start);
		const std::wstring line = text.substr(start, (pos == std::wstring::npos) ? std::wstring::npos : pos - start);
		fwprintf(fout, L"<tspan x=\"%ld\" dy=\"%s\">%s</tspan>",
			l, first ? L"0" : L"1.25em", XmlEscape(line).c_str());
		first = false;
		if (pos == std::wstring::npos) break;
		start = pos + 1;
	}
	fwprintf(fout, L"</text>\n");
}
