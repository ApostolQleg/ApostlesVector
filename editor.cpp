#include "pch.h"
#include "framework.h"
#include "editor.h"

Editor* Editor::p_instance = nullptr;

Editor* Editor::getInstance()
{
	if (!p_instance)
	{
		p_instance = new Editor();
	}
	return p_instance;
}

Editor::~Editor()
{
	delete pcurrent;
	pcurrent = nullptr;
	delete resizeOrig;
	resizeOrig = nullptr;
	if (hEditFont)
	{
		DeleteObject(hEditFont);
		hEditFont = nullptr;
	}
}

static LRESULT CALLBACK InplaceEditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR idSubclass, DWORD_PTR)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			PostMessageW(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(IDM_TEXT_CANCEL, 0), 0);
			return 0;
		}
		if (wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000))
		{
			PostMessageW(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(IDM_TEXT_COMMIT, 0), 0);
			return 0;
		}
		break;

	case WM_CHAR:
		if (wParam == VK_ESCAPE || wParam == L'\n') return 0;   // без сигналу
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hEdit, InplaceEditProc, idSubclass);
		break;
	}
	return DefSubclassProc(hEdit, msg, wParam, lParam);
}

void Editor::BeginTextEdit(HWND hWnd, Text* target, long l, long t, long r, long b)
{
	EndTextEdit(hWnd, true);

	if (r - l < 180) r = l + 180;
	if (b - t < 60)  b = t + 60;
	editTarget = target;
	editSceneRect = { l, t, r, b };

	long cx = 0, cy = 0;
	viewer.SceneToClient(l, t, cx, cy);

	const int bw = target ? target->GetBorderWidth() : borderWidth;
	hEditFont = CreateFontW(16 + bw * 4, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

	// у фігурі переноси зберігаються як '\n', а EDIT потребує "\r\n"
	std::wstring initial;
	if (target)
	{
		for (wchar_t ch : target->GetText())
		{
			if (ch == L'\n') initial += L"\r\n";
			else initial += ch;
		}
	}

	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	hEdit = CreateWindowExW(0, L"EDIT", initial.c_str(),
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
		cx, cy, (int)(r - l), (int)(b - t),
		hWnd, (HMENU)(INT_PTR)IDC_INPLACE_EDIT, hInst, nullptr);

	if (!hEdit)
	{
		DeleteObject(hEditFont);
		hEditFont = nullptr;
		return;
	}

	SendMessageW(hEdit, WM_SETFONT, (WPARAM)hEditFont, TRUE);
	SendMessageW(hEdit, EM_LIMITTEXT, 1000, 0);
	SetWindowSubclass(hEdit, InplaceEditProc, 1, 0);
	SetFocus(hEdit);
	SendMessageW(hEdit, EM_SETSEL, 0, -1);
}

void Editor::EndTextEdit(HWND hWnd, bool commit)
{
	if (!hEdit) return;

	HWND hLocal = hEdit;
	hEdit = nullptr;               // захист від повторного входу через EN_KILLFOCUS

	std::wstring str;
	if (commit)
	{
		int len = GetWindowTextLengthW(hLocal);
		str.resize(len);
		if (len > 0) GetWindowTextW(hLocal, &str[0], len + 1);
		std::wstring clean;
		for (wchar_t ch : str)
			if (ch != L'\r') clean += ch;
		str = clean;
	}

	DestroyWindow(hLocal);
	if (hEditFont)
	{
		DeleteObject(hEditFont);
		hEditFont = nullptr;
	}

	Text* target = editTarget;
	editTarget = nullptr;

	if (commit && !str.empty())
	{
		if (target)
		{
			if (str != target->GetText())
			{
				target->SetText(str);
				files.MarkDirty(hWnd);
			}
		}
		else
		{
			Text* pNew = new Text();
			pNew->Set(editSceneRect.left, editSceneRect.top,
				editSceneRect.right, editSceneRect.bottom);
			ApplyCurrentStyle(pNew);
			pNew->SetText(str);
			viewer.AddShape(pNew);
			files.MarkDirty(hWnd);
		}
	}

	if (!GetFocus()) SetFocus(hWnd);
	viewer.InvalidateCanvas(hWnd);
}

void Editor::GetSceneMousePos(LPARAM lParam, long& sx, long& sy) const
{
	int cx = (int)(short)LOWORD(lParam);
	int cy = (int)(short)HIWORD(lParam);
	viewer.ScreenToScene(cx, cy, sx, sy, true);
}

void Editor::ApplyCurrentStyle(Shape* pShape) const
{
	if (pShape)
		pShape->SetStyle(borderColor, fillColor, hasBorder, hasFill, borderWidth);
}

void Editor::StartTool(Shape* pShape, UINT cmdId)
{
	delete pcurrent;
	pcurrent = pShape;
	delete resizeOrig;
	resizeOrig = nullptr;
	currentToolId = cmdId;
	isDrawing = false;
	rubberDrawn = false;
	isMoving = false;
	movedAny = false;
	isResizing = false;
	resizeHandle = -1;

	if (cmdId != IDM_SELECT && cmdId != IDM_MOVE)
	{
		viewer.ClearSelection();
	}

	ApplyCurrentStyle(pcurrent);
	toolbar.SetActiveTool(cmdId);
}

void Editor::SelectTool(UINT cmdId)
{
	switch (cmdId)
	{
	case IDM_SELECT:      StartTool(nullptr, cmdId);          break;
	case IDM_MOVE:        StartTool(nullptr, cmdId);          break;
	case IDM_NODE:        StartTool(new Node(), cmdId);       break;
	case IDM_EDGE:        StartTool(new Edge(), cmdId);       break;
	case IDM_LINE:        StartTool(new Line(), cmdId);       break;
	case IDM_ARROW:       StartTool(new Arrow(), cmdId);      break;
	case IDM_TRNGL:       StartTool(new Trngl(), cmdId);      break;
	case IDM_RIGHT_TRNGL: StartTool(new RightTrngl(), cmdId); break;
	case IDM_RECT:        StartTool(new Rect(), cmdId);       break;
	case IDM_ROMB:        StartTool(new Romb(), cmdId);       break;
	case IDM_PENT:        StartTool(new Pent(), cmdId);       break;
	case IDM_HEX:         StartTool(new Hex(), cmdId);        break;
	case IDM_ELIPS:       StartTool(new Elips(), cmdId);      break;
	case IDM_CUBE:        StartTool(new Cube(), cmdId);       break;
	case IDM_CYL:         StartTool(new Cyl(), cmdId);        break;
	case IDM_TEXT:        StartTool(new Text(), cmdId);       break;
	}
}

void Editor::ChooseBorderColor(HWND hWnd)
{
	Shape* pSel = viewer.GetSelectedShape();

	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = pSel ? pSel->GetBorderColor() : borderColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		borderColor = cc.rgbResult;
		hasBorder = true;

		if (pSel)
		{
			pSel->SetStyle(borderColor, pSel->GetFillColor(), true, pSel->IsFilled(), pSel->GetBorderWidth());
			files.MarkDirty(hWnd);
			viewer.InvalidateCanvas(hWnd);
		}
		ApplyCurrentStyle(pcurrent);
	}
}

