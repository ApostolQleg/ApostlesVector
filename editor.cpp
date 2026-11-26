#include "pch.h"
#include "editor.h"
#include "tools.h"
#include "shape_text.h"

void Editor::SyncToolbarStates()
{
	toolbar.UpdateActionStates(viewer.GetSelectionCount());
}

void Editor::GetSceneMousePos(LPARAM lParam, long& sx, long& sy) const
{
	const int cx = static_cast<int>(static_cast<short>(LOWORD(lParam)));
	const int cy = static_cast<int>(static_cast<short>(HIWORD(lParam)));
	viewer.ScreenToScene(cx, cy, sx, sy, true);
}

void Editor::CommitHistory(HWND hWnd, SceneSnapshot before)
{
	history.PushExecuted(std::make_unique<SceneCommand>(std::move(before), viewer.CaptureSnapshot()));
	files.MarkDirty(hWnd);
	SyncToolbarStates();
	viewer.InvalidateCanvas(hWnd);
}

void Editor::SelectTool(UINT cmdId, HWND hWnd)
{
	currentToolId = cmdId;
	drawingShape.reset();
	viewer.SetPreviewShape(nullptr);
	viewer.SetSelectionMarquee(false);
	dragMode = DragMode::None;
	dragOrigShapes.clear();

	if (cmdId != IDM_SELECT && cmdId != IDM_SELECT_AREA)
		viewer.ClearSelection();

	toolbar.SetActiveTool(cmdId);
	SyncToolbarStates();
	viewer.InvalidateCanvas(hWnd);
}

void Editor::BeginMoveDrag(HWND hWnd, long sx, long sy)
{
	dragBeforeSnapshot = viewer.CaptureSnapshot();
	dragMode = DragMode::Move;
	changedDuringDrag = false;
	lastMoveX = sx;
	lastMoveY = sy;
	SetCapture(hWnd);
}

void Editor::BeginResizeDrag(HWND hWnd, int handle)
{
	dragBeforeSnapshot = viewer.CaptureSnapshot();
	dragOrigShapes.clear();
	for (int idx : viewer.GetSelectedIndices())
	{
		if (const Shape* shape = viewer.GetShape(idx))
			dragOrigShapes.emplace_back(idx, shape->Clone());
	}
	if (dragOrigShapes.empty()) return;

	viewer.GetSelectionBounds(dragOrigBounds.left, dragOrigBounds.top,
		dragOrigBounds.right, dragOrigBounds.bottom);
	activeHandle = handle;
	dragMode = DragMode::Resize;
	changedDuringDrag = false;
	SetCapture(hWnd);
}

void Editor::BeginRotateDrag(HWND hWnd)
{
	Shape* sel = viewer.GetSingleSelectedShape();
	if (!sel) return;

	dragBeforeSnapshot = viewer.CaptureSnapshot();
	dragOrigShapes.clear();
	dragOrigShapes.emplace_back(viewer.GetSelectedIndices()[0], sel->Clone());
	dragMode = DragMode::Rotate;
	changedDuringDrag = false;
	SetCapture(hWnd);
}

bool Editor::TryStartSelectionInteraction(HWND hWnd, long sx, long sy)
{
	const int handle = viewer.HitSelectionHandle(sx, sy);
	if (handle == kHandleRotate)
	{
		BeginRotateDrag(hWnd);
		return true;
	}
	if (handle >= 0)
	{
		BeginResizeDrag(hWnd, handle);
		return true;
	}
	if (viewer.IsPointInSelection(sx, sy))
	{
		BeginMoveDrag(hWnd, sx, sy);
		return true;
	}
	return false;
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
				auto before = viewer.CaptureSnapshot();
				result.target->SetText(result.text);
				CommitHistory(hWnd, std::move(before));
			}
		}
		else
		{
			auto before = viewer.CaptureSnapshot();
			auto newText = std::make_unique<Text>();
			newText->SetCoords(result.sceneRect.left, result.sceneRect.top,
				result.sceneRect.right, result.sceneRect.bottom);
			newText->SetStyle(activeStyle);
			newText->SetText(result.text);

			viewer.AddShape(std::move(newText));
			viewer.SelectLastShape(); // Залишаємо активним IDM_TEXT без перемикання на Select
			CommitHistory(hWnd, std::move(before));
		}
	}
	viewer.InvalidateCanvas(hWnd);
}

