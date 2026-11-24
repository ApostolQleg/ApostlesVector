#include "pch.h"
#include "framework.h"
#include "editor.h"
#include "tools.h"

Editor* Editor::instance = nullptr;

Editor* Editor::GetInstance()
{
	if (!instance)
	{
		instance = new Editor();
	}
	return instance;
}

static void DrawSelectionMarquee(HDC hdc, long x1, long y1, long x2, long y2)
{
	HPEN hPen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
	HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
	HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

	Rectangle(hdc, x1, y1, x2, y2);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);
}

std::vector<ShapeSnapshot> Editor::CaptureSelectedSnapshots() const
{
	std::vector<ShapeSnapshot> snapshots;
	for (int idx : viewer.GetSelectedIndices())
	{
		if (const Shape* shape = viewer.GetShape(idx))
		{
			snapshots.push_back({ idx, shape->Clone() });
		}
	}
	return snapshots;
}

void Editor::BeginResizeDrag(HWND hWnd, int handle)
{
	dragOrigStates = CaptureSelectedSnapshots();
	if (dragOrigStates.empty()) return;

	viewer.GetSelectionBounds(dragOrigBounds.left, dragOrigBounds.top,
		dragOrigBounds.right, dragOrigBounds.bottom);
	resizeHandle = handle;
	isResizing = true;
	changedDuringDrag = false;
	SetCapture(hWnd);
}

void Editor::BeginMoveDrag(HWND hWnd, long sx, long sy)
{
	dragOrigStates = CaptureSelectedSnapshots();
	if (dragOrigStates.empty()) return;

	isMoving = true;
	changedDuringDrag = false;
	lastMoveX = sx;
	lastMoveY = sy;
	SetCapture(hWnd);
}

void Editor::FinishTextEdit(HWND hWnd, bool commit)
{
	TextEditResult result = textEditor.End(hWnd, commit);
	if (result.committed)
	{
		if (result.target)
		{
			if (result.text != result.target->GetText())
			{
				const int idx = viewer.FindShapeIndex(result.target);
				std::vector<ShapeSnapshot> before;
				std::vector<ShapeSnapshot> after;
				if (idx >= 0)
					before.push_back({ idx, result.target->Clone() });

				result.target->SetText(result.text);

				if (idx >= 0)
				{
					after.push_back({ idx, result.target->Clone() });
					history.PushExecuted(std::make_unique<ModifyShapesCommand>(std::move(before), std::move(after)));
				}
				files.MarkDirty(hWnd);
			}
		}
		else
		{
			auto newText = std::make_unique<Text>();
			newText->Set(result.sceneRect.left, result.sceneRect.top,
				result.sceneRect.right, result.sceneRect.bottom);
			ApplyCurrentStyle(newText.get());
			newText->SetText(result.text);

			history.Execute(std::make_unique<AddShapeCommand>(std::move(newText)), viewer);
			files.MarkDirty(hWnd);

			SelectTool(IDM_SELECT);
			viewer.SelectLastShape();
		}
	}

	viewer.InvalidateCanvas(hWnd);
}

void Editor::GetSceneMousePos(LPARAM lParam, long& sx, long& sy) const
{
	const int cx = static_cast<int>(static_cast<short>(LOWORD(lParam)));
	const int cy = static_cast<int>(static_cast<short>(HIWORD(lParam)));
	viewer.ScreenToScene(cx, cy, sx, sy, true);
}

void Editor::ApplyCurrentStyle(Shape* shape) const
{
	if (shape)
		shape->SetStyle(borderColor, fillColor, hasBorder, hasFill, borderWidth);
}

void Editor::StartTool(std::unique_ptr<Shape> shape, UINT cmdId)
{
	currentShape = std::move(shape);
	dragOrigStates.clear();
	currentToolId = cmdId;
	isDrawing = false;
	isSelectingArea = false;
	rubberDrawn = false;
	isMoving = false;
	isResizing = false;
	changedDuringDrag = false;
	resizeHandle = -1;

	if (cmdId != IDM_SELECT && cmdId != IDM_SELECT_AREA)
	{
		viewer.ClearSelection();
	}

	ApplyCurrentStyle(currentShape.get());
	toolbar.SetActiveTool(cmdId);
}

