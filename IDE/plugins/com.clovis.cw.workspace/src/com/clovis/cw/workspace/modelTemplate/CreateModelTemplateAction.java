/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.viewers.StructuredSelection;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;

/**
 * Abstract Action class for creating model template.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class CreateModelTemplateAction extends IActionClassAdapter
		implements ModelTemplateConstants {

	protected IProject _project;

	protected StructuredSelection _selection;

	/**
	 * Visible for Components and Resources.
	 * 
	 * @param environment
	 *            Environment
	 * @return true for Components and Resources, false otherwise.
	 */
	public boolean isVisible(Environment environment) {
		_project = (IProject) environment.getValue("project");
		_selection = (StructuredSelection) environment.getValue("selection");

		if (_selection.size() == 1) {
			if (_selection.getFirstElement() instanceof BaseDiagramEditPart) {
				return false;
			} else if (_selection.getFirstElement() instanceof BaseEditPart) {
				return true;
			}
		}

		return false;
	}

	/**
	 * Enable for Components and Resources except Chassis and Cluster
	 * 
	 * @param environment
	 *            Environment
	 * @return true for Components and Resources except Chassis and Cluster,
	 *         false otherwise.
	 */
	public abstract boolean isEnabled(Environment environment);

	/**
	 * Sub classes should implement it to handle the action and pop up the
	 * create model template dialog.
	 * 
	 * @param args
	 *            0 - Shell
	 * @return true if action is successfull else false.
	 */
	public abstract boolean run(Object[] args);
}
