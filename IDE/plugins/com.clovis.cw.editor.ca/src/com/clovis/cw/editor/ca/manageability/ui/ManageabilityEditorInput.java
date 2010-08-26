package com.clovis.cw.editor.ca.manageability.ui;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IPersistableElement;

import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Editor Input for Manageability Editor
 * @author Pushparaj
 *
 */
public class ManageabilityEditorInput implements IEditorInput {

	private ProjectDataModel _pdm;
	private GenericEditorInput _caInput;
	private GenericEditorInput _compInput;
	private String _toolTipText;
	private ManageabilityEditor _editor;
	public ManageabilityEditorInput(GenericEditorInput caInput, GenericEditorInput compInput, ProjectDataModel pdm) {
		_pdm = pdm;
		_caInput = caInput;
		_compInput = compInput;
	}
	
	public ManageabilityEditorInput(ProjectDataModel pdm) {
		_pdm = pdm;
	
	}
	
	public ManageabilityEditor getEditor() {
		return _editor;
	}
	
	public void setEditor(ManageabilityEditor editor) {
		_editor = editor;
	}
	
	public boolean isDirty() {
		return (_editor != null && _editor.isDirty());
	}
	public GenericEditorInput getCAInput() {
		return _caInput;
	}
	public GenericEditorInput getCompInput() {
		return _compInput;
	}
	
	public void setCaInput(GenericEditorInput caInput) {
		_caInput = caInput;
	}
	
	public void setCompInput(GenericEditorInput compInput) {
		_compInput = compInput;
	}
	
	public boolean exists() {
		// TODO Auto-generated method stub
		return false;
	}

	public ImageDescriptor getImageDescriptor() {
		// TODO Auto-generated method stub
		return null;
	}

	public String getName() {
		// TODO Auto-generated method stub
		return "Manageability Editor";
	}

	public IPersistableElement getPersistable() {
		// TODO Auto-generated method stub
		return null;
	}

	/**
     * Return ToolTipText
     * @return ToolTipText
     */
    public String getToolTipText()
    {
        return _toolTipText;
    }
    /**
     * sets ToolTipText
     * @param toolTipText ToolTipText
     */
    public void setToolTipText(String toolTipText)
    {
        _toolTipText = toolTipText;
    }

	public Object getAdapter(Class adapter) {
		// TODO Auto-generated method stub
		return null;
	}
	/**
	 * Returns ProjectDataModel
	 * @return
	 */
	public ProjectDataModel getProjectDataModel() {
		return _pdm;
	}
}
