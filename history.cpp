#include "pch.h"
#include "framework.h"
#include "history.h"
#include "viewer.h"

AddShapeCommand::AddShapeCommand(std::unique_ptr<Shape> newShape)
	: shape(std::move(newShape))
{
}

void AddShapeCommand::Execute(Viewer& viewer)
{
	if (!shape) return;
	shape->SetSelected(false);
	viewer.AddShape(std::move(shape));
}

void AddShapeCommand::Undo(Viewer& viewer)
{
	shape = viewer.RemoveLastShape();
	if (shape)
		shape->SetSelected(false);
}

ModifyShapesCommand::ModifyShapesCommand(std::vector<ShapeSnapshot> before,
	std::vector<ShapeSnapshot> after)
	: beforeStates(std::move(before))
	, afterStates(std::move(after))
{
}

void ModifyShapesCommand::Execute(Viewer& viewer)
{
	viewer.ClearSelection();
	for (const auto& item : afterStates)
	{
		if (item.state)
			viewer.ReplaceShapeAt(item.index, item.state->Clone());
	}
}

void ModifyShapesCommand::Undo(Viewer& viewer)
{
	viewer.ClearSelection();
	for (const auto& item : beforeStates)
	{
		if (item.state)
			viewer.ReplaceShapeAt(item.index, item.state->Clone());
	}
}

void CommandHistory::Execute(std::unique_ptr<ICommand> cmd, Viewer& viewer)
{
	if (!cmd) return;
	cmd->Execute(viewer);
	undoStack.push_back(std::move(cmd));
	redoStack.clear();
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
