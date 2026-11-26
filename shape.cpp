#include "pch.h"
#include "shape.h"

static double NormalizeAngleDeg(double deg)
{
	while (deg >= 180.0) deg -= 360.0;
	while (deg < -180.0) deg += 360.0;
	return deg;
}

static XFORM MakeRotationXForm(double cx, double cy, double angleDeg)
{
	const double rad = angleDeg * kPi / 180.0;
	const double c = std::cos(rad);
	const double s = std::sin(rad);

	XFORM xf = {};
	xf.eM11 = static_cast<FLOAT>(c);
	xf.eM12 = static_cast<FLOAT>(s);
	xf.eM21 = static_cast<FLOAT>(-s);
	xf.eM22 = static_cast<FLOAT>(c);
	xf.eDx = static_cast<FLOAT>(cx - cx * c + cy * s);
	xf.eDy = static_cast<FLOAT>(cy - cx * s - cy * c);
	return xf;
}

void Shape::WriteSvgColorAttr(FILE* fout, const WCHAR* attrName, COLORREF color, bool enabled)
{
	if (!fout) return;
	if (!enabled)
	{
		fwprintf(fout, L" %s=\"none\"", attrName);
		return;
	}
	fwprintf(fout, L" %s=\"rgb(%d,%d,%d)\"", attrName,
		static_cast<int>(GetRValue(color)),
		static_cast<int>(GetGValue(color)),
		static_cast<int>(GetBValue(color)));
}

void Shape::WriteSvgPointsAttr(FILE* fout, const POINT* pts, int count)
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

void Shape::SetCoords(long startX, long startY, long endX, long endY)
{
	x1 = startX;
	y1 = startY;
	x2 = endX;
	y2 = endY;
}

void Shape::MoveBy(long dx, long dy)
{
	x1 += dx;
	y1 += dy;
	x2 += dx;
	y2 += dy;
}

void Shape::SetAngle(double degrees)
{
	angleDeg = NormalizeAngleDeg(degrees);
}

void Shape::FlipHorizontal()
{
	std::swap(x1, x2);
	angleDeg = NormalizeAngleDeg(-angleDeg);
}

void Shape::FlipVertical()
{
	std::swap(y1, y2);
	angleDeg = NormalizeAngleDeg(-angleDeg);
}

void Shape::RotateToPoint(const Shape& origin, long worldX, long worldY)
{
	double cx = 0.0, cy = 0.0;
	origin.GetCenter(cx, cy);
	const double rad = std::atan2(static_cast<double>(worldY) - cy, static_cast<double>(worldX) - cx);
	SetAngle(rad * 180.0 / kPi + 90.0);
}

void Shape::GetCenter(double& cx, double& cy) const
{
	long l = 0, t = 0, r = 0, b = 0;
	GetLocalBounds(l, t, r, b);
	cx = (l + r) * 0.5;
	cy = (t + b) * 0.5;
}

void Shape::PointToLocal(long worldX, long worldY, long& localX, long& localY) const
{
	if (std::abs(angleDeg) < 1e-4)
	{
		localX = worldX;
		localY = worldY;
		return;
	}
	double cx = 0.0, cy = 0.0;
	GetCenter(cx, cy);
	const double rad = angleDeg * kPi / 180.0;
	const double c = std::cos(rad);
	const double s = std::sin(rad);
	const double dx = static_cast<double>(worldX) - cx;
	const double dy = static_cast<double>(worldY) - cy;
	localX = std::lround(cx + dx * c + dy * s);
	localY = std::lround(cy - dx * s + dy * c);
}

void Shape::PointToWorld(long localX, long localY, long& worldX, long& worldY) const
{
	if (std::abs(angleDeg) < 1e-4)
	{
		worldX = localX;
		worldY = localY;
		return;
	}
	double cx = 0.0, cy = 0.0;
	GetCenter(cx, cy);
	const double rad = angleDeg * kPi / 180.0;
	const double c = std::cos(rad);
	const double s = std::sin(rad);
	const double dx = static_cast<double>(localX) - cx;
	const double dy = static_cast<double>(localY) - cy;
	worldX = std::lround(cx + dx * c - dy * s);
	worldY = std::lround(cy + dx * s + dy * c);
}

void Shape::NormalizeCoords(long& l, long& t, long& r, long& b) const
{
	l = std::min(x1, x2);
	r = std::max(x1, x2);
	t = std::min(y1, y2);
	b = std::max(y1, y2);
}

void Shape::GetLocalBounds(long& l, long& t, long& r, long& b) const
{
	NormalizeCoords(l, t, r, b);
}

