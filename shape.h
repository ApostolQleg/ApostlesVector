#pragma once

constexpr double kPi = 3.14159265358979323846;
constexpr long kNodeRadius = 12;
constexpr long kSelectionMargin = 4;
constexpr long kHandleSize = 4;
constexpr long kRotateHandleRadius = 5;
constexpr long kRotateHandleOffset = 22;
constexpr long kMinShapeSize = 8;
constexpr int kHandleRotate = 8;

struct ShapeStyle
{
	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasBorder = true;
	bool hasFill = false;
	int borderWidth = 1;
};

class Shape
{
protected:
	long x1 = 0, y1 = 0;
	long x2 = 0, y2 = 0;
	double angleDeg = 0.0;

	ShapeStyle style = {};
	bool selected = false;
	bool groupSelected = false;

	HPEN CreateBorderPen() const;
	HBRUSH CreateFillBrush(bool& ownsBrush) const;
	void NormalizeCoords(long& l, long& t, long& r, long& b) const;

	void DrawLinePrimitive(HDC hdc, long ax, long ay, long bx, long by) const;
	void DrawRectPrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const;
	void DrawEllipsePrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const;

	virtual void DrawGeometry(HDC hdc) const = 0;
	virtual void SerializeSvgBody(FILE* fout) const = 0;
	virtual bool HitTestLocal(long lx, long ly, long tolerance) const;

public:
	enum class ResizeMode { None, Box, Endpoints };

	virtual ~Shape() = default;

	void SetCoords(long startX, long startY, long endX, long endY);
	void MoveBy(long dx, long dy);
	void SetAngle(double degrees);
	double GetAngle() const { return angleDeg; }

	void FlipHorizontal();
	void FlipVertical();
	void RotateToPoint(const Shape& origin, long worldX, long worldY);

	void GetCenter(double& cx, double& cy) const;
	void PointToLocal(long worldX, long worldY, long& localX, long& localY) const;
	void PointToWorld(long localX, long localY, long& worldX, long& worldY) const;

	virtual void GetLocalBounds(long& l, long& t, long& r, long& b) const;
	void GetBounds(long& l, long& t, long& r, long& b) const;
	bool HitTest(long worldX, long worldY, long tolerance = 6) const;

	virtual ResizeMode GetResizeMode() const { return ResizeMode::Box; }
	int GetHandlePoints(POINT* pts) const;
	void GetRotateHandleLocalPos(long& hx, long& hy) const;
	int HitHandle(long worldX, long worldY) const;
	void ResizeFrom(const Shape& origin, int handle, long worldX, long worldY);
	void TransformInBox(const Shape& origin,
		long oldL, long oldT, long oldR, long oldB,
		long newL, long newT, long newR, long newB);

	static int GetBoxHandlePoints(long l, long t, long r, long b, POINT* pts);
	static int HitBoxHandle(long l, long t, long r, long b, long sx, long sy);
	static void CalcResizedBox(long origL, long origT, long origR, long origB,
		int handle, long mx, long my,
		long& newL, long& newT, long& newR, long& newB);
	static void DrawBoxSelectionFrame(HDC hdc, long l, long t, long r, long b, bool drawHandles);

	void SetStyle(const ShapeStyle& newStyle);
	const ShapeStyle& GetStyle() const { return style; }
	void SetSelected(bool isSel, bool inGroup = false) { selected = isSel; groupSelected = inGroup; }
	bool IsSelected() const { return selected; }

	long GetX1() const { return x1; }
	long GetY1() const { return y1; }
	long GetX2() const { return x2; }
	long GetY2() const { return y2; }

	void Show(HDC hdc) const;
	void DrawSelection(HDC hdc) const;

	virtual std::unique_ptr<Shape> Clone() const = 0;
	virtual const WCHAR* GetTypeName() const = 0;

	virtual void Serialize(FILE* fout) const;
	virtual bool Deserialize(const WCHAR* params);
	void SerializeSvg(FILE* fout) const;

	static void WriteSvgColorAttr(FILE* fout, const WCHAR* attrName, COLORREF color, bool enabled);
	static void WriteSvgPointsAttr(FILE* fout, const POINT* pts, int count);
};