void Editor::SelectTool(UINT cmdId)
{
	StartTool(ToolRegistry::CreateShapeByCmdId(cmdId), cmdId);
}

template <typename MutateFn>
void Editor::ApplyStyleChange(HWND hWnd, MutateFn mutate)
{
	if (viewer.HasSelection())
	{
		auto before = CaptureSelectedSnapshots();
		for (int idx : viewer.GetSelectedIndices())
		{
			if (Shape* shape = viewer.GetShape(idx))
				mutate(shape);
		}
		auto after = CaptureSelectedSnapshots();
		history.PushExecuted(std::make_unique<ModifyShapesCommand>(std::move(before), std::move(after)));
		files.MarkDirty(hWnd);
		viewer.InvalidateCanvas(hWnd);
	}
	ApplyCurrentStyle(currentShape.get());
}

void Editor::ChooseBorderColor(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();

	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = primary ? primary->GetBorderColor() : borderColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		borderColor = cc.rgbResult;
		hasBorder = true;
		ApplyStyleChange(hWnd, [this](Shape* s) {
			s->SetStyle(borderColor, s->GetFillColor(), true, s->IsFilled(), s->GetBorderWidth());
			});
	}
}

void Editor::ChooseFillColor(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();

	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = primary ? primary->GetFillColor() : fillColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		fillColor = cc.rgbResult;
		hasFill = true;
		ApplyStyleChange(hWnd, [this](Shape* s) {
			s->SetStyle(s->GetBorderColor(), fillColor, s->HasBorder(), true, s->GetBorderWidth());
			});
	}
}

void Editor::ToggleNoBorder(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();
	hasBorder = primary ? !primary->HasBorder() : !hasBorder;
	ApplyStyleChange(hWnd, [this](Shape* s) {
		s->SetStyle(s->GetBorderColor(), s->GetFillColor(), hasBorder, s->IsFilled(), s->GetBorderWidth());
		});
}

void Editor::ToggleNoFill(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();
	hasFill = primary ? !primary->IsFilled() : !hasFill;
	ApplyStyleChange(hWnd, [this](Shape* s) {
		s->SetStyle(s->GetBorderColor(), s->GetFillColor(), s->HasBorder(), hasFill, s->GetBorderWidth());
		});
}

void Editor::SetBorderWidth(HWND hWnd, int width)
{
	borderWidth = width;
	hasBorder = true;
	ApplyStyleChange(hWnd, [width](Shape* s) {
		s->SetStyle(s->GetBorderColor(), s->GetFillColor(), true, s->IsFilled(), width);
		});
}

void Editor::OnCreate(HWND hWnd)
{
	auto hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hWnd, GWLP_HINSTANCE));

	toolbar.Create(hWnd, hInst);
	viewer.SetTopOffset(toolbar.GetHeight());
	viewer.UpdateScrollBars(hWnd);

	files.UpdateWindowTitle(hWnd);

	SelectTool(IDM_SELECT);
}

void Editor::OnSize(HWND hWnd)
{
	FinishTextEdit(hWnd, true);
	rubberDrawn = false;
	toolbar.OnSize(hWnd);
	viewer.SetTopOffset(toolbar.GetHeight());
	viewer.UpdateScrollBars(hWnd);
	viewer.InvalidateCanvas(hWnd);
}

LRESULT Editor::OnNotify(LPARAM lParam)
{
	return toolbar.OnNotify(lParam);
}

void Editor::OnHScroll(HWND hWnd, WPARAM wParam)
{
	FinishTextEdit(hWnd, true);
	rubberDrawn = false;
	viewer.OnHScroll(hWnd, wParam);
}

void Editor::OnVScroll(HWND hWnd, WPARAM wParam)
{
	FinishTextEdit(hWnd, true);
	rubberDrawn = false;
	viewer.OnVScroll(hWnd, wParam);
}

