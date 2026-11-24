#include "pch.h"
#include "framework.h"
#include "viewer.h"

void Viewer::AddShape(std::unique_ptr<Shape> shape)
{
	if (shape)
		shapes.push_back(std::move(shape));
}

std::unique_ptr<Shape> Viewer::RemoveLastShape()
{
	if (shapes.empty()) return nullptr;

	auto last = std::move(shapes.back());
	shapes.pop_back();

	if (selectedIndex >= static_cast<int>(shapes.size()))
		selectedIndex = -1;

	return last;
}

void Viewer::ReplaceShapeAt(int index, std::unique_ptr<Shape> shape)
{
	if (!shape || index < 0 || index >= static_cast<int>(shapes.size()))
		return;

	ClearSelection();
	shape->SetSelected(false);
	shapes[index] = std::move(shape);
}

void Viewer::Clear()
{
	shapes.clear();
	selectedIndex = -1;
}

void Viewer::ClearSelection()
{
	for (auto& shape : shapes)
	{
		if (shape)
			shape->SetSelected(false);
	}
	selectedIndex = -1;
}

int Viewer::FindShapeIndex(const Shape* target) const
{
	if (!target) return -1;
	for (int i = 0; i < static_cast<int>(shapes.size()); ++i)
	{
		if (shapes[i].get() == target)
			return i;
	}
	return -1;
}

int Viewer::SelectAt(long sceneX, long sceneY)
{
	ClearSelection();

	// Перевіряємо з останньої (верхньої) фігури
	for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i)
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
	return GetShape(selectedIndex);
}

bool Viewer::MoveSelectedBy(long dx, long dy)
{
	Shape* sel = GetSelectedShape();
	if (!sel || (dx == 0 && dy == 0))
		return false;

	sel->MoveBy(dx, dy);
	return true;
}

void Viewer::InvalidateCanvas(HWND hWnd) const
{
	if (!hWnd) return;

	RECT rc = {};
	GetClientRect(hWnd, &rc);
	rc.top = topOffset;
	if (rc.bottom > rc.top)
	{
		InvalidateRect(hWnd, &rc, FALSE);
	}
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

	const int maxScrollX = (kSceneWidth > clientW) ? (kSceneWidth - clientW) : 0;
	const int maxScrollY = (kSceneHeight > clientH) ? (kSceneHeight - clientH) : 0;

	if (scrollX > maxScrollX) scrollX = maxScrollX;
	if (scrollX < 0) scrollX = 0;
	if (scrollY > maxScrollY) scrollY = maxScrollY;
	if (scrollY < 0) scrollY = 0;

	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

	si.nMin = 0;
	si.nMax = kSceneWidth - 1;
	si.nPage = static_cast<UINT>(clientW);
	si.nPos = scrollX;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	si.nMin = 0;
	si.nMax = kSceneHeight - 1;
	si.nPage = static_cast<UINT>(clientH);
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
	constexpr int lineStep = 20;
	const int pageStep = (pageSize > lineStep) ? (pageSize - lineStep) : lineStep;

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

	const int maxPos = (maxSize > pageSize) ? (maxSize - pageSize) : 0;
	if (newPos < 0) newPos = 0;
	if (newPos > maxPos) newPos = maxPos;

	if (newPos != scrollPos)
	{
		scrollPos = newPos;
		si.fMask = SIF_POS;
		si.nPos = scrollPos;
		SetScrollInfo(hWnd, barType, &si, TRUE);
		InvalidateCanvas(hWnd);
	}
}

void Viewer::OnHScroll(HWND hWnd, WPARAM wParam)
{
	RECT rc = {};
	GetClientRect(hWnd, &rc);
	const int clientW = rc.right - rc.left;

	HandleScroll(hWnd, SB_HORZ, wParam, scrollX, clientW, kSceneWidth);
}

void Viewer::OnVScroll(HWND hWnd, WPARAM wParam)
{
	RECT rc = {};
	GetClientRect(hWnd, &rc);
	int clientH = (rc.bottom - rc.top) - topOffset;
	if (clientH < 0) clientH = 0;

	HandleScroll(hWnd, SB_VERT, wParam, scrollY, clientH, kSceneHeight);
}

void Viewer::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
	const int step = -(delta / WHEEL_DELTA) * 40;
	if (step == 0) return;

	RECT rc = {};
	GetClientRect(hWnd, &rc);

	if (LOWORD(wParam) & MK_SHIFT)
	{
		const int clientW = rc.right - rc.left;
		const int maxPos = (kSceneWidth > clientW) ? (kSceneWidth - clientW) : 0;
		int newPos = scrollX + step;
		if (newPos < 0) newPos = 0;
		if (newPos > maxPos) newPos = maxPos;

		if (newPos != scrollX)
		{
			scrollX = newPos;
			SetScrollPos(hWnd, SB_HORZ, scrollX, TRUE);
			InvalidateCanvas(hWnd);
		}
	}
	else
	{
		int clientH = (rc.bottom - rc.top) - topOffset;
		if (clientH < 0) clientH = 0;
		const int maxPos = (kSceneHeight > clientH) ? (kSceneHeight - clientH) : 0;
		int newPos = scrollY + step;
		if (newPos < 0) newPos = 0;
		if (newPos > maxPos) newPos = maxPos;

		if (newPos != scrollY)
		{
			scrollY = newPos;
			SetScrollPos(hWnd, SB_VERT, scrollY, TRUE);
			InvalidateCanvas(hWnd);
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
		if (sceneX > kSceneWidth) sceneX = kSceneWidth;
		if (sceneY < 0) sceneY = 0;
		if (sceneY > kSceneHeight) sceneY = kSceneHeight;
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
	const int width = rc.right - rc.left;
	const int height = rc.bottom - rc.top;

	if (width > 0 && height > topOffset)
	{
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
		HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDC, memBmp));

		HBRUSH hOuterBrush = CreateSolidBrush(RGB(235, 235, 235));
		FillRect(memDC, &rc, hOuterBrush);
		DeleteObject(hOuterBrush);

		BeginCanvasDraw(memDC, hWnd);

		HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
		HPEN hOldPen = static_cast<HPEN>(SelectObject(memDC, hBorderPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(memDC, GetStockObject(WHITE_BRUSH)));

		Rectangle(memDC, 0, 0, kSceneWidth, kSceneHeight);

		SelectObject(memDC, hOldBrush);
		SelectObject(memDC, hOldPen);
		DeleteObject(hBorderPen);

		for (const auto& shape : shapes)
		{
			if (shape)
				shape->Show(memDC);
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
