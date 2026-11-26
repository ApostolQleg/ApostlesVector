#include "pch.h"
#include "shapes_complex.h"

// Node

void Node::GetLocalBounds(long& l, long& t, long& r, long& b) const
{
	l = x1 - kNodeRadius;
	t = y1 - kNodeRadius;
	r = x1 + kNodeRadius;
	b = y1 + kNodeRadius;
}

void Node::DrawGeometry(HDC hdc) const
{
	DrawEllipsePrimitive(hdc, x1 - kNodeRadius, y1 - kNodeRadius,
		x1 + kNodeRadius, y1 + kNodeRadius, true);
}

void Node::SerializeSvgBody(FILE* fout) const
{
	fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", x1, y1, kNodeRadius);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, style.hasBorder);
	WriteSvgColorAttr(fout, L"fill", style.fillColor, style.hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.hasBorder ? style.borderWidth : 0);
}

// Arrow

void Arrow::DrawGeometry(HDC hdc) const
{
	DrawLinePrimitive(hdc, x1, y1, x2, y2);

	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len = std::hypot(dx, dy);
	if (len < 1.0 || !style.hasBorder) return;

	const double angle = std::atan2(dy, dx);
	const double wingLen = std::min(12.0 + style.borderWidth * 2.0, len * 0.6);
	constexpr double spread = 25.0 * kPi / 180.0;

	DrawLinePrimitive(hdc, x2, y2,
		x2 - std::lround(wingLen * std::cos(angle - spread)),
		y2 - std::lround(wingLen * std::sin(angle - spread)));
	DrawLinePrimitive(hdc, x2, y2,
		x2 - std::lround(wingLen * std::cos(angle + spread)),
		y2 - std::lround(wingLen * std::sin(angle + spread)));
}

void Arrow::SerializeSvgBody(FILE* fout) const
{
	if (!style.hasBorder) return;
	Line::SerializeSvgBody(fout);

	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len = std::hypot(dx, dy);
	if (len < 1.0) return;

	const double angle = std::atan2(dy, dx);
	const double wingLen = std::min(12.0 + style.borderWidth * 2.0, len * 0.6);
	constexpr double spread = 25.0 * kPi / 180.0;

	const long wx1 = x2 - std::lround(wingLen * std::cos(angle - spread));
	const long wy1 = y2 - std::lround(wingLen * std::sin(angle - spread));
	const long wx2 = x2 - std::lround(wingLen * std::cos(angle + spread));
	const long wy2 = y2 - std::lround(wingLen * std::sin(angle + spread));

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x2, y2, wx1, wy1);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x2, y2, wx2, wy2);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
}

// Edge (повноцінні кружечки Node + лінія від краю до краю кіл)

bool Edge::CalcClippedSegment(long& ax, long& ay, long& bx, long& by) const
{
	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len = std::hypot(dx, dy);
	if (len <= kNodeRadius * 2.0)
		return false;

	const double ux = dx / len;
	const double uy = dy / len;
	ax = x1 + std::lround(ux * kNodeRadius);
	ay = y1 + std::lround(uy * kNodeRadius);
	bx = x2 - std::lround(ux * kNodeRadius);
	by = y2 - std::lround(uy * kNodeRadius);
	return true;
}

void Edge::GetLocalBounds(long& l, long& t, long& r, long& b) const
{
	NormalizeCoords(l, t, r, b);
	l -= kNodeRadius;
	t -= kNodeRadius;
	r += kNodeRadius;
	b += kNodeRadius;
}

bool Edge::HitTestLocal(long lx, long ly, long tolerance) const
{
	const double hitR = static_cast<double>(kNodeRadius + tolerance);
	if (std::hypot(static_cast<double>(lx - x1), static_cast<double>(ly - y1)) <= hitR)
		return true;
	if (std::hypot(static_cast<double>(lx - x2), static_cast<double>(ly - y2)) <= hitR)
		return true;
	return Line::HitTestLocal(lx, ly, tolerance);
}

void Edge::DrawGeometry(HDC hdc) const
{
	long ax = 0, ay = 0, bx = 0, by = 0;
	if (CalcClippedSegment(ax, ay, bx, by))
		DrawLinePrimitive(hdc, ax, ay, bx, by);

	DrawEllipsePrimitive(hdc, x1 - kNodeRadius, y1 - kNodeRadius,
		x1 + kNodeRadius, y1 + kNodeRadius, true);
	DrawEllipsePrimitive(hdc, x2 - kNodeRadius, y2 - kNodeRadius,
		x2 + kNodeRadius, y2 + kNodeRadius, true);
}

