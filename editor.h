#pragma once

#include "Resource.h"
#include "shape.h"
#include "toolbar.h"
#include "viewer.h"
#include "files.h"

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

	long xstart = 0, ystart = 0;
	long lastMoveX = 0, lastMoveY = 0;

	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasBorder = true;
	bool hasFill = false;
	int borderWidth = 1;
	COLORREF customColors[16] = {};

	void GetSceneMousePos(HWND hWnd, long& sx, long& sy) const;
	void ApplyCurrentStyle(Shape* pShape) const;
	void StartTool(Shape* pShape, UINT cmdId);
	void SelectTool(UINT cmdId);

	void ChooseBorderColor(HWND hWnd);
	void ChooseFillColor(HWND hWnd);
	void ToggleNoBorder(HWND hWnd);
	void ToggleNoFill(HWND hWnd);
	void SetBorderWidth(HWND hWnd, int width);

	bool PromptTextInput(HWND hWnd, std::wstring& outText) const;

public:
	static Editor* getInstance();

	void OnCreate(HWND hWnd);
	void OnSize(HWND hWnd);
	LRESULT OnNotify(LPARAM lParam);

	void OnHScroll(HWND hWnd, WPARAM wParam);
	void OnVScroll(HWND hWnd, WPARAM wParam);
	void OnMouseWheel(HWND hWnd, WPARAM wParam);

	void OnLBDown(HWND hWnd);
	void OnLBDblClk(HWND hWnd);
	void OnMouseMove(HWND hWnd);
	void OnLBUp(HWND hWnd);

	void OnPaint(HWND hWnd);
	void OnInitMenuPopup(HWND hWnd);
	bool OnCommand(HWND hWnd, WPARAM wParam);
	bool OnClose(HWND hWnd);
};