void Editor::ChooseFillColor(HWND hWnd)
{
	Shape* pSel = viewer.GetSelectedShape();

	CHOOSECOLORW cc = {};
	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = customColors;
	cc.rgbResult = pSel ? pSel->GetFillColor() : fillColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColorW(&cc))
	{
		fillColor = cc.rgbResult;
		hasFill = true;

		if (pSel)
		{
			pSel->SetStyle(pSel->GetBorderColor(), fillColor, pSel->HasBorder(), true, pSel->GetBorderWidth());
			files.MarkDirty(hWnd);
			viewer.InvalidateCanvas(hWnd);
		}
		ApplyCurrentStyle(pcurrent);
	}
}

void Editor::ToggleNoBorder(HWND hWnd)
{
	Shape* pSel = viewer.GetSelectedShape();
	if (pSel)
	{
		bool nextBorder = !pSel->HasBorder();
		pSel->SetStyle(pSel->GetBorderColor(), pSel->GetFillColor(), nextBorder, pSel->IsFilled(), pSel->GetBorderWidth());
		hasBorder = nextBorder;
		files.MarkDirty(hWnd);
		viewer.InvalidateCanvas(hWnd);
	}
	else
	{
		hasBorder = !hasBorder;
	}
	ApplyCurrentStyle(pcurrent);
}

void Editor::ToggleNoFill(HWND hWnd)
{
	Shape* pSel = viewer.GetSelectedShape();
	if (pSel)
	{
		bool nextFill = !pSel->IsFilled();
		pSel->SetStyle(pSel->GetBorderColor(), pSel->GetFillColor(), pSel->HasBorder(), nextFill, pSel->GetBorderWidth());
		hasFill = nextFill;
		files.MarkDirty(hWnd);
		viewer.InvalidateCanvas(hWnd);
	}
	else
	{
		hasFill = !hasFill;
	}
	ApplyCurrentStyle(pcurrent);
}

