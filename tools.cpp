#include "pch.h"
#include "framework.h"
#include "tools.h"
#include "shape.h"

template <typename T>
static std::unique_ptr<Shape> MakeShape()
{
	return std::make_unique<T>();
}

static const ToolDescriptor kTools[] = {
	{ IDM_SELECT,      0,  L"Select",         nullptr,          nullptr },
	{ IDM_SELECT_AREA, 1,  L"Select Area",    nullptr,          nullptr },
	{ IDM_NODE,        2,  L"Node",           L"Node",          &MakeShape<Node> },
	{ IDM_EDGE,        3,  L"Edge",           L"Edge",          &MakeShape<Edge> },
	{ IDM_LINE,        4,  L"Line",           L"Line",          &MakeShape<Line> },
	{ IDM_ARROW,       5,  L"Arrow",          L"Arrow",         &MakeShape<Arrow> },
	{ IDM_TRNGL,       6,  L"Triangle",       L"Triangle",      &MakeShape<Trngl> },
	{ IDM_RIGHT_TRNGL, 7,  L"Right Triangle", L"RightTriangle", &MakeShape<RightTrngl> },
	{ IDM_RECT,        8,  L"Rectangle",      L"Rectangle",     &MakeShape<Rect> },
	{ IDM_ROMB,        9,  L"Rhombus",        L"Rhombus",       &MakeShape<Romb> },
	{ IDM_PENT,        10, L"Pentagon",       L"Pentagon",      &MakeShape<Pent> },
	{ IDM_HEX,         11, L"Hexagon",        L"Hexagon",       &MakeShape<Hex> },
	{ IDM_ELIPS,       12, L"Ellipse",        L"Ellipse",       &MakeShape<Elips> },
	{ IDM_CUBE,        13, L"Cube",           L"Cube",          &MakeShape<Cube> },
	{ IDM_CYL,         14, L"Cylinder",       L"Cylinder",      &MakeShape<Cyl> },
	{ IDM_TEXT,        15, L"Text",           L"Text",          &MakeShape<Text> }
};

std::span<const ToolDescriptor> ToolRegistry::GetAll()
{
	return kTools;
}

const ToolDescriptor* ToolRegistry::FindByCmdId(UINT cmdId)
{
	for (const auto& tool : kTools)
	{
		if (tool.cmdId == cmdId)
			return &tool;
	}
	return nullptr;
}

bool ToolRegistry::IsToolId(UINT cmdId)
{
	return FindByCmdId(cmdId) != nullptr;
}

std::unique_ptr<Shape> ToolRegistry::CreateShapeByCmdId(UINT cmdId)
{
	const ToolDescriptor* tool = FindByCmdId(cmdId);
	if (tool && tool->createShape)
		return tool->createShape();
	return nullptr;
}

std::unique_ptr<Shape> ToolRegistry::CreateShapeByName(const WCHAR* typeName)
{
	if (!typeName) return nullptr;
	for (const auto& tool : kTools)
	{
		if (tool.typeName && tool.createShape && wcscmp(tool.typeName, typeName) == 0)
			return tool.createShape();
	}
	return nullptr;
}
