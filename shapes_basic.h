#pragma once
#include "shape.h"

class Line : public Shape
{
protected:
	bool HitTestLocal(long lx, long ly, long tolerance) const override;
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	ResizeMode GetResizeMode() const override { return ResizeMode::Endpoints; }
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Line>(*this); }
	const WCHAR* GetTypeName() const override { return L"Line"; }
};

class Rect : public Shape
{
protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Rect>(*this); }
	const WCHAR* GetTypeName() const override { return L"Rectangle"; }
};

class Elips : public Shape
{
protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Elips>(*this); }
	const WCHAR* GetTypeName() const override { return L"Ellipse"; }
};