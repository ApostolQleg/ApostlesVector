#include "pch.h"
#include "tools.h"
#include "shapes_basic.h"
#include "shapes_poly.h"
#include "shapes_complex.h"
#include "shape_text.h"

template <typename T>
static std::unique_ptr<Shape> MakeShape()
{
	return std::make_unique<T>();
}

static const ToolDescriptor kTools[] = {
	// Режими виділення
	{ IDM_SELECT,      0,  L"Select",               nullptr,          ToolbarButtonKind::ModeTool,        nullptr },
	{ IDM_SELECT_AREA, 1,  L"Select Area",          nullptr,          ToolbarButtonKind::ModeTool,        nullptr },

	// Інструменти малювання фігур
	{ IDM_NODE,        2,  L"Node",                 L"Node",          ToolbarButtonKind::ModeTool,        &MakeShape<Node> },
	{ IDM_EDGE,        3,  L"Edge",                 L"Edge",          ToolbarButtonKind::ModeTool,        &MakeShape<Edge> },
	{ IDM_LINE,        4,  L"Line",                 L"Line",          ToolbarButtonKind::ModeTool,        &MakeShape<Line> },
	{ IDM_ARROW,       5,  L"Arrow",                L"Arrow",         ToolbarButtonKind::ModeTool,        &MakeShape<Arrow> },
	{ IDM_TRNGL,       6,  L"Triangle",             L"Triangle",      ToolbarButtonKind::ModeTool,        &MakeShape<Trngl> },
	{ IDM_RIGHT_TRNGL, 7,  L"Right Triangle",       L"RightTriangle", ToolbarButtonKind::ModeTool,        &MakeShape<RightTrngl> },
	{ IDM_RECT,        8,  L"Rectangle",            L"Rectangle",     ToolbarButtonKind::ModeTool,        &MakeShape<Rect> },
	{ IDM_ROMB,        9,  L"Rhombus",              L"Rhombus",       ToolbarButtonKind::ModeTool,        &MakeShape<Romb> },
	{ IDM_PENT,        10, L"Pentagon",             L"Pentagon",      ToolbarButtonKind::ModeTool,        &MakeShape<Pent> },
	{ IDM_HEX,         11, L"Hexagon",              L"Hexagon",       ToolbarButtonKind::ModeTool,        &MakeShape<Hex> },
	{ IDM_ELIPS,       12, L"Ellipse",              L"Ellipse",       ToolbarButtonKind::ModeTool,        &MakeShape<Elips> },
	{ IDM_CUBE,        13, L"Cube",                 L"Cube",          ToolbarButtonKind::ModeTool,        &MakeShape<Cube> },
	{ IDM_CYL,         14, L"Cylinder",             L"Cylinder",      ToolbarButtonKind::ModeTool,        &MakeShape<Cyl> },
	{ IDM_TEXT,        15, L"Text",                 L"Text",          ToolbarButtonKind::ModeTool,        &MakeShape<Text> },

	// Миттєві дії віддзеркалення (тільки для 1 фігури)
	{ IDM_FLIP_H,      16, L"Flip Horizontal",      nullptr,          ToolbarButtonKind::SingleAction,    nullptr },
	{ IDM_FLIP_V,      17, L"Flip Vertical",        nullptr,          ToolbarButtonKind::SingleAction,    nullptr },

	// Керування шарами (для 1+ фігур)
	{ IDM_Z_FRONT,     18, L"Bring to Front",       nullptr,          ToolbarButtonKind::SelectionAction, nullptr },
	{ IDM_Z_UP,        19, L"Bring Forward",        nullptr,          ToolbarButtonKind::SelectionAction, nullptr },
	{ IDM_Z_DOWN,      20, L"Send Backward",        nullptr,          ToolbarButtonKind::SelectionAction, nullptr },
	{ IDM_Z_BACK,      21, L"Send to Back",         nullptr,          ToolbarButtonKind::SelectionAction, nullptr }
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

bool ToolRegistry::IsModeToolId(UINT cmdId)
{
	const ToolDescriptor* tool = FindByCmdId(cmdId);
	return tool && tool->kind == ToolbarButtonKind::ModeTool;
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