void Shape::GetBounds(long& l, long& t, long& r, long& b) const
{
	long ll = 0, lt = 0, lr = 0, lb = 0;
	GetLocalBounds(ll, lt, lr, lb);
	if (std::abs(angleDeg) < 1e-4)
	{
		l = ll; t = lt; r = lr; b = lb;
		return;
	}

	const POINT corners[4] = { { ll, lt },{ lr, lt },{ lr, lb },{ ll, lb } };
	for (int i = 0; i < 4; ++i)
	{
		long wx = 0, wy = 0;
		PointToWorld(corners[i].x, corners[i].y, wx, wy);
		if (i == 0)
		{
			l = r = wx;
			t = b = wy;
		}
		else
		{
			l = std::min(l, wx);
			r = std::max(r, wx);
			t = std::min(t, wy);
			b = std::max(b, wy);
		}
	}
}

bool Shape::HitTestLocal(long lx, long ly, long tolerance) const
{
	long l = 0, t = 0, r = 0, b = 0;
	GetLocalBounds(l, t, r, b);
	return (lx >= l - tolerance && lx <= r + tolerance &&
		ly >= t - tolerance && ly <= b + tolerance);
}

bool Shape::HitTest(long worldX, long worldY, long tolerance) const
{
	long lx = 0, ly = 0;
	PointToLocal(worldX, worldY, lx, ly);
	return HitTestLocal(lx, ly, tolerance);
}

int Shape::GetBoxHandlePoints(long l, long t, long r, long b, POINT* pts)
{
	l -= kSelectionMargin;
	t -= kSelectionMargin;
	r += kSelectionMargin;
	b += kSelectionMargin;
	const long cx = (l + r) / 2;
	const long cy = (t + b) / 2;

	pts[0] = { l, t };  pts[1] = { cx, t };  pts[2] = { r, t };
	pts[3] = { r, cy }; pts[4] = { r, b };   pts[5] = { cx, b };
	pts[6] = { l, b };  pts[7] = { l, cy };
	return 8;
}

int Shape::HitBoxHandle(long l, long t, long r, long b, long sx, long sy)
{
	POINT pts[8] = {};
	const int n = GetBoxHandlePoints(l, t, r, b, pts);
	for (int i = n - 1; i >= 0; --i)
	{
		if (labs(sx - pts[i].x) <= kHandleSize + 2 && labs(sy - pts[i].y) <= kHandleSize + 2)
			return i;
	}
	return -1;
}

void Shape::CalcResizedBox(long origL, long origT, long origR, long origB,
	int handle, long mx, long my,
	long& newL, long& newT, long& newR, long& newB)
{
	const bool moveLeft = (handle == 0 || handle == 6 || handle == 7);
	const bool moveRight = (handle == 2 || handle == 3 || handle == 4);
	const bool moveTop = (handle == 0 || handle == 1 || handle == 2);
	const bool moveBottom = (handle == 4 || handle == 5 || handle == 6);

	newL = origL; newT = origT; newR = origR; newB = origB;
	if (moveLeft)   newL = mx + kSelectionMargin;
	if (moveRight)  newR = mx - kSelectionMargin;
	if (moveTop)    newT = my + kSelectionMargin;
	if (moveBottom) newB = my - kSelectionMargin;

	if (newR - newL < kMinShapeSize)
	{
		if (moveLeft) newL = newR - kMinShapeSize;
		else newR = newL + kMinShapeSize;
	}
	if (newB - newT < kMinShapeSize)
	{
		if (moveTop) newT = newB - kMinShapeSize;
		else newB = newT + kMinShapeSize;
	}
}

void Shape::TransformInBox(const Shape& origin,
	long oldL, long oldT, long oldR, long oldB,
	long newL, long newT, long newR, long newB)
{
	const double sx = (oldR > oldL) ? static_cast<double>(newR - newL) / (oldR - oldL) : 1.0;
	const double sy = (oldB > oldT) ? static_cast<double>(newB - newT) / (oldB - oldT) : 1.0;

	x1 = newL + std::lround((origin.x1 - oldL) * sx);
	y1 = newT + std::lround((origin.y1 - oldT) * sy);
	x2 = newL + std::lround((origin.x2 - oldL) * sx);
	y2 = newT + std::lround((origin.y2 - oldT) * sy);
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
		long l = 0, t = 0, r = 0, b = 0;
		GetLocalBounds(l, t, r, b);
		return GetBoxHandlePoints(l, t, r, b, pts);
	}

	default:
		return 0;
	}
}

