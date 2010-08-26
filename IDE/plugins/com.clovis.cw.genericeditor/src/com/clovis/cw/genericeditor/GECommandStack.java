/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GECommandStack.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.util.ArrayList;
import java.util.EventObject;
import java.util.List;
import java.util.Stack;

import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CommandStack;
import org.eclipse.gef.commands.CommandStackEvent;
import org.eclipse.gef.commands.CommandStackEventListener;
import org.eclipse.gef.commands.CommandStackListener;

/**
 * An implementation of a command stack.  A stack manages the executing, undoing, and
 * redoing of {@link Command Commands}.  Executed commands are pushed onto a a stack for
 * undoing later. Commands which are undone are pushed onto a redo stack. Whenever a new
 * command is executed, the Redo stack is flushed.
 * <P>
 * A CommandStack contains a dirty property. This property can be used to determine
 * when persisting changes is required. The stack is dirty whenever the last executed or
 * redone command is different than the command that was at the top of the undo stack when
 * {@link #markSaveLocation()} was last called. Initially, the undo stack is
 * empty, and not dirty.
 * 
 * @author Pushparaj
 */
public class GECommandStack extends CommandStack
{
	private int undoLimit = 0;
	private int saveLocation = 0;
	private Stack undoable = new Stack();
	private Stack redoable = new Stack();

	/**
	 * The list of {@link CommandStackListener}s.
	 * @deprecated This field should not be referenced, use {@link #notifyListeners()}
	 */
	protected List listeners = new ArrayList();
	private List eventListeners = new ArrayList();

	/**
	 * Constant indicating notification prior to executing a command (value is 1).
	 */
	public static final int PRE_EXECUTE = 1;
	/**
	 * Constant indicating notification prior to redoing a command (value is 2).
	 */
	public static final int PRE_REDO = 2;
	/**
	 * Constant indicating notification prior to undoing a command (value is 4).
	 */
	public static final int PRE_UNDO = 4;

	/**
	 * Constant indicating notification after a command has been executed (value is 8).
	 */
	public static final int POST_EXECUTE = 8;
	/**
	 * Constant indicating notification after a command has been redone (value is 16).
	 */
	public static final int POST_REDO = 16;
	/**
	 * Constant indicating notification after a command has been undone (value is 32).
	 */
	public static final int POST_UNDO = 32;

	/**
	 * Constructs a new command stack. By default, there is no undo limit, and isDirty() will
	 * return <code>false</code>.
	 */
	public GECommandStack() { }

	/**
	 * Appends the listener to the list of command stack listeners. Multiple adds will result
	 * in multiple notifications.
	 * @param listener the listener
	 */
	public void addCommandStackListener(CommandStackListener listener) {
		listeners.add(listener);
	}

	/**
	 * Appends the listener to the list of command stack listeners.  Multiple adds result in
	 * multiple notifications.
	 * @since 3.1
	 * @param listener the event listener
	 */
	public void addCommandStackEventListener(CommandStackEventListener listener) {
		eventListeners.add(listener);
	}

	/**
	 * @return <code>true</code> if it is appropriate to call {@link #redo()}.
	 */
	public boolean canRedo() {
		return !redoable.isEmpty();
	}

	/**
	 * @return <code>true</code> if {@link #undo()} can be called
	 */
	public boolean canUndo() {
		if (undoable.size() == 0)
			return false;
		return ((Command)undoable.lastElement()).canUndo();
	}

	/**
	 * Executes the specified Command if possible. Prior to executing the command, a
	 * CommandStackEvent for {@link #PRE_EXECUTE} will be fired to event listeners. 
	 * Similarly, after attempting to execute the command, an event for {@link #POST_EXECUTE}
	 * will be fired.  If the execution of the command completely normally,  stack listeners
	 * will receive {@link CommandStackListener#commandStackChanged(EventObject) stackChanged}
	 * notification.
	 * <P>
	 * If the command is <code>null</code> or cannot be executed, nothing happens.
	 * @param command the Command to execute
	 * @see CommandStackEventListener
	 */
	public void execute(Command command) {
		if (command == null || !command.canExecute())
			return;
		flushRedo();
		notifyListeners(command, PRE_EXECUTE);
		try {
			command.execute();
			if (getUndoLimit() > 0) {
				while (undoable.size() >= getUndoLimit()) {
					((Command)undoable.remove(0)).dispose();
					if (saveLocation > -1)
						saveLocation--;
				}
			}
			if (saveLocation > undoable.size())
				saveLocation = -1; //The save point was somewhere in the redo stack
			//undoable.push(command);
			if (command.canUndo()) {
				undoable.push(command);
			}
			notifyListeners();
		} finally {
			notifyListeners(command, POST_EXECUTE);
		}
	}

	/**
	 * @since 3.1
	 * @param command
	 * @param post_execute2
	 */
	protected void notifyListeners(Command command, int state) {
		CommandStackEvent event = new CommandStackEvent(this, command, state);
		for (int i = 0; i < eventListeners.size(); i++)
			((CommandStackEventListener)eventListeners.get(i)).stackChanged(event);
	}

