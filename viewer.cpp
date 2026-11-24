#include "pch.h"
#include "framework.h"
#include "viewer.h"

Viewer::~Viewer()
{
	Clear();
}

void Viewer::PushToArray(Shape**& arr, int& count, int& capacity, Shape* p)
{
	if (!p) return;

	if (count >= capacity)
	{
		int newCapacity = (capacity > 0) ? capacity * 2 : INIT_SHAPES_CAPACITY;
		if (newCapacity <= count)
			newCapacity = count + 1;

		Shape** newArr = new Shape * [newCapacity]();

		for (int i = 0; i < count && i < newCapacity; i++)
			newArr[i] = arr[i];

		delete[] arr;
		arr = newArr;
		capacity = newCapacity;
	}

	if (arr && count >= 0 && count < capacity)
		arr[count++] = p;
	else
		delete p;
}

Shape* Viewer::PopFromArray(Shape** arr, int& count)
{
	if (!arr || count <= 0) return nullptr;

	Shape* last = arr[--count];
	arr[count] = nullptr;
	return last;
}

void Viewer::ClearRedo()
{
	for (int i = 0; i < redoCount; i++)
		delete redoShapes[i];

	delete[] redoShapes;
	redoShapes = nullptr;
	redoCount = redoCapacity = 0;
}

void Viewer::AddShape(Shape* p, bool clearRedoHistory)
{
	if (!p) return;

	if (clearRedoHistory)
		ClearRedo();

	PushToArray(shapes, shapesCount, shapesCapacity, p);
}

void Viewer::Clear()
{
	for (int i = 0; i < shapesCount; i++)
		delete shapes[i];

	delete[] shapes;
	shapes = nullptr;
	shapesCount = shapesCapacity = 0;
	selectedIndex = -1;

	ClearRedo();
}

bool Viewer::Undo()
{
	Shape* last = PopFromArray(shapes, shapesCount);
	if (!last) return false;

	if (shapesCount <= selectedIndex)
		selectedIndex = -1;

	PushToArray(redoShapes, redoCount, redoCapacity, last);
	return true;
}

bool Viewer::Redo()
{
	Shape* restored = PopFromArray(redoShapes, redoCount);
	if (!restored) return false;

	PushToArray(shapes, shapesCount, shapesCapacity, restored);
	return true;
}

void Viewer::ClearSelection()
{
	for (int i = 0; i < shapesCount; i++)
	{
		if (shapes[i])
			shapes[i]->SetSelected(false);
	}
	selectedIndex = -1;
}

int Viewer::SelectAt(long sceneX, long sceneY)
{
	ClearSelection();

	// перевіряє з останнього об'єкта
	for (int i = shapesCount - 1; i >= 0; i--)
	{
		if (shapes[i] && shapes[i]->HitTest(sceneX, sceneY))
		{
			shapes[i]->SetSelected(true);
			selectedIndex = i;
			return i;
		}
	}

	return -1;
}

Shape* Viewer::GetSelectedShape() const
{
	if (selectedIndex >= 0 && selectedIndex < shapesCount)
		return shapes[selectedIndex];
	return nullptr;
}

bool Viewer::MoveSelectedBy(long dx, long dy)
{
	Shape* pSel = GetSelectedShape();
	if (!pSel || (dx == 0 && dy == 0))
		return false;

	pSel->MoveBy(dx, dy);
	return true;
}

