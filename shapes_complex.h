#pragma once
#include "shapes_basic.h"

class Node : public Shape
{
protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	ResizeMode GetResizeMode() const override { return ResizeMode::None; }
	void GetLocalBounds(long& l, long& t, long& r, long& b) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Node>(*this); }
	const WCHAR* GetTypeName() const override { return L"Node"; }
};

class Arrow : public Line
{
protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Arrow>(*this); }
	const WCHAR* GetTypeName() const override { return L"Arrow"; }
};

class Edge : public Line
{
private:
	bool CalcClippedSegment(long& ax, long& ay, long& bx, long& by) const;

protected:
	bool HitTestLocal(long lx, long ly, long tolerance) const override;
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	void GetLocalBounds(long& l, long& t, long& r, long& b) const override;
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Edge>(*this); }
	const WCHAR* GetTypeName() const override { return L"Edge"; }
};

class Cube : public Shape
{
private:
	struct Layout
	{
		POINT front[4] = {};
		POINT back[4] = {};
		POINT sideA[4] = {};
		POINT sideB[4] = {};
	};

	Layout CalcLayout() const;

protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Cube>(*this); }
	const WCHAR* GetTypeName() const override { return L"Cube"; }
};

class Cyl : public Shape
{
private:
	struct Layout
	{
		long left = 0, right = 0;
		long cap1Top = 0, cap1Bottom = 0;
		long cap2Top = 0, cap2Bottom = 0;
		long bodyTop = 0, bodyBottom = 0;
		bool invertedY = false;
	};

	Layout CalcLayout() const;

protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Cyl>(*this); }
	const WCHAR* GetTypeName() const override { return L"Cylinder"; }
};