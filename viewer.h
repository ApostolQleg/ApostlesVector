#pragma once

#include "shape.h"

#define SCENE_WIDTH 2000
#define SCENE_HEIGHT 2000
#define INIT_SHAPES_CAPACITY 16

class Viewer
{
private:
	Shape** shapes = nullptr;
	int shapesCount = 0;
	int shapesCapacity = 0;

	Shape** redoShapes = nullptr;
	int redoCount = 0;
	int redoCapacity = 0;

	int scrollX = 0;
	int scrollY = 0;
	int topOffset = 0;

	void PushToArray(Shape**& arr, int& count, int& capacity, Shape* p);
	Shape* PopFromArray(Shape** arr, int& count);
	void ClearRedo();

	void HandleScroll(HWND hWnd, int barType, WPARAM wParam, int& scrollPos, int pageSize, int maxSize);

public:
	Viewer() {}
	~Viewer();

	Viewer(const Viewer&) = delete;
	Viewer& operator=(const Viewer&) = delete;

	void SetTopOffset(int offset) { topOffset = offset; }
	int GetTopOffset() const { return topOffset; }

	void AddShape(Shape* p, bool clearRedoHistory = true);
	void Clear();

	bool CanUndo() const { return shapesCount > 0; }
	bool CanRedo() const { return redoCount > 0; }
	bool Undo();
	bool Redo();

	int GetCount() const { return shapesCount; }
	Shape* GetShape(int index) const
	{
		if (index >= 0 && index < shapesCount)
			return shapes[index];
		return nullptr;
	}

	void UpdateScrollBars(HWND hWnd);
	void OnHScroll(HWND hWnd, WPARAM wParam);
	void OnVScroll(HWND hWnd, WPARAM wParam);
	void OnMouseWheel(HWND hWnd, WPARAM wParam);

	void ScreenToScene(long clientX, long clientY, long& sceneX, long& sceneY, bool clampToScene = true) const;
	void BeginCanvasDraw(HDC hdc, HWND hWnd) const;

	void Render(HWND hWnd) const;
};