void Edge::SerializeSvgBody(FILE* fout) const
{
	long ax = 0, ay = 0, bx = 0, by = 0;
	if (style.hasBorder && CalcClippedSegment(ax, ay, bx, by))
	{
		fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", ax, ay, bx, by);
		WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
	}

	const POINT centers[2] = { { x1, y1 },{ x2, y2 } };
	for (const auto& pt : centers)
	{
		fwprintf(fout, L"  <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\"", pt.x, pt.y, kNodeRadius);
		WriteSvgColorAttr(fout, L"stroke", style.borderColor, style.hasBorder);
		WriteSvgColorAttr(fout, L"fill", style.fillColor, style.hasFill);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", style.hasBorder ? style.borderWidth : 0);
	}
}

// Cube (каркас без заливки / приховані задні ребра із заливкою)

Cube::Layout Cube::CalcLayout() const
{
	const long dx = x2 - x1;
	const long dy = y2 - y1;
	const long depth = std::min(labs(dx), labs(dy)) / 3;
	const long sx = (dx >= 0) ? depth : -depth;
	const long sy = (dy >= 0) ? depth : -depth;

	Layout g = {};
	// Передня грань (F0..F3) і задня грань (B0..B3); B0 завжди прихована за передньою гранню
	g.front[0] = { x1, y1 };
	g.front[1] = { x2 - sx, y1 };
	g.front[2] = { x2 - sx, y2 - sy };
	g.front[3] = { x1, y2 - sy };

	g.back[0] = { x1 + sx, y1 + sy };
	g.back[1] = { x2, y1 + sy };
	g.back[2] = { x2, y2 };
	g.back[3] = { x1 + sx, y2 };

	// Дві видимі бічні грані
	g.sideA[0] = g.front[1]; g.sideA[1] = g.back[1];
	g.sideA[2] = g.back[2];  g.sideA[3] = g.front[2];

	g.sideB[0] = g.front[3]; g.sideB[1] = g.front[2];
	g.sideB[2] = g.back[2];  g.sideB[3] = g.back[3];
	return g;
}

void Cube::DrawGeometry(HDC hdc) const
{
	const Layout g = CalcLayout();

	if (style.hasFill)
	{
		bool ownsBrush = false;
		HPEN hPen = CreateBorderPen();
		HBRUSH hBrush = CreateFillBrush(ownsBrush);
		HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

		Polygon(hdc, g.sideA, 4);
		Polygon(hdc, g.sideB, 4);
		Polygon(hdc, g.front, 4);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		if (ownsBrush) DeleteObject(hBrush);
		if (style.hasBorder) DeleteObject(hPen);
		return;
	}

	if (!style.hasBorder) return;

	// Каркасний режим без заливки: видно всі ребра
	for (int i = 0; i < 4; ++i)
	{
		const int next = (i + 1) % 4;
		DrawLinePrimitive(hdc, g.back[i].x, g.back[i].y, g.back[next].x, g.back[next].y);
		DrawLinePrimitive(hdc, g.front[i].x, g.front[i].y, g.back[i].x, g.back[i].y);
		DrawLinePrimitive(hdc, g.front[i].x, g.front[i].y, g.front[next].x, g.front[next].y);
	}
}

void Cube::SerializeSvgBody(FILE* fout) const
{
	const Layout g = CalcLayout();

	if (style.hasFill)
	{
		const POINT* faces[3] = { g.sideA, g.sideB, g.front };
		for (const POINT* face : faces)
		{
			fwprintf(fout, L"  <polygon");
			WriteSvgPointsAttr(fout, face, 4);
			WriteSvgColorAttr(fout, L"stroke", style.borderColor, style.hasBorder);
			WriteSvgColorAttr(fout, L"fill", style.fillColor, true);
			fwprintf(fout, L" stroke-width=\"%d\" />\n", style.hasBorder ? style.borderWidth : 0);
		}
		return;
	}

	if (!style.hasBorder) return;
	for (int i = 0; i < 4; ++i)
	{
		const int next = (i + 1) % 4;
		auto writeLine = [&](POINT a, POINT b) {
			fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", a.x, a.y, b.x, b.y);
			WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
			fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
			};
		writeLine(g.back[i], g.back[next]);
		writeLine(g.front[i], g.back[i]);
		writeLine(g.front[i], g.front[next]);
	}
}

// Cyl (каркас без заливки / прихована задня дуга нижньої основи із заливкою)

