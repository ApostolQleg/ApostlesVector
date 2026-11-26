#include "pch.h"
#include "viewer.h"

SceneSnapshot SceneSnapshot::Clone() const
{
	SceneSnapshot copy;
	copy.shapes.reserve(shapes.size());
	for (const auto& s : shapes)
		copy.shapes.push_back(s ? s->Clone() : nullptr);
	copy.selectedIndices = selectedIndices;
	return copy;
}

void Viewer::RefreshSelectionFlags()
{
	for (auto& shape : shapes)
	{
		if (shape)
			shape->SetSelected(false, false);
	}

	const bool isGroup = (selectedIndices.size() > 1);
	for (int idx : selectedIndices)
	{
		if (Shape* shape = GetShape(idx))
			shape->SetSelected(true, isGroup);
	}
}

std::vector<bool> Viewer::BuildSelectionMask() const
{
	std::vector<bool> mask(shapes.size(), false);
	for (int idx : selectedIndices)
	{
		if (idx >= 0 && idx < static_cast<int>(shapes.size()))
			mask[idx] = true;
	}
	return mask;
}

void Viewer::RebuildSelectionFromMask(const std::vector<bool>& mask)
{
	selectedIndices.clear();
	for (int i = 0; i < static_cast<int>(mask.size()); ++i)
	{
		if (mask[i])
			selectedIndices.push_back(i);
	}
	RefreshSelectionFlags();
}

void Viewer::SetSelectionMarquee(bool active, long x1, long y1, long x2, long y2)
{
	hasMarquee = active;
	marqueeRect = { std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2) };
}

void Viewer::AddShape(std::unique_ptr<Shape> shape)
{
	if (shape)
		shapes.push_back(std::move(shape));
}

bool Viewer::DeleteSelected()
{
	if (selectedIndices.empty()) return false;
	const auto mask = BuildSelectionMask();

	std::vector<std::unique_ptr<Shape>> remaining;
	remaining.reserve(shapes.size() - selectedIndices.size());
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (!mask[i])
			remaining.push_back(std::move(shapes[i]));
	}

	shapes = std::move(remaining);
	selectedIndices.clear();
	RefreshSelectionFlags();
	return true;
}

void Viewer::Clear()
{
	shapes.clear();
	selectedIndices.clear();
	previewShape = nullptr;
	hasMarquee = false;
}

SceneSnapshot Viewer::CaptureSnapshot() const
{
	SceneSnapshot snap;
	snap.shapes.reserve(shapes.size());
	for (const auto& s : shapes)
	{
		auto clone = s ? s->Clone() : nullptr;
		if (clone) clone->SetSelected(false, false);
		snap.shapes.push_back(std::move(clone));
	}
	snap.selectedIndices = selectedIndices;
	return snap;
}

void Viewer::RestoreSnapshot(const SceneSnapshot& snapshot)
{
	shapes.clear();
	shapes.reserve(snapshot.shapes.size());
	for (const auto& s : snapshot.shapes)
		shapes.push_back(s ? s->Clone() : nullptr);

	selectedIndices = snapshot.selectedIndices;
	RefreshSelectionFlags();
}

bool Viewer::BringSelectedToFront()
{
	if (selectedIndices.empty() || shapes.size() <= 1) return false;
	const auto mask = BuildSelectionMask();

	std::vector<std::unique_ptr<Shape>> reordered;
	std::vector<int> newSel;
	reordered.reserve(shapes.size());

	bool changed = false;
	bool seenSelected = false;
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (mask[i]) seenSelected = true;
		else if (seenSelected) changed = true;
	}
	if (!changed) return false;

	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (!mask[i]) reordered.push_back(std::move(shapes[i]));
	}
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (mask[i])
		{
			newSel.push_back(static_cast<int>(reordered.size()));
			reordered.push_back(std::move(shapes[i]));
		}
	}

	shapes = std::move(reordered);
	selectedIndices = std::move(newSel);
	RefreshSelectionFlags();
	return true;
}

