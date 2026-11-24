#include "pch.h"
#include "framework.h"
#include "files.h"
#include "viewer.h"
#include "shape.h"

#define FILE_LINE_BUFFER 2048

std::wstring Files::ExtractFileName(const std::wstring& path) const
{
	if (path.empty()) return L"Untitled";

	size_t pos = path.find_last_of(L"\\/");
	if (pos == std::wstring::npos)
		return path;

	return path.substr(pos + 1);
}

void Files::UpdateWindowTitle(HWND hWnd) const
{
	if (!hWnd) return;

	std::wstring title = L"ApostlesVector - " + ExtractFileName(currentFilePath);
	if (isDirty)
		title += L"*";

	SetWindowTextW(hWnd, title.c_str());
}

void Files::MarkDirty(HWND hWnd)
{
	if (!isDirty)
	{
		isDirty = true;
		UpdateWindowTitle(hWnd);
	}
}

bool Files::ConfirmDiscardChanges(HWND hWnd, Viewer& viewer)
{
	if (!isDirty) return true;

	int res = MessageBoxW(
		hWnd,
		L"Save changes to the current scene?",
		L"ApostlesVector",
		MB_YESNOCANCEL | MB_ICONQUESTION
	);

	if (res == IDYES)
		return Save(hWnd, viewer);

	return (res == IDNO);
}

bool Files::WriteToFile(HWND hWnd, const std::wstring& path, const Viewer& viewer) const
{
	FILE* fout = nullptr;
	if (_wfopen_s(&fout, path.c_str(), L"wt, ccs=UTF-8") != 0 || !fout)
	{
		MessageBoxW(hWnd, L"Failed to save the scene file.", L"File Error", MB_OK | MB_ICONERROR);
		return false;
	}

	int count = viewer.GetCount();
	fwprintf(fout, L"APOSTLES_VECTOR 1.1\n");
	fwprintf(fout, L"COUNT %d\n", count);

	for (int i = 0; i < count; i++)
	{
		Shape* pShape = viewer.GetShape(i);
		if (pShape)
			pShape->Serialize(fout);
	}

	fclose(fout);
	return true;
}

bool Files::ReadFromFile(HWND hWnd, const std::wstring& path, Viewer& viewer) const
{
	FILE* fin = nullptr;
	if (_wfopen_s(&fin, path.c_str(), L"rt, ccs=UTF-8") != 0 || !fin)
	{
		MessageBoxW(hWnd, L"Failed to open the scene file.", L"File Error", MB_OK | MB_ICONERROR);
		return false;
	}

	viewer.Clear();

	WCHAR line[FILE_LINE_BUFFER] = {};
	while (fgetws(line, FILE_LINE_BUFFER, fin))
	{
		size_t len = wcslen(line);
		while (len > 0 && (line[len - 1] == L'\n' || line[len - 1] == L'\r'))
			line[--len] = L'\0';

		if (len == 0) continue;

		WCHAR typeName[64] = {};
		int offset = 0;
		if (swscanf_s(line, L"%63s%n", typeName, (unsigned)_countof(typeName), &offset) == 1)
		{
			if (wcscmp(typeName, L"APOSTLES_VECTOR") == 0 || wcscmp(typeName, L"COUNT") == 0)
				continue;

			Shape* pShape = Shape::CreateByName(typeName);
			if (pShape)
			{
				if (pShape->Deserialize(line + offset))
					viewer.AddShape(pShape, false);
				else
					delete pShape;
			}
		}
	}

	fclose(fin);
	return true;
}

bool Files::WriteSvgToFile(HWND hWnd, const std::wstring& path, const Viewer& viewer) const
{
	FILE* fout = nullptr;
	if (_wfopen_s(&fout, path.c_str(), L"wt, ccs=UTF-8") != 0 || !fout)
	{
		MessageBoxW(hWnd, L"Failed to export SVG file.", L"SVG Export Error", MB_OK | MB_ICONERROR);
		return false;
	}

	fwprintf(fout, L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fwprintf(fout, L"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
		SCENE_WIDTH, SCENE_HEIGHT, SCENE_WIDTH, SCENE_HEIGHT);

	int count = viewer.GetCount();
	for (int i = 0; i < count; i++)
	{
		Shape* pShape = viewer.GetShape(i);
		if (pShape)
			pShape->SerializeSvg(fout);
	}

	fwprintf(fout, L"</svg>\n");
	fclose(fout);
	return true;
}

bool Files::New(HWND hWnd, Viewer& viewer)
{
	if (!ConfirmDiscardChanges(hWnd, viewer))
		return false;

	viewer.Clear();
	currentFilePath.clear();
	isDirty = false;
	UpdateWindowTitle(hWnd);

	if (hWnd) InvalidateRect(hWnd, nullptr, FALSE);
	return true;
}

bool Files::Open(HWND hWnd, Viewer& viewer)
{
	if (!ConfirmDiscardChanges(hWnd, viewer))
		return false;

	WCHAR fileBuf[MAX_PATH] = {};

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"Apostles Vector Files (*.avec)\0*.avec\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = fileBuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = L"avec";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (!GetOpenFileNameW(&ofn))
		return false;

	if (!ReadFromFile(hWnd, fileBuf, viewer))
		return false;

	currentFilePath = fileBuf;
	isDirty = false;
	UpdateWindowTitle(hWnd);

	if (hWnd)
	{
		viewer.UpdateScrollBars(hWnd);
		InvalidateRect(hWnd, nullptr, FALSE);
	}
	return true;
}

bool Files::Save(HWND hWnd, const Viewer& viewer)
{
	if (currentFilePath.empty())
		return SaveAs(hWnd, viewer);

	if (!WriteToFile(hWnd, currentFilePath, viewer))
		return false;

	isDirty = false;
	UpdateWindowTitle(hWnd);
	return true;
}

bool Files::SaveAs(HWND hWnd, const Viewer& viewer)
{
	WCHAR fileBuf[MAX_PATH] = L"scene.avec";
	if (!currentFilePath.empty())
	{
		wcsncpy_s(fileBuf, currentFilePath.c_str(), _TRUNCATE);
	}

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"Apostles Vector Files (*.avec)\0*.avec\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = fileBuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = L"avec";
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	if (!GetSaveFileNameW(&ofn))
		return false;

	if (!WriteToFile(hWnd, fileBuf, viewer))
		return false;

	currentFilePath = fileBuf;
	isDirty = false;
	UpdateWindowTitle(hWnd);
	return true;
}

bool Files::ExportSvg(HWND hWnd, const Viewer& viewer) const
{
	WCHAR fileBuf[MAX_PATH] = L"scene.svg";

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"Scalable Vector Graphics (*.svg)\0*.svg\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = fileBuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = L"svg";
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	if (!GetSaveFileNameW(&ofn))
		return false;

	return WriteSvgToFile(hWnd, fileBuf, viewer);
}