void Editor::SetBorderWidth(HWND hWnd, int width)
{
	borderWidth = width;
	hasBorder = true;

	Shape* pSel = viewer.GetSelectedShape();
	if (pSel)
	{
		pSel->SetStyle(pSel->GetBorderColor(), pSel->GetFillColor(), true, pSel->IsFilled(), width);
		files.MarkDirty(hWnd);
		viewer.InvalidateCanvas(hWnd);
	}
	ApplyCurrentStyle(pcurrent);
}

void Editor::OnCreate(HWND hWnd)
{
	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

	toolbar.Create(hWnd, hInst);
	viewer.SetTopOffset(toolbar.GetHeight());
	viewer.UpdateScrollBars(hWnd);

	files.UpdateWindowTitle(hWnd);

	SelectTool(IDM_LINE);
}

void Editor::OnSize(HWND hWnd)
{
	EndTextEdit(hWnd, true);
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
	EndTextEdit(hWnd, true);
	rubberDrawn = false;
	viewer.OnHScroll(hWnd, wParam);
}

void Editor::OnVScroll(HWND hWnd, WPARAM wParam)
{
	EndTextEdit(hWnd, true);
	rubberDrawn = false;
	viewer.OnVScroll(hWnd, wParam);
}

void Editor::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	EndTextEdit(hWnd, true);
	rubberDrawn = false;
	viewer.OnMouseWheel(hWnd, wParam);
}

