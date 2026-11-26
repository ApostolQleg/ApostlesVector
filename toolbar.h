#pragma once

class Toolbar
{
private:
	HWND hToolbar = nullptr;
	HIMAGELIST hImageList = nullptr;

public:
	Toolbar() = default;
	~Toolbar();

	Toolbar(const Toolbar&) = delete;
	Toolbar& operator=(const Toolbar&) = delete;

	void Create(HWND hWndParent, HINSTANCE hInst);
	void OnSize(HWND hWndParent);
	int GetHeight() const;

	LRESULT OnNotify(LPARAM lParam) const;
	void SetActiveTool(UINT cmdId);
	void UpdateActionStates(int selectionCount);
};