bool Viewer::SendSelectedToBack()
{
	if (selectedIndices.empty() || shapes.size() <= 1) return false;
	const auto mask = BuildSelectionMask();

	bool changed = false;
	bool seenUnselected = false;
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (!mask[i]) seenUnselected = true;
		else if (seenUnselected) changed = true;
	}
	if (!changed) return false;

	std::vector<std::unique_ptr<Shape>> reordered;
	std::vector<int> newSel;
	reordered.reserve(shapes.size());

	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (mask[i])
		{
			newSel.push_back(static_cast<int>(reordered.size()));
			reordered.push_back(std::move(shapes[i]));
		}
	}
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		if (!mask[i]) reordered.push_back(std::move(shapes[i]));
	}

	shapes = std::move(reordered);
	selectedIndices = std::move(newSel);
	RefreshSelectionFlags();
	return true;
}

bool Viewer::BringSelectedForward()
{
	if (selectedIndices.empty() || shapes.size() <= 1) return false;
	auto mask = BuildSelectionMask();
	bool changed = false;

	for (int i = static_cast<int>(shapes.size()) - 2; i >= 0; --i)
	{
		if (mask[i] && !mask[i + 1])
		{
			std::swap(shapes[i], shapes[i + 1]);
			mask[i] = false;
			mask[i + 1] = true;
			changed = true;
		}
	}

	if (changed) RebuildSelectionFromMask(mask);
	return changed;
}

bool Viewer::SendSelectedBackward()
{
	if (selectedIndices.empty() || shapes.size() <= 1) return false;
	auto mask = BuildSelectionMask();
	bool changed = false;

	for (int i = 1; i < static_cast<int>(shapes.size()); ++i)
	{
		if (mask[i] && !mask[i - 1])
		{
			std::swap(shapes[i], shapes[i - 1]);
			mask[i] = false;
			mask[i - 1] = true;
			changed = true;
		}
	}

	if (changed) RebuildSelectionFromMask(mask);
	return changed;
}

Shape* Viewer::GetShape(int index) const
{
	if (index >= 0 && index < static_cast<int>(shapes.size()))
		return shapes[index].get();
	return nullptr;
}

void Viewer::ClearSelection()
{
	selectedIndices.clear();
	RefreshSelectionFlags();
}

bool Viewer::IsShapeSelected(int index) const
{
	return std::find(selectedIndices.begin(), selectedIndices.end(), index) != selectedIndices.end();
}

void Viewer::SelectIndex(int index)
{
	selectedIndices.clear();
	if (index >= 0 && index < static_cast<int>(shapes.size()))
		selectedIndices.push_back(index);
	RefreshSelectionFlags();
}

void Viewer::SelectLastShape()
{
	if (!shapes.empty())
		SelectIndex(static_cast<int>(shapes.size()) - 1);
}

int Viewer::HitTestTopShape(long sceneX, long sceneY) const
{
	for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i)
	{
		if (shapes[i] && shapes[i]->HitTest(sceneX, sceneY))
			return i;
	}
	return -1;
}

int Viewer::SelectAt(long sceneX, long sceneY)
{
	const int hit = HitTestTopShape(sceneX, sceneY);
	if (hit >= 0)
	{
		SelectIndex(hit);
		return hit;
	}
	ClearSelection();
	return -1;
}

int Viewer::SelectInRect(long x1, long y1, long x2, long y2)
{
	const long l = std::min(x1, x2);
	const long r = std::max(x1, x2);
	const long t = std::min(y1, y2);
	const long b = std::max(y1, y2);

	selectedIndices.clear();
	for (int i = 0; i < static_cast<int>(shapes.size()); ++i)
	{
		if (!shapes[i]) continue;
		long sl = 0, st = 0, sr = 0, sb = 0;
		shapes[i]->GetBounds(sl, st, sr, sb);
		if (!(sr < l || sl > r || sb < t || st > b))
			selectedIndices.push_back(i);
	}

	RefreshSelectionFlags();
	return static_cast<int>(selectedIndices.size());
}