void Editor::OnLBDown(HWND hWnd, LPARAM lParam)
{
	if (hEdit)
	{
		EndTextEdit(hWnd, true);
		return;
	}

	int cx = (int)(short)LOWORD(lParam);
	int cy = (int)(short)HIWORD(lParam);
	if (cy < viewer.GetTopOffset()) return;

	long sx = 0, sy = 0;
	viewer.ScreenToScene(cx, cy, sx, sy, true);

	if (currentToolId == IDM_SELECT)
	{
		if (Shape* pSel = viewer.GetSelectedShape())
		{
			int h = pSel->HitHandle(sx, sy);
			if (h >= 0)
			{
				delete resizeOrig;
				resizeOrig = pSel->Clone();
				resizeHandle = h;
				isResizing = true;
				movedAny = false;
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
		int hit = viewer.SelectAt(sx, sy);
		if (hit >= 0)
		{
			isMoving = true;
			movedAny = false;
			lastMoveX = sx;
			lastMoveY = sy;
			SetCapture(hWnd);
		}
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (!pcurrent) return;

	isDrawing = true;
	rubberDrawn = false;
	xstart = sx;
	ystart = sy;

	pcurrent->Set(xstart, ystart, xstart, ystart);
	ApplyCurrentStyle(pcurrent);

	SetCapture(hWnd);
}

void Editor::OnLBDblClk(HWND hWnd)
{
	if (currentToolId != IDM_SELECT) return;
	Text* pText = dynamic_cast<Text*>(viewer.GetSelectedShape());
	if (!pText) return;

	long l, t, r, b;
	pText->GetBounds(l, t, r, b);
	BeginTextEdit(hWnd, pText, l, t, r, b);
}

void Editor::OnMouseMove(HWND hWnd, LPARAM lParam)
{
	if (isResizing)
	{
		long sx = 0, sy = 0;
		GetSceneMousePos(lParam, sx, sy);
		if (Shape* pSel = viewer.GetSelectedShape())
		{
			if (resizeOrig)
			{
				pSel->ResizeFrom(*resizeOrig, resizeHandle, sx, sy);
				movedAny = true;
				viewer.InvalidateCanvas(hWnd);
			}
		}
		return;
	}

	if (isMoving)
	{
		long sx = 0, sy = 0;
		GetSceneMousePos(lParam, sx, sy);
		long dx = sx - lastMoveX;
		long dy = sy - lastMoveY;

		if (viewer.MoveSelectedBy(dx, dy))
		{
			lastMoveX = sx;
			lastMoveY = sy;
			movedAny = true;
			viewer.InvalidateCanvas(hWnd);
		}
		return;
	}

	if (!isDrawing || !pcurrent) return;

	HDC hdc = GetDC(hWnd);
	viewer.BeginCanvasDraw(hdc, hWnd);
	SetROP2(hdc, R2_NOTXORPEN);
	SetBkMode(hdc, TRANSPARENT);

	pcurrent->SetRubber(true);

	if (rubberDrawn)
		pcurrent->Show(hdc);

	long x = 0, y = 0;
	GetSceneMousePos(lParam, x, y);

	pcurrent->Set(xstart, ystart, x, y);
	pcurrent->Show(hdc);

	rubberDrawn = true;
	pcurrent->SetRubber(false);

	ReleaseDC(hWnd, hdc);
}

void Editor::OnLBUp(HWND hWnd, LPARAM lParam)
{
	if (isResizing)
	{
		isResizing = false;
		ReleaseCapture();
		delete resizeOrig;
		resizeOrig = nullptr;
		if (movedAny)
		{
			files.MarkDirty(hWnd);
			movedAny = false;
		}
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (isMoving)
	{
		isMoving = false;
		ReleaseCapture();
		if (movedAny)
		{
			files.MarkDirty(hWnd);
			movedAny = false;
		}
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	if (!isDrawing || !pcurrent) return;

	isDrawing = false;
	rubberDrawn = false;
	ReleaseCapture();

	long x = 0, y = 0;
	GetSceneMousePos(lParam, x, y);

	pcurrent->Set(xstart, ystart, x, y);
	ApplyCurrentStyle(pcurrent);
	pcurrent->SetRubber(false);

	if (currentToolId == IDM_TEXT)
	{
		long l = (xstart < x) ? xstart : x, r = (xstart < x) ? x : xstart;
		long t = (ystart < y) ? ystart : y, b = (ystart < y) ? y : ystart;
		BeginTextEdit(hWnd, nullptr, l, t, r, b);
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	// клік без перетягування не створює фігуру (крім Node, якій потрібна лише точка)
	if (currentToolId != IDM_NODE && labs(x - xstart) < 3 && labs(y - ystart) < 3)
	{
		viewer.InvalidateCanvas(hWnd);
		return;
	}

	Shape* next = pcurrent->Clone();
	viewer.AddShape(pcurrent);
	files.MarkDirty(hWnd);
	pcurrent = next;

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

	const UINT toolIds[] = {
		IDM_SELECT, IDM_MOVE,
		IDM_NODE, IDM_EDGE, IDM_LINE, IDM_ARROW,
		IDM_TRNGL, IDM_RIGHT_TRNGL, IDM_RECT, IDM_ROMB,
		IDM_PENT, IDM_HEX, IDM_ELIPS, IDM_CUBE, IDM_CYL, IDM_TEXT
	};

	for (UINT id : toolIds)
	{
		CheckMenuItem(hMenu, id, MF_BYCOMMAND | ((id == currentToolId) ? MF_CHECKED : MF_UNCHECKED));
	}

	Shape* pSel = viewer.GetSelectedShape();
	bool curHasBorder = pSel ? pSel->HasBorder() : hasBorder;
	bool curHasFill = pSel ? pSel->IsFilled() : hasFill;
	int curWidth = pSel ? pSel->GetBorderWidth() : borderWidth;

	CheckMenuItem(hMenu, IDM_NO_BORDER, MF_BYCOMMAND | ((!curHasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_NO_COLOR, MF_BYCOMMAND | ((!curHasFill) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_1, MF_BYCOMMAND | ((curWidth == 1 && curHasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_3, MF_BYCOMMAND | ((curWidth == 3 && curHasBorder) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_WIDTH_5, MF_BYCOMMAND | ((curWidth == 5 && curHasBorder) ? MF_CHECKED : MF_UNCHECKED));

	EnableMenuItem(hMenu, IDM_UNDO, MF_BYCOMMAND | (viewer.CanUndo() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_REDO, MF_BYCOMMAND | (viewer.CanRedo() ? MF_ENABLED : MF_GRAYED));
}

bool Editor::OnCommand(HWND hWnd, WPARAM wParam)
{
	UINT wmId = LOWORD(wParam);
	if (wmId == IDC_INPLACE_EDIT)
	{
		if (HIWORD(wParam) == EN_KILLFOCUS) EndTextEdit(hWnd, true);
		return true;
	}
	if (wmId == IDM_TEXT_COMMIT) { EndTextEdit(hWnd, true);  return true; }
	if (wmId == IDM_TEXT_CANCEL) { EndTextEdit(hWnd, false); return true; }
	EndTextEdit(hWnd, true);   // будь-яка інша команда спершу завершує редагування

	if (toolbar.IsToolId(wmId))
	{
		SelectTool(wmId);
		viewer.InvalidateCanvas(hWnd);
		return true;
	}

	switch (wmId)
	{
	case IDM_NEW:
		files.New(hWnd, viewer);
		return true;

	case IDM_OPEN:
		files.Open(hWnd, viewer);
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
		if (viewer.Undo())
		{
			files.MarkDirty(hWnd);
			viewer.InvalidateCanvas(hWnd);
		}
		return true;

	case IDM_REDO:
		if (viewer.Redo())
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
	EndTextEdit(hWnd, true);
	return files.ConfirmDiscardChanges(hWnd, viewer);
}
