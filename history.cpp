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

ModifyShapeCommand::ModifyShapeCommand(int shapeIndex,
	std::unique_ptr<Shape> before, std::unique_ptr<Shape> after)
	: index(shapeIndex)
	, beforeState(std::move(before))
	, afterState(std::move(after))
{
}

void ModifyShapeCommand::Execute(Viewer& viewer)
{
	if (afterState)
		viewer.ReplaceShapeAt(index, afterState->Clone());
}

void ModifyShapeCommand::Undo(Viewer& viewer)
{
	if (beforeState)
		viewer.ReplaceShapeAt(index, beforeState->Clone());
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
