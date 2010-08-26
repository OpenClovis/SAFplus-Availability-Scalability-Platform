/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/editparts/GraphicalPartFactory.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.editparts;

import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPartFactory;

import com.clovis.cw.genericeditor.GenericeditorPlugin;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * 
 * @author pushparaj
 * 
 * EditPartFactory for Generic Editor.
 */
public class GraphicalPartFactory implements EditPartFactory {

	private Map _editParts;

	private ClassLoader _classLoader;

	/**
	 * Create Method.
	 * 
	 * @param iContext
	 * @param iModel
	 */
	public EditPart createEditPart(EditPart iContext, Object iModel) {
		EditPart editPart = null;
		if (iModel instanceof EditorModel) {
			editPart = new BaseDiagramEditPart();
		} else if (iModel instanceof NodeModel) {
			String className = ((NodeModel) iModel).getName();
			try {
				editPart = (EditPart) _classLoader.loadClass(
						getEditPartName(className)).newInstance();
			} catch (ClassNotFoundException e) {
				GenericeditorPlugin.LOG.error(Messages.CLASSLOADER_UNABLE_LOAD
						+ className, e);
			} catch (InstantiationException e) {
				GenericeditorPlugin.LOG.error(Messages.CLASSLOADER_UNABLE_INSTANTIATE
						+ className, e);
			} catch (IllegalAccessException e) {
				GenericeditorPlugin.LOG.error(Messages.CLASSLOADER_UNABLE_ACCESS
						+ className, e);
			} catch (Exception e) {
				GenericeditorPlugin.LOG.error(e);
			}
		} else if (iModel instanceof EdgeModel) {
			String className = ((EdgeModel) iModel).getName();
			try {
				editPart = (EditPart) _classLoader.loadClass(
						getEditPartName(className)).newInstance();
			} catch (ClassNotFoundException e) {
				GenericeditorPlugin.LOG.error(Messages.CLASSLOADER_UNABLE_LOAD
						+ className, e);
			} catch (InstantiationException e) {
				GenericeditorPlugin.LOG.error(Messages.CLASSLOADER_UNABLE_INSTANTIATE
						+ className, e);
			} catch (IllegalAccessException e) {
				GenericeditorPlugin.LOG.error(Messages.CLASSLOADER_UNABLE_ACCESS
						+ className, e);
			} catch (Exception e) {
				GenericeditorPlugin.LOG.error(e);
			}
		} 
		if (editPart != null) {
			editPart.setModel(iModel);
		}
		return editPart;
	}

	/**
	 * This will return editpart class name for meta classes.
	 * 
	 * @param name
	 *            meta-class name
	 * @return editPart for meta-class
	 */
	private String getEditPartName(String name) {
		Set keys = _editParts.keySet();
		Iterator it = keys.iterator();
		while (it.hasNext()) {
			String key = (String) it.next();
			if (key.equals(name)) {
				return String.valueOf(_editParts.get(key));
			}
		}
		return null;
	}

	/**
	 * 
	 * @param map
	 *            list of editparts
	 */
	public void setEditParts(Map map) {
		_editParts = map;
	}

	/**
	 * set class loader. this wiil be used for loading editparts.
	 * 
	 * @param loader
	 */
	public void setClassLoader(ClassLoader loader) {
		_classLoader = loader;
	}
}