Cyl::Layout Cyl::CalcLayout() const
{
	Layout g = {};
	g.left = std::min(x1, x2);
	g.right = std::max(x1, x2);
	g.invertedY = (y2 < y1);

	const long absH = labs(y2 - y1);
	long ry = std::min(absH / 4, absH / 2);
	if (ry == 0 && absH > 0) ry = 1;

	if (!g.invertedY)
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
	g.bodyTop = std::min(c1Y, c2Y);
	g.bodyBottom = std::max(c1Y, c2Y);
	return g;
}

void Cyl::DrawGeometry(HDC hdc) const
{
	const Layout g = CalcLayout();

	if (style.hasFill)
	{
		HPEN hFillPen = CreatePen(PS_SOLID, 1, style.fillColor);
		HBRUSH hFillBrush = CreateSolidBrush(style.fillColor);
		HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hFillPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hFillBrush));

		Ellipse(hdc, g.left, g.cap2Top, g.right, g.cap2Bottom);
		Rectangle(hdc, g.left, g.bodyTop, g.right, g.bodyBottom + 1);
		Ellipse(hdc, g.left, g.cap1Top, g.right, g.cap1Bottom);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hFillBrush);
		DeleteObject(hFillPen);
	}

	if (!style.hasBorder) return;

	const long rightEdge = (g.right > g.left) ? (g.right - 1) : g.right;

	if (style.hasFill)
	{
		// Малюємо лише видиму зовнішню напівдугу дальньої основи
		HPEN hPen = CreateBorderPen();
		HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
		const long cap2MidY = (g.cap2Top + g.cap2Bottom) / 2;
		if (!g.invertedY)
			Arc(hdc, g.left, g.cap2Top, g.right, g.cap2Bottom, g.left, cap2MidY, g.right, cap2MidY);
		else
			Arc(hdc, g.left, g.cap2Top, g.right, g.cap2Bottom, g.right, cap2MidY, g.left, cap2MidY);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
	}
	else
	{
		DrawEllipsePrimitive(hdc, g.left, g.cap2Top, g.right, g.cap2Bottom, false);
	}

	DrawLinePrimitive(hdc, g.left, g.bodyTop, g.left, g.bodyBottom);
	DrawLinePrimitive(hdc, rightEdge, g.bodyTop, rightEdge, g.bodyBottom);
	DrawEllipsePrimitive(hdc, g.left, g.cap1Top, g.right, g.cap1Bottom, false);
}

void Cyl::SerializeSvgBody(FILE* fout) const
{
	const Layout g = CalcLayout();
	const long width = g.right - g.left;
	const double cx = (g.left + g.right) * 0.5;
	const double rx = width * 0.5;
	const double cap1Cy = (g.cap1Top + g.cap1Bottom) * 0.5;
	const double cap2Cy = (g.cap2Top + g.cap2Bottom) * 0.5;
	const double ry1 = (g.cap1Bottom - g.cap1Top) * 0.5;
	const double ry2 = (g.cap2Bottom - g.cap2Top) * 0.5;

	if (style.hasFill)
	{
		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" stroke=\"none\"", cx, cap2Cy, rx, ry2);
		WriteSvgColorAttr(fout, L"fill", style.fillColor, true);
		fwprintf(fout, L" />\n");

		fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\" stroke=\"none\"",
			g.left, g.bodyTop, width, g.bodyBottom - g.bodyTop);
		WriteSvgColorAttr(fout, L"fill", style.fillColor, true);
		fwprintf(fout, L" />\n");

		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" stroke=\"none\"", cx, cap1Cy, rx, ry1);
		WriteSvgColorAttr(fout, L"fill", style.fillColor, true);
		fwprintf(fout, L" />\n");
	}

	if (!style.hasBorder) return;

	if (style.hasFill)
	{
		const int sweep = g.invertedY ? 1 : 0;
		fwprintf(fout, L"  <path d=\"M %ld,%.1f A %.1f,%.1f 0 0,%d %ld,%.1f\" fill=\"none\"",
			g.left, cap2Cy, rx, ry2, sweep, g.right, cap2Cy);
		WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
	}
	else
	{
		fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" fill=\"none\"", cx, cap2Cy, rx, ry2);
		WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
		fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
	}

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", g.left, g.bodyTop, g.left, g.bodyBottom);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);

	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", g.right, g.bodyTop, g.right, g.bodyBottom);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);

	fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\" fill=\"none\"", cx, cap1Cy, rx, ry1);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
}