#pragma once
#include "shape.h"

class Text : public Shape
{
private:
	std::wstring text;

protected:
	void DrawGeometry(HDC hdc) const override;
	void SerializeSvgBody(FILE* fout) const override;

public:
	void SetText(const std::wstring& value) { text = value; }
	const std::wstring& GetText() const { return text; }

	std::unique_ptr<Shape> Clone() const override { return std::make_unique<Text>(*this); }
	const WCHAR* GetTypeName() const override { return L"Text"; }

	void Serialize(FILE* fout) const override;
	bool Deserialize(const WCHAR* params) override;
};