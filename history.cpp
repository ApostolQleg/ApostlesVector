#include "pch.h"
#include "history.h"

SceneCommand::SceneCommand(SceneSnapshot before, SceneSnapshot after)
	: beforeState(std::move(before))
	, afterState(std::move(after))
{
}

void SceneCommand::Execute(Viewer& viewer)
{
	viewer.RestoreSnapshot(afterState);
}

void SceneCommand::Undo(Viewer& viewer)
{
	viewer.RestoreSnapshot(beforeState);
}

void CommandHistory::PushExecuted(std::unique_ptr<ICommand> cmd)
{
	if (!cmd) return;
	undoStack.push_back(std::move(cmd));
	redoStack.clear();
}

bool CommandHistory::Undo(Viewer& viewer)
{
	if (undoStack.empty()) return false;
	auto cmd = std::move(undoStack.back());
	undoStack.pop_back();
	cmd->Undo(viewer);
	redoStack.push_back(std::move(cmd));
	return true;
}

bool CommandHistory::Redo(Viewer& viewer)
{
	if (redoStack.empty()) return false;
	auto cmd = std::move(redoStack.back());
	redoStack.pop_back();
	cmd->Execute(viewer);
	undoStack.push_back(std::move(cmd));
	return true;
}

void CommandHistory::Clear()
{
	undoStack.clear();
	redoStack.clear();
}