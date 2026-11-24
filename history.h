#pragma once

#include "shape.h"

class Viewer;

struct ShapeSnapshot
{
	int index = -1;
	std::unique_ptr<Shape> state;
};

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

class ModifyShapesCommand : public ICommand
{
private:
	std::vector<ShapeSnapshot> beforeStates;
	std::vector<ShapeSnapshot> afterStates;

public:
	ModifyShapesCommand(std::vector<ShapeSnapshot> before, std::vector<ShapeSnapshot> after);

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