Shape* Viewer::GetSingleSelectedShape() const
{
	return (selectedIndices.size() == 1) ? GetShape(selectedIndices[0]) : nullptr;
}

Shape* Viewer::GetPrimarySelectedShape() const
{
	return !selectedIndices.empty() ? GetShape(selectedIndices[0]) : nullptr;
}

bool Viewer::GetSelectionBounds(long& l, long& t, long& r, long& b) const
{
	bool hasAny = false;
	for (int idx : selectedIndices)
	{
		const Shape* shape = GetShape(idx);
		if (!shape) continue;

		long sl = 0, st = 0, sr = 0, sb = 0;
		shape->GetBounds(sl, st, sr, sb);
		if (!hasAny)
		{
			l = sl; t = st; r = sr; b = sb;
			hasAny = true;
		}
		else
		{
			l = std::min(l, sl); t = std::min(t, st);
			r = std::max(r, sr); b = std::max(b, sb);
		}
	}
	return hasAny;
}

int Viewer::HitSelectionHandle(long sx, long sy) const
{
	if (selectedIndices.empty()) return -1;
	if (selectedIndices.size() == 1)
	{
		const Shape* sel = GetShape(selectedIndices[0]);
		return sel ? sel->HitHandle(sx, sy) : -1;
	}

	// Для групи повертаємо лише маркери масштабування (без маркера обертання)
	long l = 0, t = 0, r = 0, b = 0;
	if (!GetSelectionBounds(l, t, r, b)) return -1;
	return Shape::HitBoxHandle(l, t, r, b, sx, sy);
}

bool Viewer::IsPointInSelection(long sx, long sy) const
{
	if (selectedIndices.empty()) return false;

	const int hit = HitTestTopShape(sx, sy);
	if (hit >= 0 && IsShapeSelected(hit))
		return true;

	if (selectedIndices.size() > 1)
	{
		long l = 0, t = 0, r = 0, b = 0;
		if (GetSelectionBounds(l, t, r, b))
		{
			return (sx >= l - kSelectionMargin && sx <= r + kSelectionMargin &&
				sy >= t - kSelectionMargin && sy <= b + kSelectionMargin);
		}
	}
	return false;
}

bool Viewer::MoveSelectedBy(long dx, long dy)
{
	if (selectedIndices.empty() || (dx == 0 && dy == 0))
		return false;

	for (int idx : selectedIndices)
	{
		if (Shape* shape = GetShape(idx))
			shape->MoveBy(dx, dy);
	}
	return true;
}

void Viewer::InvalidateCanvas(HWND hWnd) const
{
	if (!hWnd) return;
	RECT rc = {};
	GetClientRect(hWnd, &rc);
	rc.top = topOffset;
	if (rc.bottom > rc.top)
		InvalidateRect(hWnd, &rc, FALSE);
}

void Viewer::UpdateScrollBars(HWND hWnd)
{
	if (!hWnd) return;
	RECT rc = {};
	GetClientRect(hWnd, &rc);

	const int clientW = std::max<int>(0, rc.right - rc.left);
	const int clientH = std::max<int>(0, (rc.bottom - rc.top) - topOffset);

	scrollX = std::clamp(scrollX, 0, std::max(0, kSceneWidth - clientW));
	scrollY = std::clamp(scrollY, 0, std::max(0, kSceneHeight - clientH));

	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

	si.nMin = 0; si.nMax = kSceneWidth - 1;
	si.nPage = static_cast<UINT>(clientW); si.nPos = scrollX;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	si.nMin = 0; si.nMax = kSceneHeight - 1;
	si.nPage = static_cast<UINT>(clientH); si.nPos = scrollY;
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
	const int pageStep = std::max(lineStep, pageSize - lineStep);

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

	newPos = std::clamp(newPos, 0, std::max(0, maxSize - pageSize));
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
	HandleScroll(hWnd, SB_HORZ, wParam, scrollX, rc.right - rc.left, kSceneWidth);
}

