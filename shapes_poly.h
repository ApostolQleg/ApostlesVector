#pragma once
#include "shape.h"

class Poly : public Shape
{
protected:
	virtual void CalcVertices(POINT* pts, int& count) const = 0;
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;
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