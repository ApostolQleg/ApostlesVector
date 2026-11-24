#pragma once

#include "shape.h"

constexpr int kSceneWidth = 2000;
constexpr int kSceneHeight = 2000;

class Viewer
{
private:
	std::vector<std::unique_ptr<Shape>> shapes;
	std::vector<int> selectedIndices;

	int scrollX = 0;
	int scrollY = 0;
	int topOffset = 0;

	void RefreshSelectionFlags();
	void HandleScroll(HWND hWnd, int barType, WPARAM wParam, int& scrollPos, int pageSize, int maxSize);

public:
	Viewer() = default;
	~Viewer() = default;

	Viewer(const Viewer&) = delete;
	Viewer& operator=(const Viewer&) = delete;

	void SetTopOffset(int offset) { topOffset = offset; }
	int GetTopOffset() const { return topOffset; }

	void AddShape(std::unique_ptr<Shape> shape);
	std::unique_ptr<Shape> RemoveLastShape();
	void ReplaceShapeAt(int index, std::unique_ptr<Shape> shape);
	void Clear();

	int HitTestTopShape(long sceneX, long sceneY) const;
	int SelectAt(long sceneX, long sceneY);
	int SelectInRect(long x1, long y1, long x2, long y2);
	void SelectIndex(int index);
	void SelectLastShape();
	void ClearSelection();

	bool IsShapeSelected(int index) const;
	bool HasSelection() const { return !selectedIndices.empty(); }
	int GetSelectionCount() const { return static_cast<int>(selectedIndices.size()); }
	const std::vector<int>& GetSelectedIndices() const { return selectedIndices; }
	int FindShapeIndex(const Shape* target) const;
	Shape* GetSingleSelectedShape() const;
	Shape* GetPrimarySelectedShape() const;

	bool GetSelectionBounds(long& l, long& t, long& r, long& b) const;
	int HitSelectionHandle(long sx, long sy) const;
	bool IsPointInSelection(long sx, long sy) const;
	bool MoveSelectedBy(long dx, long dy);

	int GetCount() const { return static_cast<int>(shapes.size()); }
	Shape* GetShape(int index) const
	{
		if (index >= 0 && index < static_cast<int>(shapes.size()))
			return shapes[index].get();
		return nullptr;
	}

	void UpdateScrollBars(HWND hWnd);
	void OnHScroll(HWND hWnd, WPARAM wParam);
	void OnVScroll(HWND hWnd, WPARAM wParam);
	void OnMouseWheel(HWND hWnd, WPARAM wParam);

	void ScreenToScene(long clientX, long clientY, long& sceneX, long& sceneY, bool clampToScene = true) const;
	void SceneToClient(long sceneX, long sceneY, long& clientX, long& clientY) const
	{
		clientX = sceneX - scrollX;
		clientY = sceneY + topOffset - scrollY;
	}
	void BeginCanvasDraw(HDC hdc, HWND hWnd) const;
	void InvalidateCanvas(HWND hWnd) const;

	void Render(HWND hWnd) const;
};
