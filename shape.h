#pragma once

#define NODE_RADIUS 12
#define EDGE_RADIUS 5
#define SELECTION_MARGIN 4
#define HANDLE_SIZE 4
#define MIN_SHAPE_SIZE 8

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

	HPEN CreateBorderPen() const;
	HBRUSH CreateFillBrush(bool& createdBrush) const;
	void NormalizeCoords(long& l, long& t, long& r, long& b) const;

public:
	enum class ResizeMode { None, Box, Endpoints };

	virtual ~Shape() {}

	void Set(long xstart, long ystart, long xend, long yend);
	void MoveBy(long dx, long dy);
	virtual void GetBounds(long& l, long& t, long& r, long& b) const;
	virtual bool HitTest(long sx, long sy, long tolerance = 6) const;

	virtual ResizeMode GetResizeMode() const { return ResizeMode::Box; }
	int GetHandlePoints(POINT* pts) const;
	int HitHandle(long sx, long sy) const;
	void ResizeFrom(const Shape& orig, int handle, long mx, long my);

	void SetStyle(COLORREF border, COLORREF fill, bool borderEnabled, bool filled, int width);
	void SetRubber(bool r) { rubber = r; }
	void SetSelected(bool s) { selected = s; }

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

	virtual void Show(HDC hdc) = 0;
	virtual void DrawSelection(HDC hdc) const;
	virtual Shape* Clone() const = 0;
	virtual const WCHAR* GetTypeName() const = 0;

	virtual void Serialize(FILE* fout) const;
	virtual bool Deserialize(const WCHAR* params);
	virtual void SerializeSvg(FILE* fout) const;

	static Shape* CreateByName(const WCHAR* typeName);
};

// Примітиви

class Line : virtual public Shape
{
public:
	bool HitTest(long sx, long sy, long tol = 6) const override;
	ResizeMode GetResizeMode() const override { return ResizeMode::Endpoints; }
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Line(*this); }
	const WCHAR* GetTypeName() const override { return L"Line"; }
	void SerializeSvg(FILE* fout) const override;
};

class Rect : virtual public Shape
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Rect(*this); }
	const WCHAR* GetTypeName() const override { return L"Rectangle"; }
	void SerializeSvg(FILE* fout) const override;
};

class Elips : virtual public Shape
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Elips(*this); }
	const WCHAR* GetTypeName() const override { return L"Ellipse"; }
	void SerializeSvg(FILE* fout) const override;
};

// Багатокутники

class Poly : virtual public Shape
{
protected:
	virtual void CalcVertices(POINT* pts, int& count) const = 0;
public:
	void GetBounds(long& l, long& t, long& r, long& b) const override;
	void Show(HDC hdc) override;
	void SerializeSvg(FILE* fout) const override;
};

class Trngl : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;
public:
	Shape* Clone() const override { return new Trngl(*this); }
	const WCHAR* GetTypeName() const override { return L"Triangle"; }
};

class RightTrngl : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;
public:
	Shape* Clone() const override { return new RightTrngl(*this); }
	const WCHAR* GetTypeName() const override { return L"RightTriangle"; }
};

class Romb : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;
public:
	Shape* Clone() const override { return new Romb(*this); }
	const WCHAR* GetTypeName() const override { return L"Rhombus"; }
};

class Pent : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;
public:
	Shape* Clone() const override { return new Pent(*this); }
	const WCHAR* GetTypeName() const override { return L"Pentagon"; }
};

class Hex : public Poly
{
protected:
	void CalcVertices(POINT* pts, int& count) const override;
public:
	Shape* Clone() const override { return new Hex(*this); }
	const WCHAR* GetTypeName() const override { return L"Hexagon"; }
};

// Складні фігури

class Node : public Elips
{
public:
	ResizeMode GetResizeMode() const override { return ResizeMode::None; }
	void GetBounds(long& l, long& t, long& r, long& b) const override;
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Node(*this); }
	const WCHAR* GetTypeName() const override { return L"Node"; }
	void SerializeSvg(FILE* fout) const override;
};

class Arrow : public Line
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Arrow(*this); }
	const WCHAR* GetTypeName() const override { return L"Arrow"; }
	void SerializeSvg(FILE* fout) const override;
};

class Edge : public Line, public Elips
{
public:
	bool HitTest(long sx, long sy, long tol = 6) const override { return Line::HitTest(sx, sy, tol); }
	ResizeMode GetResizeMode() const override { return ResizeMode::Endpoints; }
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Edge(*this); }
	const WCHAR* GetTypeName() const override { return L"Edge"; }
	void SerializeSvg(FILE* fout) const override;
};

class Cube : public Line, public Rect
{
public:
	bool HitTest(long sx, long sy, long tol = 6) const override { return Shape::HitTest(sx, sy, tol); }
	ResizeMode GetResizeMode() const override { return ResizeMode::Box; }
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Cube(*this); }
	const WCHAR* GetTypeName() const override { return L"Cube"; }
	void SerializeSvg(FILE* fout) const override;
};

class Cyl : public Line, public Elips
{
public:
	bool HitTest(long sx, long sy, long tol = 6) const override { return Shape::HitTest(sx, sy, tol); }
	ResizeMode GetResizeMode() const override { return ResizeMode::Box; }
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Cyl(*this); }
	const WCHAR* GetTypeName() const override { return L"Cylinder"; }
	void SerializeSvg(FILE* fout) const override;
};

// Текст

class Text : virtual public Shape
{
private:
	std::wstring text;
public:
	void SetText(const std::wstring& str) { text = str; }
	const std::wstring& GetText() const { return text; }

	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Text(*this); }
	const WCHAR* GetTypeName() const override { return L"Text"; }

	void Serialize(FILE* fout) const override;
	bool Deserialize(const WCHAR* params) override;
	void SerializeSvg(FILE* fout) const override;
};
