#pragma once

#include "viewer.h"

class ICommand
{
public:
	virtual ~ICommand() = default;
	virtual void Execute(Viewer& viewer) = 0;
	virtual void Undo(Viewer& viewer) = 0;
};

class SceneCommand : public ICommand
{
private:
	SceneSnapshot beforeState;
	SceneSnapshot afterState;

public:
	SceneCommand(SceneSnapshot before, SceneSnapshot after);

	void Execute(Viewer& viewer) override;
	void Undo(Viewer& viewer) override;
};

class CommandHistory
{
private:
	std::vector<std::unique_ptr<ICommand>> undoStack;
	std::vector<std::unique_ptr<ICommand>> redoStack;

public:
	void PushExecuted(std::unique_ptr<ICommand> cmd);

	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }

	bool Undo(Viewer& viewer);
	bool Redo(Viewer& viewer);
	void Clear();
};