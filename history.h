#pragma once

#include "shape.h"

class Viewer;

class ICommand
{
public:
	virtual ~ICommand() = default;
	virtual void Execute(Viewer& viewer) = 0;
	virtual void Undo(Viewer& viewer) = 0;
};

class AddShapeCommand : public ICommand
{
private:
	std::unique_ptr<Shape> shape;

public:
	explicit AddShapeCommand(std::unique_ptr<Shape> newShape);

	void Execute(Viewer& viewer) override;
	void Undo(Viewer& viewer) override;
};

class ModifyShapeCommand : public ICommand
{
private:
	int index = -1;
	std::unique_ptr<Shape> beforeState;
	std::unique_ptr<Shape> afterState;

public:
	ModifyShapeCommand(int shapeIndex, std::unique_ptr<Shape> before, std::unique_ptr<Shape> after);

	void Execute(Viewer& viewer) override;
	void Undo(Viewer& viewer) override;
};

class CommandHistory
{
private:
	std::vector<std::unique_ptr<ICommand>> undoStack;
	std::vector<std::unique_ptr<ICommand>> redoStack;

public:
	void Execute(std::unique_ptr<ICommand> cmd, Viewer& viewer);
	void PushExecuted(std::unique_ptr<ICommand> cmd);

	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }

	bool Undo(Viewer& viewer);
	bool Redo(Viewer& viewer);
	void Clear();
};
