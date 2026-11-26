#include "pch.h"
#include "shapes_poly.h"

// Вписує правильний n-кутник у рамку (x1, y1)-(x2, y2) з урахуванням знаку напрямку (для віддзеркалення)
static void CalcBoxRegularPolygon(long x1, long y1, long x2, long y2, int sides, POINT* pts)
{
	double ux[8] = {};
	double uy[8] = {};
	double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;

	constexpr double startAngle = -kPi * 0.5;
	for (int i = 0; i < sides; ++i)
	{
		const double angle = startAngle + (2.0 * kPi * i) / sides;
		ux[i] = std::cos(angle);
		uy[i] = std::sin(angle);
		minX = std::min(minX, ux[i]);
		maxX = std::max(maxX, ux[i]);
		minY = std::min(minY, uy[i]);
		maxY = std::max(maxY, uy[i]);
	}

	const double spanX = (maxX > minX) ? (maxX - minX) : 1.0;
	const double spanY = (maxY > minY) ? (maxY - minY) : 1.0;
	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);

	for (int i = 0; i < sides; ++i)
	{
		pts[i].x = std::lround(static_cast<double>(x1) + ((ux[i] - minX) / spanX) * dx);
		pts[i].y = std::lround(static_cast<double>(y1) + ((uy[i] - minY) / spanY) * dy);
	}
}

void Poly::DrawGeometry(HDC hdc) const
{
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count < 3) return;

	bool ownsBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = CreateFillBrush(ownsBrush);

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

	Polygon(hdc, pts, count);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (ownsBrush) DeleteObject(hBrush);
	if (style.hasBorder) DeleteObject(hPen);
}

void Poly::SerializeSvgBody(FILE* fout) const
{
	POINT pts[8] = {};
	int count = 0;
	CalcVertices(pts, count);
	if (count < 3) return;

	fwprintf(fout, L"  <polygon");
	WriteSvgPointsAttr(fout, pts, count);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, style.hasBorder);
	WriteSvgColorAttr(fout, L"fill", style.fillColor, style.hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.hasBorder ? style.borderWidth : 0);
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
	pts[1] = { x1, y2 };
	pts[2] = { x2, y2 };
}

void Romb::CalcVertices(POINT* pts, int& count) const
{
	count = 4;
	pts[0] = { (x1 + x2) / 2, y1 };
	pts[1] = { x2, (y1 + y2) / 2 };
	pts[2] = { (x1 + x2) / 2, y2 };
	pts[3] = { x1, (y1 + y2) / 2 };
}

void Pent::CalcVertices(POINT* pts, int& count) const
{
	count = 5;
	CalcBoxRegularPolygon(x1, y1, x2, y2, count, pts);
}

void Hex::CalcVertices(POINT* pts, int& count) const
{
	count = 6;
	CalcBoxRegularPolygon(x1, y1, x2, y2, count, pts);
}