#pragma once

#include "Resource.h"
#include "shape.h"
#include "toolbar.h"
#include "viewer.h"
#include "files.h"
#include "history.h"
#include "text.h"

class Editor
{
private:
	static Editor* instance;

	Editor() = default;
	~Editor() = default;

	Editor(const Editor&) = delete;
	Editor& operator=(const Editor&) = delete;

	Toolbar toolbar;
	Viewer viewer;
	Files files;
	CommandHistory history;
	InplaceTextEditor textEditor;

	std::unique_ptr<Shape> currentShape;
	UINT currentToolId = 0;

	bool isDrawing = false;
	bool isSelectingArea = false;
	bool rubberDrawn = false;
	bool isMoving = false;
	bool isResizing = false;
	bool changedDuringDrag = false;
	int resizeHandle = -1;
	RECT dragOrigBounds = {};
	std::vector<ShapeSnapshot> dragOrigStates;

	long xStart = 0, yStart = 0;
	long lastMoveX = 0, lastMoveY = 0;
	long rubberEndX = 0, rubberEndY = 0;

	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasBorder = true;
	bool hasFill = false;
	int borderWidth = 1;
	COLORREF customColors[16] = {};

	void GetSceneMousePos(LPARAM lParam, long& sx, long& sy) const;
	void ApplyCurrentStyle(Shape* shape) const;
	void StartTool(std::unique_ptr<Shape> shape, UINT cmdId);
	void SelectTool(UINT cmdId);
	void FinishTextEdit(HWND hWnd, bool commit);

	std::vector<ShapeSnapshot> CaptureSelectedSnapshots() const;
	void BeginResizeDrag(HWND hWnd, int handle);
	void BeginMoveDrag(HWND hWnd, long sx, long sy);

	template <typename MutateFn>
	void ApplyStyleChange(HWND hWnd, MutateFn mutate);

	void ChooseBorderColor(HWND hWnd);
	void ChooseFillColor(HWND hWnd);
	void ToggleNoBorder(HWND hWnd);
	void ToggleNoFill(HWND hWnd);
	void SetBorderWidth(HWND hWnd, int width);

public:
	static Editor* GetInstance();

	bool IsTextEditing() const { return textEditor.IsActive(); }

	void OnCreate(HWND hWnd);
	void OnSize(HWND hWnd);
	LRESULT OnNotify(LPARAM lParam);

	void OnHScroll(HWND hWnd, WPARAM wParam);
	void OnVScroll(HWND hWnd, WPARAM wParam);
	void OnMouseWheel(HWND hWnd, WPARAM wParam);

	void OnLBDown(HWND hWnd, LPARAM lParam);
	void OnLBDblClk(HWND hWnd);
	void OnMouseMove(HWND hWnd, LPARAM lParam);
	void OnLBUp(HWND hWnd, LPARAM lParam);

	void OnPaint(HWND hWnd);
	void OnInitMenuPopup(HWND hWnd);
	bool OnCommand(HWND hWnd, WPARAM wParam);
	bool OnClose(HWND hWnd);
};