template <typename MutateFn>
void Editor::ApplyStyleChange(HWND hWnd, MutateFn mutate)
{
	if (!viewer.HasSelection()) return;

	auto before = viewer.CaptureSnapshot();
	for (int idx : viewer.GetSelectedIndices())
	{
		if (Shape* shape = viewer.GetShape(idx))
		{
			ShapeStyle s = shape->GetStyle();
			mutate(s);
			shape->SetStyle(s);
		}
	}
	CommitHistory(hWnd, std::move(before));
}

void Editor::ChooseBorderColor(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();
	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = primary ? primary->GetStyle().borderColor : activeStyle.borderColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		activeStyle.borderColor = cc.rgbResult;
		activeStyle.hasBorder = true;
		ApplyStyleChange(hWnd, [this](ShapeStyle& s) {
			s.borderColor = activeStyle.borderColor;
			s.hasBorder = true;
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
	cc.rgbResult = primary ? primary->GetStyle().fillColor : activeStyle.fillColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		activeStyle.fillColor = cc.rgbResult;
		activeStyle.hasFill = true;
		ApplyStyleChange(hWnd, [this](ShapeStyle& s) {
			s.fillColor = activeStyle.fillColor;
			s.hasFill = true;
			});
	}
}

void Editor::ToggleNoBorder(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();
	activeStyle.hasBorder = primary ? !primary->GetStyle().hasBorder : !activeStyle.hasBorder;
	ApplyStyleChange(hWnd, [this](ShapeStyle& s) { s.hasBorder = activeStyle.hasBorder; });
}

void Editor::ToggleNoFill(HWND hWnd)
{
	const Shape* primary = viewer.GetPrimarySelectedShape();
	activeStyle.hasFill = primary ? !primary->GetStyle().hasFill : !activeStyle.hasFill;
	ApplyStyleChange(hWnd, [this](ShapeStyle& s) { s.hasFill = activeStyle.hasFill; });
}

void Editor::SetBorderWidth(HWND hWnd, int width)
{
	activeStyle.borderWidth = width;
	activeStyle.hasBorder = true;
	ApplyStyleChange(hWnd, [width](ShapeStyle& s) {
		s.borderWidth = width;
		s.hasBorder = true;
		});
}

void Editor::DeleteSelectedShapes(HWND hWnd)
{
	if (!viewer.HasSelection()) return;
	auto before = viewer.CaptureSnapshot();
	if (viewer.DeleteSelected())
		CommitHistory(hWnd, std::move(before));
}

void Editor::FlipSelectedShape(HWND hWnd, bool horizontal)
{
	Shape* sel = viewer.GetSingleSelectedShape();
	if (!sel) return;

	auto before = viewer.CaptureSnapshot();
	if (horizontal) sel->FlipHorizontal();
	else sel->FlipVertical();
	CommitHistory(hWnd, std::move(before));
}

void Editor::ApplyZOrderAction(HWND hWnd, UINT cmdId)
{
	if (!viewer.HasSelection()) return;
	auto before = viewer.CaptureSnapshot();

	bool changed = false;
	switch (cmdId)
	{
	case IDM_Z_FRONT: changed = viewer.BringSelectedToFront(); break;
	case IDM_Z_UP:    changed = viewer.BringSelectedForward(); break;
	case IDM_Z_DOWN:  changed = viewer.SendSelectedBackward(); break;
	case IDM_Z_BACK:  changed = viewer.SendSelectedToBack(); break;
	}

	if (changed)
		CommitHistory(hWnd, std::move(before));
}

void Editor::OnCreate(HWND hWnd)
{
	auto hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hWnd, GWLP_HINSTANCE));
	toolbar.Create(hWnd, hInst);
	viewer.SetTopOffset(toolbar.GetHeight());
	viewer.UpdateScrollBars(hWnd);
	files.UpdateWindowTitle(hWnd);
	SelectTool(IDM_SELECT, hWnd);
}