	/**
	 * This will <code>dispose()</code> all the commands in both the undo and redo stack. Both
	 * stacks will be empty afterwards.
	 */
	public void dispose() {
		flushUndo();
		flushRedo();
	}

	/**
	 * Flushes the entire stack and resets the save location to zero. This method might be
	 * called when performing "revert to saved".
	 */
	public void flush() {
		flushRedo();
		flushUndo();
		saveLocation = 0;
		notifyListeners();
	}

	private void flushRedo() {
		while (!redoable.isEmpty())
			((Command)redoable.pop()).dispose();
	}

	private void flushUndo() {
		while (!undoable.isEmpty())
			((Command)undoable.pop()).dispose();
	}

	/**
	 * @return an array containing all commands in the order they were executed
	 */
	public Object[] getCommands() {
		List commands = new ArrayList(undoable);
		for (int i = redoable.size() - 1; i >= 0; i--) {
			commands.add(redoable.get(i));
		}
		return commands.toArray();
	}

	/**
	 * Peeks at the top of the <i>redo</i> stack. This is useful for describing to the User
	 * what will be redone. The returned <code>Command</code> has a label describing it.
	 * @return the top of the <i>redo</i> stack, which may be <code>null</code>
	 */
	public Command getRedoCommand() {
		return redoable.isEmpty() ? null : (Command)redoable.peek();
	}

	/**
	 * Peeks at the top of the <i>undo</i> stack. This is useful for describing to the User
	 * what will be undone. The returned <code>Command</code> has a label describing it.
	 * @return the top of the <i>undo</i> stack, which may be <code>null</code>
	 */
	public Command getUndoCommand() {
		return undoable.isEmpty() ? null : (Command)undoable.peek();
	}

	/**
	 * Returns the undo limit. The undo limit is the maximum number of atomic operations that
	 * the User can undo. <code>-1</code> is used to indicate no limit.
	 * @return the undo limit
	 */
	public int getUndoLimit() {
		return undoLimit;
	}

	/**
	 * Returns true if the stack is dirty. The stack is dirty whenever the last executed or
	 * redone command is different than the command that was at the top of the undo stack when
	 * {@link #markSaveLocation()} was last called. 
	 * @return <code>true</code> if the stack is dirty
	 */
	public boolean isDirty() {
		return undoable.size() != saveLocation;
	}

	/**
	 * Marks the last executed or redone Command as the point at which the changes were saved.
	 * Calculation of {@link #isDirty()} will be based on this checkpoint.
	 */
	public void markSaveLocation() {
		saveLocation = undoable.size();
		notifyListeners();
	}

	/**
	 * Sends notification to all {@link CommandStackListener}s.
	 * @deprecated
	 */
	protected void notifyListeners() {
		EventObject event = new EventObject(this);
		for (int i = 0; i < listeners.size(); i++)
			((CommandStackListener)listeners.get(i))
				.commandStackChanged(event);
	}

	/**
	 * Calls redo on the Command at the top of the <i>redo</i> stack, and pushes that Command
	 * onto the <i>undo</i> stack. This method should only be called when {@link #canUndo()}
	 * returns <code>true</code>.
	 */
	public void redo() {
		//Assert.isTrue(canRedo())
		if (!canRedo())
			return;
		Command command = (Command)redoable.pop();
		notifyListeners(command, PRE_REDO);
		try {
			command.redo();
			undoable.push(command);
			notifyListeners();
		} finally {
			notifyListeners(command, POST_REDO);
		}
	}

	/**
	 * Removes the first occurrence of the specified listener.
	 * @param listener the listener
	 */
	public void removeCommandStackListener(CommandStackListener listener) {
		listeners.remove(listener);
	}

	/**
	 * Removes the first occurrence of the specified listener.
	 * @param listener the listener
	 */
	public void removeCommandStackEventListener(CommandStackEventListener listener) {
		eventListeners.remove(listener);
	}

	/**
	 * Sets the undo limit. The undo limit is the maximum number of atomic operations that the
	 * User can undo. <code>-1</code> is used to indicate no limit.
	 * @param undoLimit the undo limit
	 */
	public void setUndoLimit(int undoLimit) {
		this.undoLimit = undoLimit;
	}

	/**
	 * Undoes the most recently executed (or redone) Command. The Command is popped from the
	 * undo stack to and pushed onto the redo stack. This method should only be called when
	 * {@link #canUndo()} returns <code>true</code>.
	 */
	public void undo() {
		//Assert.isTrue(canUndo());
		Command command = (Command)undoable.pop();
		notifyListeners(command, PRE_UNDO);
		try {
			command.undo();
			redoable.push(command);
			notifyListeners();
		} finally {
			notifyListeners(command, POST_UNDO);
		}
	}
}
