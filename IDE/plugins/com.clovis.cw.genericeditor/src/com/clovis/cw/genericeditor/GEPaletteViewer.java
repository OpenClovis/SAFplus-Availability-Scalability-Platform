/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEPaletteViewer.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import org.eclipse.gef.palette.ToolEntry;
import org.eclipse.gef.ui.palette.PaletteViewer;

public class GEPaletteViewer extends PaletteViewer
{
	private GenericEditor _editor = null;
	public GEPaletteViewer(GenericEditor editor)
	{
		super();
		_editor = editor;
	}
	/**
	 * Sets the active entry for this palette.  The Editpart for the given entry will be
	 * activated (selected).
	 * 
	 * @param	newMode		the ToolEntry whose EditPart has to be set as the active tool 
	 * 						in this palette
	 */
	public void setActiveTool(ToolEntry newMode) {
		super.setActiveTool(newMode);
		_editor.setFocus();
	}
	
}
