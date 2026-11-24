#pragma once

#include "Resource.h"

class Shape;

struct ToolDescriptor
{
	UINT cmdId;
	int bitmapIndex;
	const WCHAR* tooltip;
	const WCHAR* typeName;
	std::unique_ptr<Shape>(*createShape)();
};

class ToolRegistry
{
public:
	static std::span<const ToolDescriptor> GetAll();
	static const ToolDescriptor* FindByCmdId(UINT cmdId);
	static bool IsToolId(UINT cmdId);
	static std::unique_ptr<Shape> CreateShapeByCmdId(UINT cmdId);
	static std::unique_ptr<Shape> CreateShapeByName(const WCHAR* typeName);
};
