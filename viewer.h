#pragma once

#include "shape.h"

constexpr int kSceneWidth = 2000;
constexpr int kSceneHeight = 2000;

struct SceneSnapshot
{
	std::vector<std::unique_ptr<Shape>> shapes;
	std::vector<int> selectedIndices;

	SceneSnapshot Clone() const;
};

class Viewer
{
private:
	std::vector<std::unique_ptr<Shape>> shapes;
	std::vector<int> selectedIndices;

	const Shape* previewShape = nullptr;
	bool hasMarquee = false;
	RECT marqueeRect = {};

	int scrollX = 0;
	int scrollY = 0;
	int topOffset = 0;

	void RefreshSelectionFlags();
	void HandleScroll(HWND hWnd, int barType, WPARAM wParam, int& scrollPos, int pageSize, int maxSize);
	std::vector<bool> BuildSelectionMask() const;
	void RebuildSelectionFromMask(const std::vector<bool>& mask);

public:
	Viewer() = default;
	~Viewer() = default;

	Viewer(const Viewer&) = delete;
	Viewer& operator=(const Viewer&) = delete;

	void SetTopOffset(int offset) { topOffset = offset; }
	int GetTopOffset() const { return topOffset; }

	void SetPreviewShape(const Shape* shape) { previewShape = shape; }
	void SetSelectionMarquee(bool active, long x1 = 0, long y1 = 0, long x2 = 0, long y2 = 0);

	void AddShape(std::unique_ptr<Shape> shape);
	bool DeleteSelected();
	void Clear();

	SceneSnapshot CaptureSnapshot() const;
	void RestoreSnapshot(const SceneSnapshot& snapshot);

	bool BringSelectedToFront();
	bool SendSelectedToBack();
	bool BringSelectedForward();
	bool SendSelectedBackward();

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
	Shape* GetSingleSelectedShape() const;
	Shape* GetPrimarySelectedShape() const;

	bool GetSelectionBounds(long& l, long& t, long& r, long& b) const;
	int HitSelectionHandle(long sx, long sy) const;
	bool IsPointInSelection(long sx, long sy) const;
	bool MoveSelectedBy(long dx, long dy);

	int GetCount() const { return static_cast<int>(shapes.size()); }
	Shape* GetShape(int index) const;

	void UpdateScrollBars(HWND hWnd);
	void OnHScroll(HWND hWnd, WPARAM wParam);
	void OnVScroll(HWND hWnd, WPARAM wParam);
	void OnMouseWheel(HWND hWnd, WPARAM wParam);

	void ScreenToScene(long clientX, long clientY, long& sceneX, long& sceneY, bool clampToScene = true) const;
	void SceneToClient(long sceneX, long sceneY, long& clientX, long& clientY) const;
	void InvalidateCanvas(HWND hWnd) const;

	void Render(HWND hWnd) const;
};