void Shape::GetRotateHandleLocalPos(long& hx, long& hy) const
{
	long l = 0, t = 0, r = 0, b = 0;
	GetLocalBounds(l, t, r, b);
	hx = (l + r) / 2;
	hy = t - kSelectionMargin - kRotateHandleOffset;
}

int Shape::HitHandle(long worldX, long worldY) const
{
	long lx = 0, ly = 0;
	PointToLocal(worldX, worldY, lx, ly);

	long rx = 0, ry = 0;
	GetRotateHandleLocalPos(rx, ry);
	if (std::hypot(static_cast<double>(lx - rx), static_cast<double>(ly - ry)) <= kRotateHandleRadius + 4.0)
		return kHandleRotate;

	POINT pts[8] = {};
	const int n = GetHandlePoints(pts);
	for (int i = n - 1; i >= 0; --i)
	{
		if (labs(lx - pts[i].x) <= kHandleSize + 2 && labs(ly - pts[i].y) <= kHandleSize + 2)
			return i;
	}
	return -1;
}

void Shape::ResizeFrom(const Shape& origin, int handle, long worldX, long worldY)
{
	long lx = 0, ly = 0;
	origin.PointToLocal(worldX, worldY, lx, ly);

	double oldCx = 0.0, oldCy = 0.0;
	origin.GetCenter(oldCx, oldCy);

	switch (GetResizeMode())
	{
	case ResizeMode::Endpoints:
		x1 = origin.x1; y1 = origin.y1;
		x2 = origin.x2; y2 = origin.y2;
		if (handle == 0) { x1 = lx; y1 = ly; }
		else if (handle == 1) { x2 = lx; y2 = ly; }
		break;

	case ResizeMode::Box:
	{
		long l = 0, t = 0, r = 0, b = 0;
		origin.GetLocalBounds(l, t, r, b);
		long nl = 0, nt = 0, nr = 0, nb = 0;
		CalcResizedBox(l, t, r, b, handle, lx, ly, nl, nt, nr, nb);
		TransformInBox(origin, l, t, r, b, nl, nt, nr, nb);
		break;
	}

	default:
		return;
	}

	// Компенсуємо зміщення локального центру при обертанні, щоб протилежний край не «плив»
	if (std::abs(angleDeg) > 1e-4)
	{
		double newCx = 0.0, newCy = 0.0;
		GetCenter(newCx, newCy);
		const double dLocalX = newCx - oldCx;
		const double dLocalY = newCy - oldCy;
		const double rad = angleDeg * kPi / 180.0;
		const double c = std::cos(rad);
		const double s = std::sin(rad);
		const double dWorldX = dLocalX * c - dLocalY * s;
		const double dWorldY = dLocalX * s + dLocalY * c;
		MoveBy(std::lround(dWorldX - dLocalX), std::lround(dWorldY - dLocalY));
	}
}

void Shape::SetStyle(const ShapeStyle& newStyle)
{
	style = newStyle;
	if (style.borderWidth <= 0)
		style.borderWidth = 1;
}

HPEN Shape::CreateBorderPen() const
{
	if (!style.hasBorder)
		return static_cast<HPEN>(GetStockObject(NULL_PEN));
	return CreatePen(PS_SOLID, style.borderWidth, style.borderColor);
}

HBRUSH Shape::CreateFillBrush(bool& ownsBrush) const
{
	ownsBrush = false;
	if (!style.hasFill)
		return static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
	ownsBrush = true;
	return CreateSolidBrush(style.fillColor);
}

void Shape::DrawLinePrimitive(HDC hdc, long ax, long ay, long bx, long by) const
{
	if (!style.hasBorder) return;
	HPEN hPen = CreateBorderPen();
	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	MoveToEx(hdc, ax, ay, nullptr);
	LineTo(hdc, bx, by);
	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);
}

void Shape::DrawRectPrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const
{
	bool ownsBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = allowFill ? CreateFillBrush(ownsBrush)
		: static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

	Rectangle(hdc, ax, ay, bx, by);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (ownsBrush) DeleteObject(hBrush);
	if (style.hasBorder) DeleteObject(hPen);
}

void Shape::DrawEllipsePrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const
{
	bool ownsBrush = false;
	HPEN hPen = CreateBorderPen();
	HBRUSH hBrush = allowFill ? CreateFillBrush(ownsBrush)
		: static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

	Ellipse(hdc, ax, ay, bx, by);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	if (ownsBrush) DeleteObject(hBrush);
	if (style.hasBorder) DeleteObject(hPen);
}