void Viewer::OnVScroll(HWND hWnd, WPARAM wParam)
{
	RECT rc = {};
	GetClientRect(hWnd, &rc);
	const int clientH = std::max<int>(0, (rc.bottom - rc.top) - topOffset);
	HandleScroll(hWnd, SB_VERT, wParam, scrollY, clientH, kSceneHeight);
}

void Viewer::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	const int step = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA) * 40;
	if (step == 0) return;

	RECT rc = {};
	GetClientRect(hWnd, &rc);

	if (LOWORD(wParam) & MK_SHIFT)
	{
		const int maxPos = std::max<int>(0, kSceneWidth - (rc.right - rc.left));
		const int newPos = std::clamp(scrollX + step, 0, maxPos);
		if (newPos != scrollX)
		{
			scrollX = newPos;
			SetScrollPos(hWnd, SB_HORZ, scrollX, TRUE);
			InvalidateCanvas(hWnd);
		}
	}
	else
	{
		const int clientH = std::max<int>(0, (rc.bottom - rc.top) - topOffset);
		const int maxPos = std::max(0, kSceneHeight - clientH);
		const int newPos = std::clamp(scrollY + step, 0, maxPos);
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
		sceneX = std::clamp<long>(sceneX, 0, kSceneWidth);
		sceneY = std::clamp<long>(sceneY, 0, kSceneHeight);
	}
}

void Viewer::SceneToClient(long sceneX, long sceneY, long& clientX, long& clientY) const
{
	clientX = sceneX - scrollX;
	clientY = sceneY + topOffset - scrollY;
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

		SetGraphicsMode(memDC, GM_ADVANCED);

		HBRUSH hOuterBrush = CreateSolidBrush(RGB(235, 235, 235));
		FillRect(memDC, &rc, hOuterBrush);
		DeleteObject(hOuterBrush);

		IntersectClipRect(memDC, 0, topOffset, rc.right, rc.bottom);
		SetViewportOrgEx(memDC, -scrollX, topOffset - scrollY, nullptr);

		HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
		HPEN hOldPen = static_cast<HPEN>(SelectObject(memDC, hBorderPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(memDC, GetStockObject(WHITE_BRUSH)));
		Rectangle(memDC, 0, 0, kSceneWidth, kSceneHeight);
		SelectObject(memDC, hOldBrush);
		SelectObject(memDC, hOldPen);
		DeleteObject(hBorderPen);

		for (const auto& shape : shapes)
		{
			if (shape) shape->Show(memDC);
		}

		// Живий попередній перегляд фігури в її остаточному вигляді
		if (previewShape)
			previewShape->Show(memDC);

		if (selectedIndices.size() > 1)
		{
			long gl = 0, gt = 0, gr = 0, gb = 0;
			if (GetSelectionBounds(gl, gt, gr, gb))
				Shape::DrawBoxSelectionFrame(memDC, gl, gt, gr, gb, true);
		}

		if (hasMarquee)
		{
			HPEN hMarqueePen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
			HPEN hPrevPen = static_cast<HPEN>(SelectObject(memDC, hMarqueePen));
			HBRUSH hPrevBrush = static_cast<HBRUSH>(SelectObject(memDC, GetStockObject(NULL_BRUSH)));
			SetBkMode(memDC, TRANSPARENT);
			Rectangle(memDC, marqueeRect.left, marqueeRect.top, marqueeRect.right, marqueeRect.bottom);
			SelectObject(memDC, hPrevBrush);
			SelectObject(memDC, hPrevPen);
			DeleteObject(hMarqueePen);
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