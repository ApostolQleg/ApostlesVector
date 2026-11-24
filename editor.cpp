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
				auto before = result.target->Clone();
				result.target->SetText(result.text);
				if (idx >= 0)
					history.PushExecuted(std::make_unique<ModifyShapeCommand>(idx, std::move(before), result.target->Clone()));
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
	dragOrigState.reset();
	currentToolId = cmdId;
	isDrawing = false;
	rubberDrawn = false;
	isMoving = false;
	isResizing = false;
	changedDuringDrag = false;
	resizeHandle = -1;

	if (cmdId != IDM_SELECT && cmdId != IDM_MOVE)
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
	Shape* sel = viewer.GetSelectedShape();
	if (sel)
	{
		auto before = sel->Clone();
		mutate(sel);
		history.PushExecuted(std::make_unique<ModifyShapeCommand>(
			viewer.GetSelectedIndex(), std::move(before), sel->Clone()));
		files.MarkDirty(hWnd);
		viewer.InvalidateCanvas(hWnd);
	}
	ApplyCurrentStyle(currentShape.get());
}

void Editor::ChooseBorderColor(HWND hWnd)
{
	Shape* sel = viewer.GetSelectedShape();

	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = sel ? sel->GetBorderColor() : borderColor;
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
	Shape* sel = viewer.GetSelectedShape();

	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = sel ? sel->GetFillColor() : fillColor;
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
	Shape* sel = viewer.GetSelectedShape();
	hasBorder = sel ? !sel->HasBorder() : !hasBorder;
	ApplyStyleChange(hWnd, [this](Shape* s) {
		s->SetStyle(s->GetBorderColor(), s->GetFillColor(), hasBorder, s->IsFilled(), s->GetBorderWidth());
		});
}

void Editor::ToggleNoFill(HWND hWnd)
{
	Shape* sel = viewer.GetSelectedShape();
	hasFill = sel ? !sel->IsFilled() : !hasFill;
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

	SelectTool(IDM_LINE);
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
		if (Shape* sel = viewer.GetSelectedShape())
		{
			const int handle = sel->HitHandle(sx, sy);
			if (handle >= 0)
			{
				dragOrigState = sel->Clone();
				resizeHandle = handle;
				isResizing = true;
				changedDuringDrag = false;
				SetCapture(hWnd);
				return;
			}
		}
		viewer.SelectAt(sx, sy);
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (currentToolId == IDM_MOVE)
	{
		const int hit = viewer.SelectAt(sx, sy);
		if (hit >= 0)
		{
			dragOrigState = viewer.GetSelectedShape()->Clone();
			isMoving = true;
			changedDuringDrag = false;
			lastMoveX = sx;
			lastMoveY = sy;
			SetCapture(hWnd);
		}
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
	Text* textShape = dynamic_cast<Text*>(viewer.GetSelectedShape());
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
		if (Shape* sel = viewer.GetSelectedShape())
		{
			if (dragOrigState)
			{
				sel->ResizeFrom(*dragOrigState, resizeHandle, sx, sy);
				changedDuringDrag = true;
				viewer.InvalidateCanvas(hWnd);
			}
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

		if (changedDuringDrag && dragOrigState)
		{
			if (const Shape* sel = viewer.GetSelectedShape())
			{
				history.PushExecuted(std::make_unique<ModifyShapeCommand>(
					viewer.GetSelectedIndex(), std::move(dragOrigState), sel->Clone()));
				files.MarkDirty(hWnd);
			}
			changedDuringDrag = false;
		}
		dragOrigState.reset();
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

	auto next = currentShape->Clone();
	history.Execute(std::make_unique<AddShapeCommand>(std::move(currentShape)), viewer);
	files.MarkDirty(hWnd);
	currentShape = std::move(next);

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

	const Shape* sel = viewer.GetSelectedShape();
	const bool curHasBorder = sel ? sel->HasBorder() : hasBorder;
	const bool curHasFill = sel ? sel->IsFilled() : hasFill;
	const int curWidth = sel ? sel->GetBorderWidth() : borderWidth;

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