void Viewer::UpdateScrollBars(HWND hWnd)
{
	if (!hWnd) return;

	RECT rc = {};
	GetClientRect(hWnd, &rc);

	int clientW = rc.right - rc.left;
	int clientH = (rc.bottom - rc.top) - topOffset;
	if (clientW < 0) clientW = 0;
	if (clientH < 0) clientH = 0;

	int maxScrollX = (SCENE_WIDTH > clientW) ? (SCENE_WIDTH - clientW) : 0;
	int maxScrollY = (SCENE_HEIGHT > clientH) ? (SCENE_HEIGHT - clientH) : 0;

	if (scrollX > maxScrollX) scrollX = maxScrollX;
	if (scrollX < 0) scrollX = 0;
	if (scrollY > maxScrollY) scrollY = maxScrollY;
	if (scrollY < 0) scrollY = 0;

	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

	si.nMin = 0;
	si.nMax = SCENE_WIDTH - 1;
	si.nPage = (UINT)clientW;
	si.nPos = scrollX;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	si.nMin = 0;
	si.nMax = SCENE_HEIGHT - 1;
	si.nPage = (UINT)clientH;
	si.nPos = scrollY;
	SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

void Viewer::HandleScroll(HWND hWnd, int barType, WPARAM wParam, int& scrollPos, int pageSize, int maxSize)
{
	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(hWnd, barType, &si);

	int newPos = scrollPos;
	const int lineStep = 20;
	int pageStep = (pageSize > lineStep) ? (pageSize - lineStep) : lineStep;

	switch (LOWORD(wParam))
	{
	case SB_TOP:        newPos = 0; break;
	case SB_BOTTOM:     newPos = maxSize; break;
	case SB_LINEUP:     newPos -= lineStep; break;
	case SB_LINEDOWN:   newPos += lineStep; break;
	case SB_PAGEUP:     newPos -= pageStep; break;
	case SB_PAGEDOWN:   newPos += pageStep; break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		newPos = si.nTrackPos;
		break;
	default:
		return;
	}

	int maxPos = (maxSize > pageSize) ? (maxSize - pageSize) : 0;
	if (newPos < 0) newPos = 0;
	if (newPos > maxPos) newPos = maxPos;

	if (newPos != scrollPos)
	{
		scrollPos = newPos;
		si.fMask = SIF_POS;
		si.nPos = scrollPos;
		SetScrollInfo(hWnd, barType, &si, TRUE);
		InvalidateRect(hWnd, nullptr, FALSE);
	}
}

void Viewer::OnHScroll(HWND hWnd, WPARAM wParam)
{
	RECT rc = {};
	GetClientRect(hWnd, &rc);
	int clientW = rc.right - rc.left;

	HandleScroll(hWnd, SB_HORZ, wParam, scrollX, clientW, SCENE_WIDTH);
}

void Viewer::OnVScroll(HWND hWnd, WPARAM wParam)
{
	RECT rc = {};
	GetClientRect(hWnd, &rc);
	int clientH = (rc.bottom - rc.top) - topOffset;
	if (clientH < 0) clientH = 0;

	HandleScroll(hWnd, SB_VERT, wParam, scrollY, clientH, SCENE_HEIGHT);
}

void Viewer::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	int delta = GET_WHEEL_DELTA_WPARAM(wParam);
	int step = -(delta / WHEEL_DELTA) * 40;
	if (step == 0) return;

	RECT rc = {};
	GetClientRect(hWnd, &rc);

	if (LOWORD(wParam) & MK_SHIFT)
	{
		int clientW = rc.right - rc.left;
		int maxPos = (SCENE_WIDTH > clientW) ? (SCENE_WIDTH - clientW) : 0;
		int newPos = scrollX + step;
		if (newPos < 0) newPos = 0;
		if (newPos > maxPos) newPos = maxPos;

		if (newPos != scrollX)
		{
			scrollX = newPos;
			SetScrollPos(hWnd, SB_HORZ, scrollX, TRUE);
			InvalidateRect(hWnd, nullptr, FALSE);
		}
	}
	else
	{
		int clientH = (rc.bottom - rc.top) - topOffset;
		if (clientH < 0) clientH = 0;
		int maxPos = (SCENE_HEIGHT > clientH) ? (SCENE_HEIGHT - clientH) : 0;
		int newPos = scrollY + step;
		if (newPos < 0) newPos = 0;
		if (newPos > maxPos) newPos = maxPos;

		if (newPos != scrollY)
		{
			scrollY = newPos;
			SetScrollPos(hWnd, SB_VERT, scrollY, TRUE);
			InvalidateRect(hWnd, nullptr, FALSE);
		}
	}
}

void Viewer::ScreenToScene(long clientX, long clientY, long& sceneX, long& sceneY, bool clampToScene) const
{
	sceneX = clientX + scrollX;
	sceneY = (clientY - topOffset) + scrollY;

	if (clampToScene)
	{
		if (sceneX < 0) sceneX = 0;
		if (sceneX > SCENE_WIDTH) sceneX = SCENE_WIDTH;
		if (sceneY < 0) sceneY = 0;
		if (sceneY > SCENE_HEIGHT) sceneY = SCENE_HEIGHT;
	}
}

void Viewer::BeginCanvasDraw(HDC hdc, HWND hWnd) const
{
	RECT rc = {};
	GetClientRect(hWnd, &rc);

	IntersectClipRect(hdc, 0, topOffset, rc.right, rc.bottom);
	SetViewportOrgEx(hdc, -scrollX, topOffset - scrollY, nullptr);
}

void Viewer::Render(HWND hWnd) const
{
	PAINTSTRUCT ps = {};
	HDC hdc = BeginPaint(hWnd, &ps);

	RECT rc = {};
	GetClientRect(hWnd, &rc);
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;

	if (width > 0 && height > topOffset)
	{
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		HBRUSH hOuterBrush = CreateSolidBrush(RGB(235, 235, 235));
		FillRect(memDC, &rc, hOuterBrush);
		DeleteObject(hOuterBrush);

		BeginCanvasDraw(memDC, hWnd);

		HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
		HPEN hOldPen = (HPEN)SelectObject(memDC, hBorderPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(WHITE_BRUSH));

		Rectangle(memDC, 0, 0, SCENE_WIDTH, SCENE_HEIGHT);

		SelectObject(memDC, hOldBrush);
		SelectObject(memDC, hOldPen);
		DeleteObject(hBorderPen);

		for (int i = 0; i < shapesCount; i++)
		{
			if (shapes[i])
				shapes[i]->Show(memDC);
		}

		SetViewportOrgEx(memDC, 0, 0, nullptr);
		SelectClipRgn(memDC, nullptr);

		BitBlt(hdc, 0, topOffset, width, height - topOffset, memDC, 0, topOffset, SRCCOPY);

		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);
	}

	EndPaint(hWnd, &ps);
}
