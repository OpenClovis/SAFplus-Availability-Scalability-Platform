/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/tools/GECreationTool.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.tools;

import org.eclipse.gef.palette.ToolEntry;
import org.eclipse.gef.tools.CreationTool;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;

import com.clovis.cw.genericeditor.GEEditDomain;
import com.clovis.cw.genericeditor.GenericEditor;

/**
 * @author pushparaj
 *
 */
public class GECreationTool extends CreationTool{
	
	private String _keyStroke = "";
	private GenericEditor _editor;
	
	/**
	 *@see org.eclipse.gef.tools.AbstractTool#handleKeyDown(org.eclipse.swt.events.KeyEvent)
	 */
	protected boolean handleKeyDown(KeyEvent e) {
		if (e.character == SWT.ESC) {
			((GEEditDomain)getDomain()).updateESCAction();
			return true;
		} else if (e.keyCode == 32) {
			((GEEditDomain)getDomain()).updateESCAction();
			return true;
		} else if(!(String.valueOf(e.character).equals(_keyStroke))){
			String character = String.valueOf(e.character);
	 		ToolEntry entry = (ToolEntry) _editor.getToolEntries().get(character.toUpperCase());
			if(entry == null) {
				entry = (ToolEntry) _editor.getToolEntries().get(character.toLowerCase());
			}
			if(entry != null) {
				_editor.getViewer().getEditDomain().getPaletteViewer().setActiveTool(entry);
			}
		}
		return false;
	}
	/**
	 * Sets the key stroke 
	 * @param keyStroke String
	 */
	public void setKeyStroke(String keyStroke) {
		_keyStroke = keyStroke;
	}
	/**
	 * Sets Editor Instance
	 */
	public void setEditor(GenericEditor editor) {
		_editor = editor;
	}
}
