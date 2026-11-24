#pragma once

class Toolbar
{
private:
	HWND hToolbar = nullptr;

public:
	void Create(HWND hWndParent, HINSTANCE hInst);
	void OnSize(HWND hWndParent);
	int GetHeight() const;

	LRESULT OnNotify(LPARAM lParam);
	void SetActiveTool(UINT cmdId);
	bool IsToolId(UINT cmdId) const;
};
