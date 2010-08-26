/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GenericEditorInput.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.io.File;

import org.eclipse.core.resources.IResource;
import org.eclipse.emf.common.util.EList;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IPersistableElement;

import com.clovis.common.utils.ecore.Model;
/**
 * @author nadeem
 * This class is the EditorInput for Generic Editor.
 */
public class GenericEditorInput implements IEditorInput
{
    protected Model       _model;
    protected File        _uiPropertyFile;
    protected EList       _metaObjects;
    protected int         _paletteOption;
    protected IResource   _resource;
    protected String      _toolTipText;
    private GenericEditor _editor;
        
    /**
     * constructor
     * @param resource IResource for this project or file
     * @param model Model which contains EObjects
     * @param uiFile ui property file
     * @param objs meta-objects list. this is optional
     * @param option option to populate palette
     */
    public GenericEditorInput(IResource resource, Model model,
            File uiFile, EList objs, int option)
    {
       _resource = resource;
       _uiPropertyFile = uiFile;
       _model = model;
       _metaObjects = objs;
       _paletteOption = option;
    }
    /**
     * Where EditorInput exists.
     * @return false
     */
    public boolean exists()
    {
        return false;
    }
    /**
     * Return image.
     * @return null
     */
    public ImageDescriptor getImageDescriptor()
    {
        return null;
    }
    /**
     * Return name of input.
     * @return null
     */
    public String getName()
    {
        return getClass().getName();
    }
    /**
     * Return null
     * @return null
     */
    public IPersistableElement getPersistable()
    {
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
    /**
     * @param adapter Class
     * @return null
     */
    public Object getAdapter(Class adapter)
    {
        return null;
    }
    /**
     * Sets the updated model
     * @param model
     */
    public void setModel(Model model)
    {
    	_model = model;
    }
    /**
     * Gets editor Model
     * @return Model
     */
    public Model getModel()
    {
        return _model;
    }
    /**
     * Gets Meta-Objects
     * @return list of EObjects
     */
    public EList getMetaObjects()
    {
        return _metaObjects;
    }
    /**
     * Gets Palette Option. This will decide whether the
     * palette should be populated with only meta-classes or
     * only with meta-objects or both.
     * @return palette option
     */
    public int getPaletteOption()
    {
        return _paletteOption;
    }
    /**
     * Get UI Property file.
     * @return Name of UI property file
     */
    public File  getUIPropertyFile()
    {
        return _uiPropertyFile;
    }
    /**
     * Gets resource
     * @return Resource
     */
    public IResource getResource()
    {
        return _resource;
    }
    /**
     * Sets the editor
     * @param editor Editor
     */
    public void setEditor(GenericEditor editor)
    {
    	_editor = editor;
    }
    /**
     * Returns Editor
     * @return Editor
     */
    public GenericEditor getEditor()
    {
    	return _editor;
    }
    /**
     * Returns Editor Model status
     * @return boolean
     */
    public boolean isDirty()
    {
    	return _editor != null ? _editor.getEditorModel().isDirty() : false;
    }
}