void Editor::OnSize(HWND hWnd)
{
	FinishTextEdit(hWnd, true);
	toolbar.OnSize(hWnd);
	viewer.SetTopOffset(toolbar.GetHeight());
	viewer.UpdateScrollBars(hWnd);
	viewer.InvalidateCanvas(hWnd);
}

LRESULT Editor::OnNotify(LPARAM lParam) const
{
	return toolbar.OnNotify(lParam);
}

void Editor::OnHScroll(HWND hWnd, WPARAM wParam)
{
	FinishTextEdit(hWnd, true);
	viewer.OnHScroll(hWnd, wParam);
}

void Editor::OnVScroll(HWND hWnd, WPARAM wParam)
{
	FinishTextEdit(hWnd, true);
	viewer.OnVScroll(hWnd, wParam);
}

void Editor::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	FinishTextEdit(hWnd, true);
	viewer.OnMouseWheel(hWnd, wParam);
}

void Editor::OnLBDown(HWND hWnd, LPARAM lParam)
{
	if (textEditor.IsActive())
	{
		FinishTextEdit(hWnd, true);
		return;
	}

	const int cy = static_cast<int>(static_cast<short>(HIWORD(lParam)));
	if (cy < viewer.GetTopOffset()) return;

	long sx = 0, sy = 0;
	GetSceneMousePos(lParam, sx, sy);

	if (currentToolId == IDM_SELECT)
	{
		if (TryStartSelectionInteraction(hWnd, sx, sy))
			return;

		const int hit = viewer.HitTestTopShape(sx, sy);
		if (hit >= 0)
		{
			viewer.SelectIndex(hit);
			SyncToolbarStates();
			BeginMoveDrag(hWnd, sx, sy);
			viewer.InvalidateCanvas(hWnd);
			return;
		}

		viewer.ClearSelection();
		SyncToolbarStates();
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (currentToolId == IDM_SELECT_AREA)
	{
		if (TryStartSelectionInteraction(hWnd, sx, sy))
			return;

		viewer.ClearSelection();
		SyncToolbarStates();
		dragMode = DragMode::SelectArea;
		startX = sx; startY = sy;
		viewer.SetSelectionMarquee(true, sx, sy, sx, sy);
		SetCapture(hWnd);
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	// У режимі малювання дозволяємо одразу редагувати щойно виділену фігуру
	if (viewer.HasSelection() && TryStartSelectionInteraction(hWnd, sx, sy))
		return;

	viewer.ClearSelection();
	SyncToolbarStates();

	drawingShape = ToolRegistry::CreateShapeByCmdId(currentToolId);
	if (!drawingShape) return;

	startX = sx; startY = sy;
	drawingShape->SetCoords(sx, sy, sx, sy);
	drawingShape->SetStyle(activeStyle);
	viewer.SetPreviewShape(drawingShape.get());

	dragMode = DragMode::Draw;
	SetCapture(hWnd);
	viewer.InvalidateCanvas(hWnd);
}

void Editor::OnLBDblClk(HWND hWnd)
{
	Text* textShape = dynamic_cast<Text*>(viewer.GetSingleSelectedShape());
	if (!textShape) return;

	long l = 0, t = 0, r = 0, b = 0;
	textShape->GetLocalBounds(l, t, r, b);
	textEditor.Begin(hWnd, viewer, textShape, l, t, r, b, activeStyle.borderWidth);
}

void Editor::OnMouseMove(HWND hWnd, LPARAM lParam)
{
	if (dragMode == DragMode::None) return;

	long sx = 0, sy = 0;
	GetSceneMousePos(lParam, sx, sy);

	switch (dragMode)
	{
	case DragMode::Rotate:
		if (!dragOrigShapes.empty())
		{
			if (Shape* sel = viewer.GetShape(dragOrigShapes[0].first))
			{
				sel->RotateToPoint(*dragOrigShapes[0].second, sx, sy);
				changedDuringDrag = true;
				viewer.InvalidateCanvas(hWnd);
			}
		}
		break;

	case DragMode::Resize:
		if (dragOrigShapes.size() == 1)
		{
			if (Shape* sel = viewer.GetShape(dragOrigShapes[0].first))
			{
				sel->ResizeFrom(*dragOrigShapes[0].second, activeHandle, sx, sy);
				changedDuringDrag = true;
				viewer.InvalidateCanvas(hWnd);
			}
		}
		else if (dragOrigShapes.size() > 1)
		{
			long newL = 0, newT = 0, newR = 0, newB = 0;
			Shape::CalcResizedBox(dragOrigBounds.left, dragOrigBounds.top,
				dragOrigBounds.right, dragOrigBounds.bottom,
				activeHandle, sx, sy, newL, newT, newR, newB);

			for (const auto& [idx, orig] : dragOrigShapes)
			{
				if (Shape* shape = viewer.GetShape(idx))
				{
					shape->TransformInBox(*orig,
						dragOrigBounds.left, dragOrigBounds.top,
						dragOrigBounds.right, dragOrigBounds.bottom,
						newL, newT, newR, newB);
				}
			}
			changedDuringDrag = true;
			viewer.InvalidateCanvas(hWnd);
		}
		break;

	case DragMode::Move:
		if (viewer.MoveSelectedBy(sx - lastMoveX, sy - lastMoveY))
		{
			lastMoveX = sx;
			lastMoveY = sy;
			changedDuringDrag = true;
			viewer.InvalidateCanvas(hWnd);
		}
		break;

	case DragMode::SelectArea:
		viewer.SetSelectionMarquee(true, startX, startY, sx, sy);
		viewer.InvalidateCanvas(hWnd);
		break;

	case DragMode::Draw:
		if (drawingShape)
		{
			drawingShape->SetCoords(startX, startY, sx, sy);
			viewer.InvalidateCanvas(hWnd);
		}
		break;

	default:
		break;
	}
}

void Editor::OnLBUp(HWND hWnd, LPARAM lParam)
{
	if (dragMode == DragMode::None) return;

	const DragMode finishedMode = dragMode;
	dragMode = DragMode::None;
	ReleaseCapture();

	long sx = 0, sy = 0;
	GetSceneMousePos(lParam, sx, sy);

	if (finishedMode == DragMode::Move || finishedMode == DragMode::Resize || finishedMode == DragMode::Rotate)
	{
		if (changedDuringDrag)
			CommitHistory(hWnd, std::move(dragBeforeSnapshot));
		dragOrigShapes.clear();
		changedDuringDrag = false;
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (finishedMode == DragMode::SelectArea)
	{
		viewer.SetSelectionMarquee(false);
		if (labs(sx - startX) < 3 && labs(sy - startY) < 3)
			viewer.SelectAt(sx, sy);
		else
			viewer.SelectInRect(startX, startY, sx, sy);

		SyncToolbarStates();
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (finishedMode == DragMode::Draw && drawingShape)
	{
		drawingShape->SetCoords(startX, startY, sx, sy);
		viewer.SetPreviewShape(nullptr);

		if (currentToolId == IDM_TEXT)
		{
			drawingShape.reset();
			textEditor.Begin(hWnd, viewer, nullptr,
				std::min(startX, sx), std::min(startY, sy),
				std::max(startX, sx), std::max(startY, sy), activeStyle.borderWidth);
			viewer.InvalidateCanvas(hWnd);
			return;
		}

		if (currentToolId != IDM_NODE && labs(sx - startX) < 3 && labs(sy - startY) < 3)
		{
			drawingShape.reset();
			viewer.InvalidateCanvas(hWnd);
			return;
		}

		auto before = viewer.CaptureSnapshot();
		viewer.AddShape(std::move(drawingShape));
		viewer.SelectLastShape(); // Фігура виділена, але поточний інструмент НЕ змінюється
		CommitHistory(hWnd, std::move(before));
	}
}

void Editor::OnPaint(HWND hWnd) const
{
	viewer.Render(hWnd);
}

void Editor::OnInitMenuPopup(HWND hWnd) const
{
	HMENU hMenu = GetMenu(hWnd);
	if (!hMenu) return;

	for (const auto& tool : ToolRegistry::GetAll())
	{
		if (tool.kind == ToolbarButtonKind::ModeTool)
		{
			CheckMenuItem(hMenu, tool.cmdId,
				MF_BYCOMMAND | ((tool.cmdId == currentToolId) ? MF_CHECKED : MF_UNCHECKED));
		}
	}

	const Shape* primary = viewer.GetPrimarySelectedShape();
	const ShapeStyle& st = primary ? primary->GetStyle() : activeStyle;

	CheckMenuItem(hMenu, IDM_NO_BORDER, MF_BYCOMMAND | (!st.hasBorder ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_NO_COLOR, MF_BYCOMMAND | (!st.hasFill ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_1, MF_BYCOMMAND | ((st.borderWidth == 1 && st.hasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_3, MF_BYCOMMAND | ((st.borderWidth == 3 && st.hasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_5, MF_BYCOMMAND | ((st.borderWidth == 5 && st.hasBorder) ? MF_CHECKED : MF_UNCHECKED));

	EnableMenuItem(hMenu, IDM_UNDO, MF_BYCOMMAND | (history.CanUndo() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_REDO, MF_BYCOMMAND | (history.CanRedo() ? MF_ENABLED : MF_GRAYED));

	const int selCount = viewer.GetSelectionCount();
	EnableMenuItem(hMenu, IDM_DELETE, MF_BYCOMMAND | (selCount >= 1 ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_FLIP_H, MF_BYCOMMAND | (selCount == 1 ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_FLIP_V, MF_BYCOMMAND | (selCount == 1 ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_Z_FRONT, MF_BYCOMMAND | (selCount >= 1 ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_Z_UP, MF_BYCOMMAND | (selCount >= 1 ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_Z_DOWN, MF_BYCOMMAND | (selCount >= 1 ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_Z_BACK, MF_BYCOMMAND | (selCount >= 1 ? MF_ENABLED : MF_GRAYED));
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
	if (wmId == kCmdTextCommit) { FinishTextEdit(hWnd, true); return true; }
	if (wmId == kCmdTextCancel) { FinishTextEdit(hWnd, false); return true; }

	FinishTextEdit(hWnd, true);

	if (ToolRegistry::IsModeToolId(wmId))
	{
		SelectTool(wmId, hWnd);
		return true;
	}

	switch (wmId)
	{
	case IDM_NEW:
		if (files.New(hWnd, viewer)) { history.Clear(); SyncToolbarStates(); }
		return true;
	case IDM_OPEN:
		if (files.Open(hWnd, viewer)) { history.Clear(); SyncToolbarStates(); }
		return true;
	case IDM_SAVE:        files.Save(hWnd, viewer); return true;
	case IDM_SAVE_AS:     files.SaveAs(hWnd, viewer); return true;
	case IDM_EXPORT_SVG:  files.ExportSvg(hWnd, viewer); return true;

	case IDM_UNDO:
		if (history.Undo(viewer))
		{
			files.MarkDirty(hWnd);
			SyncToolbarStates();
			viewer.InvalidateCanvas(hWnd);
		}
		return true;

	case IDM_REDO:
		if (history.Redo(viewer))
		{
			files.MarkDirty(hWnd);
			SyncToolbarStates();
			viewer.InvalidateCanvas(hWnd);
		}
		return true;

	case IDM_DELETE:      DeleteSelectedShapes(hWnd); return true;
	case IDM_FLIP_H:      FlipSelectedShape(hWnd, true); return true;
	case IDM_FLIP_V:      FlipSelectedShape(hWnd, false); return true;
	case IDM_Z_FRONT:
	case IDM_Z_UP:
	case IDM_Z_DOWN:
	case IDM_Z_BACK:      ApplyZOrderAction(hWnd, wmId); return true;

	case IDM_BORDER_COLOR: ChooseBorderColor(hWnd); return true;
	case IDM_COLOR:        ChooseFillColor(hWnd); return true;
	case IDM_NO_BORDER:    ToggleNoBorder(hWnd); return true;
	case IDM_NO_COLOR:     ToggleNoFill(hWnd); return true;
	case IDM_WIDTH_1:      SetBorderWidth(hWnd, 1); return true;
	case IDM_WIDTH_3:      SetBorderWidth(hWnd, 3); return true;
	case IDM_WIDTH_5:      SetBorderWidth(hWnd, 5); return true;
	}

	return false;
}

bool Editor::OnClose(HWND hWnd)
{
	FinishTextEdit(hWnd, true);
	return files.ConfirmDiscardChanges(hWnd, viewer);
}