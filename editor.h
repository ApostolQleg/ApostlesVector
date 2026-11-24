#pragma once

#include "Resource.h"
#include "shape.h"
#include "toolbar.h"
#include "viewer.h"
#include "files.h"

#define IDC_INPLACE_EDIT 2001
#define IDM_TEXT_COMMIT  40001
#define IDM_TEXT_CANCEL  40002

class Editor
{
private:
	static Editor* p_instance;

	Editor() {}
	~Editor();

	Editor(const Editor&) = delete;
	Editor& operator=(const Editor&) = delete;

	Toolbar toolbar;
	Viewer viewer;
	Files files;

	Shape* pcurrent = nullptr;
	UINT currentToolId = 0;

	bool isDrawing = false;
	bool rubberDrawn = false;
	bool isMoving = false;
	bool movedAny = false;
	bool isResizing = false;
	int resizeHandle = -1;
	Shape* resizeOrig = nullptr;

	long xstart = 0, ystart = 0;
	long lastMoveX = 0, lastMoveY = 0;

	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasBorder = true;
	bool hasFill = false;
	int borderWidth = 1;
	COLORREF customColors[16] = {};

	HWND hEdit = nullptr;
	HFONT hEditFont = nullptr;
	Text* editTarget = nullptr;
	RECT editSceneRect = {};

	void GetSceneMousePos(LPARAM lParam, long& sx, long& sy) const;
	void ApplyCurrentStyle(Shape* pShape) const;
	void StartTool(Shape* pShape, UINT cmdId);
	void SelectTool(UINT cmdId);

	void ChooseBorderColor(HWND hWnd);
	void ChooseFillColor(HWND hWnd);
	void ToggleNoBorder(HWND hWnd);
	void ToggleNoFill(HWND hWnd);
	void SetBorderWidth(HWND hWnd, int width);

	void BeginTextEdit(HWND hWnd, Text* target, long l, long t, long r, long b);
	void EndTextEdit(HWND hWnd, bool commit);

public:
	static Editor* getInstance();

	bool IsTextEditing() const { return hEdit != nullptr; }

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
