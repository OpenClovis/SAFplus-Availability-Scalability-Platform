/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.gef.commands.Command;

import com.clovis.cw.genericeditor.model.EditorModel;

/**
 * Command class for the use of model template inside the editor with support
 * for undo and redo.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateUseCommand extends Command {

	private EditorModel _editorModel;

	private List<EObject> _objList = new ArrayList<EObject>();

	/**
	 * Constructor.
	 * 
	 * @param editorModel
	 * @param topEditorObj
	 */
	public ModelTemplateUseCommand(EditorModel editorModel, EObject topEditorObj) {

		_editorModel = editorModel;
		createObjectList(topEditorObj);
	}

	/**
	 * Creates the object list from the top level object.
	 * 
	 * @param topEditorObj
	 */
	private void createObjectList(EObject topEditorObj) {

		List refList = topEditorObj.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			EReference ref = (EReference) refList.get(i);

			Object val = topEditorObj.eGet(ref);
			if (val != null) {

				if (val instanceof EObject) {
					_objList.add((EObject) val);

				} else if (val instanceof List) {

					List valList = (List) val;
					for (int j = 0; j < valList.size(); j++) {
						_objList.add((EObject) valList.get(j));
					}
				}
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.gef.commands.Command#execute()
	 */
	@Override
	public void execute() {
		addTemplateObjectsToEditor();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.gef.commands.Command#redo()
	 */
	@Override
	public void redo() {
		addTemplateObjectsToEditor();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.gef.commands.Command#undo()
	 */
	@Override
	public void undo() {
		removeTemplateObjectsFromEditor();
	}

	/**
	 * Removes template objects to the editor.
	 */
	private void removeTemplateObjectsFromEditor() {
		for (int i = 0; i < _objList.size(); i++) {
			_editorModel.removeEObject(_objList.get(i));
		}
	}

	/**
	 * Adds template objects to the editor.
	 */
	private void addTemplateObjectsToEditor() {
		for (int i = 0; i < _objList.size(); i++) {
			_editorModel.addEObject(_objList.get(i));
		}
	}
}
