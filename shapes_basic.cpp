#include "pch.h"
#include "shapes_basic.h"

bool Line::HitTestLocal(long lx, long ly, long tolerance) const
{
	const double dx = static_cast<double>(x2 - x1);
	const double dy = static_cast<double>(y2 - y1);
	const double len2 = dx * dx + dy * dy;
	double t = (len2 > 0.0) ? ((lx - x1) * dx + (ly - y1) * dy) / len2 : 0.0;
	t = std::clamp(t, 0.0, 1.0);
	const double dist = std::hypot(lx - (x1 + t * dx), ly - (y1 + t * dy));
	return dist <= tolerance + style.borderWidth * 0.5;
}

void Line::DrawGeometry(HDC hdc) const
{
	DrawLinePrimitive(hdc, x1, y1, x2, y2);
}

void Line::SerializeSvgBody(FILE* fout) const
{
	if (!style.hasBorder) return;
	fwprintf(fout, L"  <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\"", x1, y1, x2, y2);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, true);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.borderWidth);
}

void Rect::DrawGeometry(HDC hdc) const
{
	long l = 0, t = 0, r = 0, b = 0;
	NormalizeCoords(l, t, r, b);
	DrawRectPrimitive(hdc, l, t, r, b, true);
}

void Rect::SerializeSvgBody(FILE* fout) const
{
	long l = 0, t = 0, r = 0, b = 0;
	NormalizeCoords(l, t, r, b);
	fwprintf(fout, L"  <rect x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\"", l, t, r - l, b - t);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, style.hasBorder);
	WriteSvgColorAttr(fout, L"fill", style.fillColor, style.hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.hasBorder ? style.borderWidth : 0);
}

void Elips::DrawGeometry(HDC hdc) const
{
	long l = 0, t = 0, r = 0, b = 0;
	NormalizeCoords(l, t, r, b);
	DrawEllipsePrimitive(hdc, l, t, r, b, true);
}

void Elips::SerializeSvgBody(FILE* fout) const
{
	long l = 0, t = 0, r = 0, b = 0;
	NormalizeCoords(l, t, r, b);
	fwprintf(fout, L"  <ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\"",
		(l + r) * 0.5, (t + b) * 0.5, (r - l) * 0.5, (b - t) * 0.5);
	WriteSvgColorAttr(fout, L"stroke", style.borderColor, style.hasBorder);
	WriteSvgColorAttr(fout, L"fill", style.fillColor, style.hasFill);
	fwprintf(fout, L" stroke-width=\"%d\" />\n", style.hasBorder ? style.borderWidth : 0);
}