void Shape::DrawBoxSelectionFrame(HDC hdc, long l, long t, long r, long b, bool drawHandles)
{
	HPEN hSelPen = CreatePen(PS_DASH, 1, RGB(0, 120, 215));
	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hSelPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

	Rectangle(hdc, l - kSelectionMargin, t - kSelectionMargin,
		r + kSelectionMargin, b + kSelectionMargin);

	if (drawHandles)
	{
		POINT pts[8] = {};
		const int n = GetBoxHandlePoints(l, t, r, b, pts);
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

void Shape::DrawSelection(HDC hdc) const
{
	long l = 0, t = 0, r = 0, b = 0;
	GetLocalBounds(l, t, r, b);

	HPEN hSelPen = CreatePen(PS_DASH, 1, RGB(0, 120, 215));
	HPEN hHandlePen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hSelPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

	Rectangle(hdc, l - kSelectionMargin, t - kSelectionMargin,
		r + kSelectionMargin, b + kSelectionMargin);

	// Круглий маркер обертання над верхнім центром рамки
	long rx = 0, ry = 0;
	GetRotateHandleLocalPos(rx, ry);
	SelectObject(hdc, hHandlePen);
	MoveToEx(hdc, rx, t - kSelectionMargin, nullptr);
	LineTo(hdc, rx, ry + kRotateHandleRadius);

	SelectObject(hdc, GetStockObject(WHITE_BRUSH));
	Ellipse(hdc, rx - kRotateHandleRadius, ry - kRotateHandleRadius,
		rx + kRotateHandleRadius + 1, ry + kRotateHandleRadius + 1);

	POINT pts[8] = {};
	const int n = GetHandlePoints(pts);
	for (int i = 0; i < n; ++i)
	{
		Rectangle(hdc, pts[i].x - kHandleSize, pts[i].y - kHandleSize,
			pts[i].x + kHandleSize + 1, pts[i].y + kHandleSize + 1);
	}

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hHandlePen);
	DeleteObject(hSelPen);
}

void Shape::Show(HDC hdc) const
{
	const int savedDc = SaveDC(hdc);
	if (std::abs(angleDeg) > 1e-4)
	{
		double cx = 0.0, cy = 0.0;
		GetCenter(cx, cy);
		XFORM xf = MakeRotationXForm(cx, cy, angleDeg);
		ModifyWorldTransform(hdc, &xf, MWT_LEFTMULTIPLY);
	}

	DrawGeometry(hdc);
	if (selected && !groupSelected)
		DrawSelection(hdc);

	RestoreDC(hdc, savedDc);

	if (selected && groupSelected)
	{
		long l = 0, t = 0, r = 0, b = 0;
		GetBounds(l, t, r, b);
		DrawBoxSelectionFrame(hdc, l, t, r, b, false);
	}
}

void Shape::Serialize(FILE* fout) const
{
	if (!fout) return;
	fwprintf(fout, L"%s %ld %ld %ld %ld %lu %lu %d %d %d %.2f\n",
		GetTypeName(), x1, y1, x2, y2,
		static_cast<unsigned long>(style.borderColor),
		static_cast<unsigned long>(style.fillColor),
		style.hasBorder ? 1 : 0, style.hasFill ? 1 : 0,
		style.borderWidth, angleDeg);
}

bool Shape::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int borderFlag = 1, filled = 0, width = 1;
	double parsedAngle = 0.0;

	const int parsed = swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d %d %lf",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &borderFlag, &filled, &width, &parsedAngle);

	if (parsed < 9)
		return false;

	style.borderColor = static_cast<COLORREF>(bCol);
	style.fillColor = static_cast<COLORREF>(fCol);
	style.hasBorder = (borderFlag != 0);
	style.hasFill = (filled != 0);
	style.borderWidth = (width > 0) ? width : 1;
	angleDeg = (parsed >= 10) ? NormalizeAngleDeg(parsedAngle) : 0.0;
	return true;
}

void Shape::SerializeSvg(FILE* fout) const
{
	if (!fout) return;
	const bool rotated = (std::abs(angleDeg) > 1e-4);
	if (rotated)
	{
		double cx = 0.0, cy = 0.0;
		GetCenter(cx, cy);
		fwprintf(fout, L"  <g transform=\"rotate(%.2f %.1f %.1f)\">\n", angleDeg, cx, cy);
	}
	SerializeSvgBody(fout);
	if (rotated)
		fwprintf(fout, L"  </g>\n");
}
