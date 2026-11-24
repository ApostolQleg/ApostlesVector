#pragma once

class Viewer;

class Files
{
private:
	std::wstring currentFilePath;
	bool isDirty = false;

	bool WriteToFile(HWND hWnd, const std::wstring& path, const Viewer& viewer) const;
	bool ReadFromFile(HWND hWnd, const std::wstring& path, Viewer& viewer) const;

	std::wstring ExtractFileName(const std::wstring& path) const;

public:
	void MarkDirty(HWND hWnd);
	bool IsDirty() const { return isDirty; }
	void UpdateWindowTitle(HWND hWnd) const;

	bool ConfirmDiscardChanges(HWND hWnd, Viewer& viewer);

	bool New(HWND hWnd, Viewer& viewer);
	bool Open(HWND hWnd, Viewer& viewer);
	bool Save(HWND hWnd, const Viewer& viewer);
	bool SaveAs(HWND hWnd, const Viewer& viewer);
};
