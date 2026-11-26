#pragma once

class Text;
class Viewer;

constexpr UINT kInplaceEditCtrlId = 2001;
constexpr UINT kCmdTextCommit = 40001;
constexpr UINT kCmdTextCancel = 40002;

struct TextEditResult
{
	bool committed = false;
	Text* target = nullptr;
	RECT sceneRect = {};
	std::wstring text;
};

class InplaceTextEditor
{
private:
	HWND hEdit = nullptr;
	HFONT hFont = nullptr;
	Text* editTarget = nullptr;
	RECT editSceneRect = {};

public:
	InplaceTextEditor() = default;
	~InplaceTextEditor();

	InplaceTextEditor(const InplaceTextEditor&) = delete;
	InplaceTextEditor& operator=(const InplaceTextEditor&) = delete;

	bool IsActive() const { return hEdit != nullptr; }

	void Begin(HWND hParent, const Viewer& viewer, Text* target,
		long l, long t, long r, long b, int defaultBorderWidth);
	TextEditResult End(HWND hParent, bool commit);
};