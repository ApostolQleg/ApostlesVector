#pragma once

#include "Resource.h"

class Toolbar
{
private:
	HWND hWndToolbar = nullptr;

	COLORREF borderColor = RGB(0, 0, 0);
	COLORREF fillColor = RGB(255, 255, 255);
	bool hasFill = false;

	void DrawColorIndicator(LPNMTBCUSTOMDRAW lpcd) const;

public:
	void Create(HWND hWndParent, HINSTANCE hInst);
	void OnSize(HWND hWndParent);
	int GetHeight() const;

	LRESULT OnNotify(LPARAM lParam);
	void SetActiveTool(UINT cmdId);
	void UpdateColors(COLORREF border, COLORREF fill, bool filled);

	bool IsToolId(UINT cmdId) const;
};
