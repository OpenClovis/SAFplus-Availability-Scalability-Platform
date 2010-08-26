/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEKeyHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import org.eclipse.gef.GraphicalViewer;
import org.eclipse.gef.palette.ToolEntry;
import org.eclipse.gef.ui.palette.PaletteViewer;
import org.eclipse.gef.ui.parts.GraphicalViewerKeyHandler;
import org.eclipse.swt.events.KeyEvent;

public class GEKeyHandler extends GraphicalViewerKeyHandler 
{
	PaletteViewer _paletteViewer = null;
	GenericEditor _editor 		 = null;
	
	public GEKeyHandler(GraphicalViewer viewer, GenericEditor editor) {
		super(viewer);
		_editor = editor;
		_paletteViewer  = _editor.getViewer().getEditDomain().getPaletteViewer();
	}
	
	public boolean keyPressed(KeyEvent event) {
 		String character = String.valueOf(event.character);
 		ToolEntry entry = (ToolEntry) _editor.getToolEntries().get(character.toUpperCase());
		if(entry == null) {
			entry = (ToolEntry) _editor.getToolEntries().get(character.toLowerCase());
		}
		if(entry != null) {
			_paletteViewer.setActiveTool(entry);
		}
		return super.keyPressed(event);
	}
}