void Editor::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	FinishTextEdit(hWnd, true);
	rubberDrawn = false;
	viewer.OnMouseWheel(hWnd, wParam);
}

void Editor::OnLBDown(HWND hWnd, LPARAM lParam)
{
	if (textEditor.IsActive())
	{
		FinishTextEdit(hWnd, true);
		return;
	}

	const int cx = static_cast<int>(static_cast<short>(LOWORD(lParam)));
	const int cy = static_cast<int>(static_cast<short>(HIWORD(lParam)));
	if (cy < viewer.GetTopOffset()) return;

	long sx = 0, sy = 0;
	viewer.ScreenToScene(cx, cy, sx, sy, true);

	if (currentToolId == IDM_SELECT)
	{
		const int handle = viewer.HitSelectionHandle(sx, sy);
		if (handle >= 0)
		{
			BeginResizeDrag(hWnd, handle);
			return;
		}

		const int hit = viewer.HitTestTopShape(sx, sy);
		if (hit >= 0)
		{
			if (!viewer.IsShapeSelected(hit))
				viewer.SelectIndex(hit);

			BeginMoveDrag(hWnd, sx, sy);
			viewer.InvalidateCanvas(hWnd);
			return;
		}

		if (viewer.GetSelectionCount() > 1 && viewer.IsPointInSelection(sx, sy))
		{
			BeginMoveDrag(hWnd, sx, sy);
			return;
		}

		viewer.ClearSelection();
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (currentToolId == IDM_SELECT_AREA)
	{
		const int handle = viewer.HitSelectionHandle(sx, sy);
		if (handle >= 0)
		{
			BeginResizeDrag(hWnd, handle);
			return;
		}

		if (viewer.IsPointInSelection(sx, sy))
		{
			BeginMoveDrag(hWnd, sx, sy);
			return;
		}

		viewer.ClearSelection();
		isSelectingArea = true;
		rubberDrawn = false;
		xStart = rubberEndX = sx;
		yStart = rubberEndY = sy;
		SetCapture(hWnd);
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (!currentShape) return;

	isDrawing = true;
	rubberDrawn = false;
	xStart = sx;
	yStart = sy;

	currentShape->Set(xStart, yStart, xStart, yStart);
	ApplyCurrentStyle(currentShape.get());

	SetCapture(hWnd);
}

void Editor::OnLBDblClk(HWND hWnd)
{
	if (currentToolId != IDM_SELECT) return;
	Text* textShape = dynamic_cast<Text*>(viewer.GetSingleSelectedShape());
	if (!textShape) return;

	long l, t, r, b;
	textShape->GetBounds(l, t, r, b);
	textEditor.Begin(hWnd, viewer, textShape, l, t, r, b, borderWidth);
}

void Editor::OnMouseMove(HWND hWnd, LPARAM lParam)
{
	if (isResizing)
	{
		long sx = 0, sy = 0;
		GetSceneMousePos(lParam, sx, sy);

		if (dragOrigStates.size() == 1)
		{
			if (Shape* sel = viewer.GetShape(dragOrigStates[0].index))
			{
				sel->ResizeFrom(*dragOrigStates[0].state, resizeHandle, sx, sy);
				changedDuringDrag = true;
				viewer.InvalidateCanvas(hWnd);
			}
		}
		else if (dragOrigStates.size() > 1)
		{
			long newL, newT, newR, newB;
			Shape::CalcResizedBox(dragOrigBounds.left, dragOrigBounds.top,
				dragOrigBounds.right, dragOrigBounds.bottom,
				resizeHandle, sx, sy, newL, newT, newR, newB);

			for (const auto& item : dragOrigStates)
			{
				if (Shape* shape = viewer.GetShape(item.index))
				{
					shape->TransformInBox(*item.state,
						dragOrigBounds.left, dragOrigBounds.top,
						dragOrigBounds.right, dragOrigBounds.bottom,
						newL, newT, newR, newB);
				}
			}
			changedDuringDrag = true;
			viewer.InvalidateCanvas(hWnd);
		}
		return;
	}

	if (isMoving)
	{
		long sx = 0, sy = 0;
		GetSceneMousePos(lParam, sx, sy);
		const long dx = sx - lastMoveX;
		const long dy = sy - lastMoveY;

		if (viewer.MoveSelectedBy(dx, dy))
		{
			lastMoveX = sx;
			lastMoveY = sy;
			changedDuringDrag = true;
			viewer.InvalidateCanvas(hWnd);
		}
		return;
	}

	if (isSelectingArea)
	{
		HDC hdc = GetDC(hWnd);
		viewer.BeginCanvasDraw(hdc, hWnd);
		SetROP2(hdc, R2_NOTXORPEN);
		SetBkMode(hdc, TRANSPARENT);

		if (rubberDrawn)
			DrawSelectionMarquee(hdc, xStart, yStart, rubberEndX, rubberEndY);

		GetSceneMousePos(lParam, rubberEndX, rubberEndY);
		DrawSelectionMarquee(hdc, xStart, yStart, rubberEndX, rubberEndY);

		rubberDrawn = true;
		ReleaseDC(hWnd, hdc);
		return;
	}

	if (!isDrawing || !currentShape) return;

	HDC hdc = GetDC(hWnd);
	viewer.BeginCanvasDraw(hdc, hWnd);
	SetROP2(hdc, R2_NOTXORPEN);
	SetBkMode(hdc, TRANSPARENT);

	currentShape->SetRubber(true);

	if (rubberDrawn)
		currentShape->Show(hdc);

	long x = 0, y = 0;
	GetSceneMousePos(lParam, x, y);

	currentShape->Set(xStart, yStart, x, y);
	currentShape->Show(hdc);

	rubberDrawn = true;
	currentShape->SetRubber(false);

	ReleaseDC(hWnd, hdc);
}

void Editor::OnLBUp(HWND hWnd, LPARAM lParam)
{
	if (isResizing || isMoving)
	{
		isResizing = false;
		isMoving = false;
		ReleaseCapture();

		if (changedDuringDrag && !dragOrigStates.empty())
		{
			auto after = CaptureSelectedSnapshots();
			history.PushExecuted(std::make_unique<ModifyShapesCommand>(
				std::move(dragOrigStates), std::move(after)));
			files.MarkDirty(hWnd);
			changedDuringDrag = false;
		}
		dragOrigStates.clear();
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (isSelectingArea)
	{
		isSelectingArea = false;
		rubberDrawn = false;
		ReleaseCapture();

		long x = 0, y = 0;
		GetSceneMousePos(lParam, x, y);

		if (labs(x - xStart) < 3 && labs(y - yStart) < 3)
			viewer.SelectAt(x, y);
		else
			viewer.SelectInRect(xStart, yStart, x, y);

		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (!isDrawing || !currentShape) return;

	isDrawing = false;
	rubberDrawn = false;
	ReleaseCapture();

	long x = 0, y = 0;
	GetSceneMousePos(lParam, x, y);

	currentShape->Set(xStart, yStart, x, y);
	ApplyCurrentStyle(currentShape.get());
	currentShape->SetRubber(false);

	if (currentToolId == IDM_TEXT)
	{
		const long l = (xStart < x) ? xStart : x;
		const long r = (xStart < x) ? x : xStart;
		const long t = (yStart < y) ? yStart : y;
		const long b = (yStart < y) ? y : yStart;
		textEditor.Begin(hWnd, viewer, nullptr, l, t, r, b, borderWidth);
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	// Клік без перетягування не створює фігуру (крім Node, якій потрібна лише точка)
	if (currentToolId != IDM_NODE && labs(x - xStart) < 3 && labs(y - yStart) < 3)
	{
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	history.Execute(std::make_unique<AddShapeCommand>(std::move(currentShape)), viewer);
	files.MarkDirty(hWnd);

	SelectTool(IDM_SELECT);
	viewer.SelectLastShape();
	viewer.InvalidateCanvas(hWnd);
}

void Editor::OnPaint(HWND hWnd)
{
	rubberDrawn = false;
	viewer.Render(hWnd);
}

void Editor::OnInitMenuPopup(HWND hWnd)
{
	HMENU hMenu = GetMenu(hWnd);
	if (!hMenu) return;

	for (const auto& tool : ToolRegistry::GetAll())
	{
		CheckMenuItem(hMenu, tool.cmdId,
			MF_BYCOMMAND | ((tool.cmdId == currentToolId) ? MF_CHECKED : MF_UNCHECKED));
	}

	const Shape* primary = viewer.GetPrimarySelectedShape();
	const bool curHasBorder = primary ? primary->HasBorder() : hasBorder;
	const bool curHasFill = primary ? primary->IsFilled() : hasFill;
	const int curWidth = primary ? primary->GetBorderWidth() : borderWidth;

	CheckMenuItem(hMenu, IDM_NO_BORDER, MF_BYCOMMAND | ((!curHasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_NO_COLOR, MF_BYCOMMAND | ((!curHasFill) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_1, MF_BYCOMMAND | ((curWidth == 1 && curHasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_3, MF_BYCOMMAND | ((curWidth == 3 && curHasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_5, MF_BYCOMMAND | ((curWidth == 5 && curHasBorder) ? MF_CHECKED : MF_UNCHECKED));

	EnableMenuItem(hMenu, IDM_UNDO, MF_BYCOMMAND | (history.CanUndo() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_REDO, MF_BYCOMMAND | (history.CanRedo() ? MF_ENABLED : MF_GRAYED));
}

bool Editor::OnCommand(HWND hWnd, WPARAM wParam)
{
	const UINT wmId = LOWORD(wParam);
	if (wmId == kInplaceEditCtrlId)
	{
		if (HIWORD(wParam) == EN_KILLFOCUS)
			FinishTextEdit(hWnd, true);
		return true;
	}
	if (wmId == kCmdTextCommit)
	{
		FinishTextEdit(hWnd, true);
		return true;
	}
	if (wmId == kCmdTextCancel)
	{
		FinishTextEdit(hWnd, false);
		return true;
	}

	FinishTextEdit(hWnd, true);

	if (toolbar.IsToolId(wmId))
	{
		SelectTool(wmId);
		viewer.InvalidateCanvas(hWnd);
		return true;
	}

	switch (wmId)
	{
	case IDM_NEW:
		if (files.New(hWnd, viewer))
			history.Clear();
		return true;

	case IDM_OPEN:
		if (files.Open(hWnd, viewer))
			history.Clear();
		return true;

	case IDM_SAVE:
		files.Save(hWnd, viewer);
		return true;

	case IDM_SAVE_AS:
		files.SaveAs(hWnd, viewer);
		return true;

	case IDM_EXPORT_SVG:
		files.ExportSvg(hWnd, viewer);
		return true;

	case IDM_UNDO:
		if (history.Undo(viewer))
		{
			files.MarkDirty(hWnd);
			viewer.InvalidateCanvas(hWnd);
		}
		return true;

	case IDM_REDO:
		if (history.Redo(viewer))
		{
			files.MarkDirty(hWnd);
			viewer.InvalidateCanvas(hWnd);
		}
		return true;

	case IDM_BORDER_COLOR:
		ChooseBorderColor(hWnd);
		return true;

	case IDM_COLOR:
		ChooseFillColor(hWnd);
		return true;

	case IDM_NO_BORDER:
		ToggleNoBorder(hWnd);
		return true;

	case IDM_NO_COLOR:
		ToggleNoFill(hWnd);
		return true;

	case IDM_WIDTH_1:
		SetBorderWidth(hWnd, 1);
		return true;

	case IDM_WIDTH_3:
		SetBorderWidth(hWnd, 3);
		return true;

	case IDM_WIDTH_5:
		SetBorderWidth(hWnd, 5);
		return true;
	}

	return false;
}

bool Editor::OnClose(HWND hWnd)
{
	FinishTextEdit(hWnd, true);
	return files.ConfirmDiscardChanges(hWnd, viewer);
}
