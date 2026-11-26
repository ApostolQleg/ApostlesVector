#pragma once

#include "Resource.h"
#include "shape.h"
#include "toolbar.h"
#include "viewer.h"
#include "files.h"
#include "history.h"
#include "text_editor.h"

class Editor
{
private:
	enum class DragMode { None, Draw, SelectArea, Move, Resize, Rotate };

	Toolbar toolbar;
	Viewer viewer;
	Files files;
	CommandHistory history;
	InplaceTextEditor textEditor;

	UINT currentToolId = IDM_SELECT;
	std::unique_ptr<Shape> drawingShape;

	DragMode dragMode = DragMode::None;
	bool changedDuringDrag = false;
	int activeHandle = -1;
	long startX = 0, startY = 0;
	long lastMoveX = 0, lastMoveY = 0;
	RECT dragOrigBounds = {};
	SceneSnapshot dragBeforeSnapshot;
	std::vector<std::pair<int, std::unique_ptr<Shape>>> dragOrigShapes;

	ShapeStyle activeStyle = {};
	COLORREF customColors[16] = {};

	void SyncToolbarStates();
	void GetSceneMousePos(LPARAM lParam, long& sx, long& sy) const;
	void SelectTool(UINT cmdId, HWND hWnd);
	void CommitHistory(HWND hWnd, SceneSnapshot before);

	void BeginMoveDrag(HWND hWnd, long sx, long sy);
	void BeginResizeDrag(HWND hWnd, int handle);
	void BeginRotateDrag(HWND hWnd);
	bool TryStartSelectionInteraction(HWND hWnd, long sx, long sy);
	void FinishTextEdit(HWND hWnd, bool commit);

	template <typename MutateFn>
	void ApplyStyleChange(HWND hWnd, MutateFn mutate);

	void ChooseBorderColor(HWND hWnd);
	void ChooseFillColor(HWND hWnd);
	void ToggleNoBorder(HWND hWnd);
	void ToggleNoFill(HWND hWnd);
	void SetBorderWidth(HWND hWnd, int width);

	void DeleteSelectedShapes(HWND hWnd);
	void FlipSelectedShape(HWND hWnd, bool horizontal);
	void ApplyZOrderAction(HWND hWnd, UINT cmdId);

public:
	Editor() = default;
	~Editor() = default;

	Editor(const Editor&) = delete;
	Editor& operator=(const Editor&) = delete;

	bool IsTextEditing() const { return textEditor.IsActive(); }
	void OpenFile(HWND hWnd, const std::wstring& filePath);

	void OnCreate(HWND hWnd);
	void OnSize(HWND hWnd);
	LRESULT OnNotify(LPARAM lParam) const;

	void OnHScroll(HWND hWnd, WPARAM wParam);
	void OnVScroll(HWND hWnd, WPARAM wParam);
	void OnMouseWheel(HWND hWnd, WPARAM wParam);

	void OnLBDown(HWND hWnd, LPARAM lParam);
	void OnLBDblClk(HWND hWnd);
	void OnMouseMove(HWND hWnd, LPARAM lParam);
	void OnLBUp(HWND hWnd, LPARAM lParam);

	void OnPaint(HWND hWnd) const;
	void OnInitMenuPopup(HWND hWnd) const;
	bool OnCommand(HWND hWnd, WPARAM wParam);
	bool OnClose(HWND hWnd);
};