#include "pch.h"
#include "framework.h"
#include "editor.h"

#define TEXT_INPUT_BUFFER 1024

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
}

static INT_PTR CALLBACK TextDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static std::wstring* pResultText = nullptr;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		pResultText = reinterpret_cast<std::wstring*>(lParam);
		HWND hEdit = GetDlgItem(hDlg, IDC_TEXT_EDIT);
		if (pResultText && !pResultText->empty())
		{
			SetDlgItemTextW(hDlg, IDC_TEXT_EDIT, pResultText->c_str());
			SendMessageW(hEdit, EM_SETSEL, 0, -1);
		}
		SetFocus(hEdit);
		return (INT_PTR)FALSE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			if (pResultText)
			{
				WCHAR buf[TEXT_INPUT_BUFFER] = {};
				GetDlgItemTextW(hDlg, IDC_TEXT_EDIT, buf, _countof(buf));
				*pResultText = buf;
			}
			EndDialog(hDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool Editor::PromptTextInput(HWND hWnd, std::wstring& outText) const
{
	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	INT_PTR res = DialogBoxParamW(
		hInst,
		MAKEINTRESOURCEW(IDD_TEXT_DIALOG),
		hWnd,
		TextDlgProc,
		reinterpret_cast<LPARAM>(&outText)
	);
	return (res == IDOK && !outText.empty());
}

void Editor::GetSceneMousePos(HWND hWnd, long& sx, long& sy) const
{
	POINT pt = {};
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	viewer.ScreenToScene(pt.x, pt.y, sx, sy, true);
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
	currentToolId = cmdId;
	isDrawing = false;
	rubberDrawn = false;
	isMoving = false;
	movedAny = false;

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
	viewer.OnHScroll(hWnd, wParam);
}

void Editor::OnVScroll(HWND hWnd, WPARAM wParam)
{
	viewer.OnVScroll(hWnd, wParam);
}

void Editor::OnMouseWheel(HWND hWnd, WPARAM wParam)
{
	viewer.OnMouseWheel(hWnd, wParam);
}

void Editor::OnLBDown(HWND hWnd)
{
	POINT pt = {};
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	if (pt.y < viewer.GetTopOffset()) return;

	long sx = 0, sy = 0;
	viewer.ScreenToScene(pt.x, pt.y, sx, sy, true);

	if (currentToolId == IDM_SELECT)
	{
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

	Shape* pSel = viewer.GetSelectedShape();
	Text* pText = dynamic_cast<Text*>(pSel);
	if (!pText) return;

	std::wstring edited = pText->GetText();
	if (PromptTextInput(hWnd, edited))
	{
		pText->SetText(edited);
		files.MarkDirty(hWnd);
		viewer.InvalidateCanvas(hWnd);
	}
}

void Editor::OnMouseMove(HWND hWnd)
{
	if (isMoving)
	{
		long sx = 0, sy = 0;
		GetSceneMousePos(hWnd, sx, sy);
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
	GetSceneMousePos(hWnd, x, y);

	pcurrent->Set(xstart, ystart, x, y);
	pcurrent->Show(hdc);

	rubberDrawn = true;
	pcurrent->SetRubber(false);

	ReleaseDC(hWnd, hdc);
}

void Editor::OnLBUp(HWND hWnd)
{
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
	GetSceneMousePos(hWnd, x, y);

	pcurrent->Set(xstart, ystart, x, y);
	ApplyCurrentStyle(pcurrent);
	pcurrent->SetRubber(false);

	if (currentToolId == IDM_TEXT)
	{
		Text* pText = dynamic_cast<Text*>(pcurrent);
		std::wstring entered;
		if (pText && PromptTextInput(hWnd, entered))
		{
			pText->SetText(entered);
			Shape* next = pcurrent->Clone();
			viewer.AddShape(pcurrent);
			files.MarkDirty(hWnd);
			pcurrent = next;
		}
	}
	else
	{
		Shape* next = pcurrent->Clone();
		viewer.AddShape(pcurrent);
		files.MarkDirty(hWnd);
		pcurrent = next;
	}

	viewer.InvalidateCanvas(hWnd);
}

void Editor::OnPaint(HWND hWnd)
{
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
	return files.ConfirmDiscardChanges(hWnd, viewer);
}
