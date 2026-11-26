#pragma once

#include "Resource.h"

class Shape;

enum class ToolbarButtonKind
{
	ModeTool,
	SingleAction,
	SelectionAction
};

struct ToolDescriptor
{
	UINT cmdId;
	int bitmapIndex;
	const WCHAR* tooltip;
	const WCHAR* typeName;
	ToolbarButtonKind kind;
	std::unique_ptr<Shape>(*createShape)();
};

class ToolRegistry
{
public:
	static std::span<const ToolDescriptor> GetAll();
	static const ToolDescriptor* FindByCmdId(UINT cmdId);
	static bool IsModeToolId(UINT cmdId);
	static std::unique_ptr<Shape> CreateShapeByCmdId(UINT cmdId);
	static std::unique_ptr<Shape> CreateShapeByName(const WCHAR* typeName);
};