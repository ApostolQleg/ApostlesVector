#pragma once

#define NODE_RADIUS 12
#define EDGE_RADIUS 5

class Shape
{
protected:
	long x1 = 0, y1 = 0;
	long x2 = 0, y2 = 0;

	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasFill = false;
	int borderWidth = 1;

	bool rubber = false;

	HPEN CreateBorderPen() const;
	void NormalizeCoords(long& l, long& t, long& r, long& b) const;

public:
	virtual ~Shape() {}

	void Set(long xstart, long ystart, long xend, long yend);
	void SetStyle(COLORREF border, COLORREF fill, bool filled, int width);
	void SetRubber(bool r) { rubber = r; }

	COLORREF GetBorderColor() const { return borderColor; }
	COLORREF GetFillColor() const { return fillColor; }
	bool IsFilled() const { return hasFill; }
	int GetBorderWidth() const { return borderWidth; }

	virtual void Show(HDC hdc) = 0;
	virtual Shape* Clone() const = 0;
	virtual const WCHAR* GetTypeName() const = 0;

	virtual void Serialize(FILE* fout) const;
	virtual bool Deserialize(const WCHAR* params);

	static Shape* CreateByName(const WCHAR* typeName);
};

// Примітиви

class Line : virtual public Shape
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Line(*this); }
	const WCHAR* GetTypeName() const override { return L"Line"; }
};

class Rect : virtual public Shape
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Rect(*this); }
	const WCHAR* GetTypeName() const override { return L"Rectangle"; }
};

class Elips : virtual public Shape
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Elips(*this); }
	const WCHAR* GetTypeName() const override { return L"Ellipse"; }
};

// Багатокутники

class Poly : virtual public Shape // абстрактний клас для полігонів
{
protected:
	virtual void CalcVertices(POINT* pts, int& count) const = 0;
public:
	void Show(HDC hdc) override;
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
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Node(*this); }
	const WCHAR* GetTypeName() const override { return L"Node"; }
};

class Arrow : public Line
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Arrow(*this); }
	const WCHAR* GetTypeName() const override { return L"Arrow"; }
};

class Edge : public Line, public Elips
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Edge(*this); }
	const WCHAR* GetTypeName() const override { return L"Edge"; }
};

class Cube : public Line, public Rect
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Cube(*this); }
	const WCHAR* GetTypeName() const override { return L"Cube"; }
};

class Cyl : public Line, public Elips
{
public:
	void Show(HDC hdc) override;
	Shape* Clone() const override { return new Cyl(*this); }
	const WCHAR* GetTypeName() const override { return L"Cylinder"; }
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
};
