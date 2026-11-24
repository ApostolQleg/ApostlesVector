#pragma once

constexpr long kNodeRadius = 12;
constexpr long kEdgeRadius = 5;
constexpr long kSelectionMargin = 4;
constexpr long kHandleSize = 4;
constexpr long kMinShapeSize = 8;

class Shape
{
protected:
	long x1 = 0, y1 = 0;
	long x2 = 0, y2 = 0;

	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasBorder = true;
	bool hasFill = false;
	int borderWidth = 1;

	bool rubber = false;
	bool selected = false;
	bool groupSelected = false;

	HPEN CreateBorderPen() const;
	HBRUSH CreateFillBrush(bool& createdBrush) const;
	void NormalizeCoords(long& l, long& t, long& r, long& b) const;

	void DrawLinePrimitive(HDC hdc, long ax, long ay, long bx, long by) const;
	void DrawRectPrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const;
	void DrawEllipsePrimitive(HDC hdc, long ax, long ay, long bx, long by, bool allowFill) const;

public:
	enum class ResizeMode { None, Box, Endpoints };

	virtual ~Shape() = default;

	void Set(long xStart, long yStart, long xEnd, long yEnd);
	void MoveBy(long dx, long dy);
	virtual void GetBounds(long& l, long& t, long& r, long& b) const;
	virtual bool HitTest(long sx, long sy, long tolerance = 6) const;

	virtual ResizeMode GetResizeMode() const { return ResizeMode::Box; }
	int GetHandlePoints(POINT* pts) const;
	int HitHandle(long sx, long sy) const;
	void ResizeFrom(const Shape& orig, int handle, long mx, long my);
	void TransformInBox(const Shape& orig,
		long oldL, long oldT, long oldR, long oldB,
		long newL, long newT, long newR, long newB);

	static int GetBoxHandlePoints(long l, long t, long r, long b, POINT* pts);
	static int HitBoxHandle(long l, long t, long r, long b, long sx, long sy);
	static void CalcResizedBox(long origL, long origT, long origR, long origB,
		int handle, long mx, long my,
		long& newL, long& newT, long& newR, long& newB);
	static void DrawBoxSelectionFrame(HDC hdc, long l, long t, long r, long b, bool drawHandles);

	void SetStyle(COLORREF border, COLORREF fill, bool borderEnabled, bool filled, int width);
	void SetRubber(bool r) { rubber = r; }
	void SetSelected(bool s, bool inGroup = false) { selected = s; groupSelected = inGroup; }

	long GetX1() const { return x1; }
	long GetY1() const { return y1; }
	long GetX2() const { return x2; }
	long GetY2() const { return y2; }

	COLORREF GetBorderColor() const { return borderColor; }
	COLORREF GetFillColor() const { return fillColor; }
	bool HasBorder() const { return hasBorder; }
	bool IsFilled() const { return hasFill; }
	int GetBorderWidth() const { return borderWidth; }
	bool IsSelected() const { return selected; }

	virtual void Show(HDC hdc) const = 0;
	virtual void DrawSelection(HDC hdc) const;
	virtual std::unique_ptr<Shape> Clone() const = 0;
	virtual const WCHAR* GetTypeName() const = 0;

	virtual void Serialize(FILE* fout) const;
	virtual bool Deserialize(const WCHAR* params);
	virtual void SerializeSvg(FILE* fout) const;
};

// Базові примітиви

class Line : public Shape
{
public:
	bool HitTest(long sx, long sy, long tolerance = 6) const override;
	ResizeMode GetResizeMode() const override { return ResizeMode::Endpoints; }
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Line>(*this); }
	const WCHAR* GetTypeName() const override { return L"Line"; }
	void SerializeSvg(FILE* fout) const override;
};

class Rect : public Shape
{
public:
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Rect>(*this); }
	const WCHAR* GetTypeName() const override { return L"Rectangle"; }
	void SerializeSvg(FILE* fout) const override;
};

class Elips : public Shape
{
public:
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Elips>(*this); }
	const WCHAR* GetTypeName() const override { return L"Ellipse"; }
	void SerializeSvg(FILE* fout) const override;
};

// Багатокутники

class Poly : public Shape
{
protected:
	virtual void CalcVertices(POINT* pts, int& count) const = 0;

public:
	void GetBounds(long& l, long& t, long& r, long& b) const override;
	void Show(HDC hdc) const override;
	void SerializeSvg(FILE* fout) const override;
};

class Trngl : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Trngl>(*this); }
	const WCHAR* GetTypeName() const override { return L"Triangle"; }
};

class RightTrngl : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<RightTrngl>(*this); }
	const WCHAR* GetTypeName() const override { return L"RightTriangle"; }
};

class Romb : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Romb>(*this); }
	const WCHAR* GetTypeName() const override { return L"Rhombus"; }
};

class Pent : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Pent>(*this); }
	const WCHAR* GetTypeName() const override { return L"Pentagon"; }
};

class Hex : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Hex>(*this); }
	const WCHAR* GetTypeName() const override { return L"Hexagon"; }
};

// Складні фігури

class Node : public Shape
{
public:
	ResizeMode GetResizeMode() const override { return ResizeMode::None; }
	void GetBounds(long& l, long& t, long& r, long& b) const override;
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Node>(*this); }
	const WCHAR* GetTypeName() const override { return L"Node"; }
	void SerializeSvg(FILE* fout) const override;
};

class Arrow : public Line
{
public:
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Arrow>(*this); }
	const WCHAR* GetTypeName() const override { return L"Arrow"; }
	void SerializeSvg(FILE* fout) const override;
};

class Edge : public Line
{
public:
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Edge>(*this); }
	const WCHAR* GetTypeName() const override { return L"Edge"; }
	void SerializeSvg(FILE* fout) const override;
};

class Cube : public Shape
{
private:
	struct CubeLayout
	{
		POINT hull[6] = {};
		RECT backRect = {};
		RECT frontRect = {};
		POINT edges[4][2] = {};
	};

	CubeLayout CalcLayout() const;

public:
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Cube>(*this); }
	const WCHAR* GetTypeName() const override { return L"Cube"; }
	void SerializeSvg(FILE* fout) const override;
};

class Cyl : public Shape
{
private:
	struct CylLayout
	{
		long left = 0;
		long right = 0;
		long cap1Top = 0;
		long cap1Bottom = 0;
		long cap2Top = 0;
		long cap2Bottom = 0;
		long bodyTop = 0;
		long bodyBottom = 0;
	};

	CylLayout CalcLayout() const;

public:
	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Cyl>(*this); }
	const WCHAR* GetTypeName() const override { return L"Cylinder"; }
	void SerializeSvg(FILE* fout) const override;
};

// Текст

class Text : public Shape
{
private:
	std::wstring text;

public:
	void SetText(const std::wstring& str) { text = str; }
	const std::wstring& GetText() const { return text; }

	void Show(HDC hdc) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Text>(*this); }
	const WCHAR* GetTypeName() const override { return L"Text"; }

	void Serialize(FILE* fout) const override;
	bool Deserialize(const WCHAR* params) override;
	void SerializeSvg(FILE* fout) const override;
};
