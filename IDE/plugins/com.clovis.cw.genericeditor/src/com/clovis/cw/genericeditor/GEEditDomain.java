/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEEditDomain.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import org.eclipse.gef.DefaultEditDomain;
import org.eclipse.gef.EditPartViewer;
import org.eclipse.gef.Tool;
import org.eclipse.gef.palette.PaletteListener;
import org.eclipse.gef.palette.PaletteRoot;
import org.eclipse.gef.palette.ToolEntry;
import org.eclipse.gef.tools.CreationTool;
import org.eclipse.gef.tools.SelectionTool;
import org.eclipse.gef.ui.palette.PaletteViewer;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.ui.IEditorPart;

public class GEEditDomain extends DefaultEditDomain{
	
	private Tool _activeTool;
	private Tool _defaultTool;
	private PaletteViewer _paletteViewer;
	private PaletteRoot _paletteRoot;
	private GenericEditor _editor;
	/**
	 * Listens to the PaletteViewer for changes in selection, and sets the Domain's Tool
	 * accordingly.
	 */
	private PaletteListener _paletteListener = new PaletteListener() {
		public void activeToolChanged(PaletteViewer viewer, ToolEntry tool) {
			handlePaletteToolChanged();
		}
	};
	public GEEditDomain(GenericEditor editor) {
		super(editor);
		_editor = editor;
	}
	private void handlePaletteToolChanged() {
		ToolEntry entry = getPaletteViewer().getActiveTool();
		if (entry != null)
			setActiveTool(entry.createTool());
		else
			setActiveTool(getDefaultTool());
	}
	/**
	 * Returns the active Tool
	 * @return the active Tool
	 */
	public Tool getActiveTool() {
		return _activeTool;
	}
	/**
	 * Sets the active Tool for this EditDomain. If a current Tool is active, it is
	 * deactivated. The new Tool is told its EditDomain, and is activated.
	 * @param tool the Tool
	 */
	public void setActiveTool(Tool tool) {
		if (_activeTool != null)
			_activeTool.deactivate();
		_activeTool = tool;
		if (_activeTool != null) {
			_activeTool.setEditDomain(this);
			_activeTool.activate();
		}
	}
	/**
	 * Returns the default tool for this edit domain. This will be a {@link
	 * org.eclipse.gef.tools.SelectionTool} unless specifically replaced using {@link
	 * #setDefaultTool(Tool)}.
	 *
	 * @return The default Tool for this domain
	 */
	public Tool getDefaultTool() {
		if (_defaultTool == null)
			_defaultTool = new SelectionTool();
		return _defaultTool;
	}
	/**
	 * Sets the default Tool, which is used if the Palette does not provide a default
	 * @param tool <code>null</code> or a Tool
	 */
	public void setDefaultTool(Tool tool) {
		_defaultTool = tool;
	}
	/**
	 * Returns the palette viewer currently associated with this domain.
	 * @since 1.0
	 * @return The current palette viewer
	 */
	public PaletteViewer getPaletteViewer() {
		return _paletteViewer;
	}
	/**
	 * Loads the default Tool. If a palette has been provided and that palette has a default,
	 * then that tool is loaded. If not, the EditDomain's default tool is loaded. By default,
	 * this is the {@link org.eclipse.gef.tools.SelectionTool}.
	 */
	public void loadDefaultTool() {
		//setActiveTool(null);
		if (_paletteRoot != null) {
			if (_paletteRoot.getDefaultEntry() != null) {
				getPaletteViewer().setActiveTool(_paletteRoot.getDefaultEntry());
				return;
			} /*else
				getPaletteViewer().setActiveTool(null);*/
		}
		if(_activeTool instanceof CreationTool) {
			setActiveTool(_activeTool);
		} else {
			setActiveTool(getDefaultTool());
		}
	}
	/**
	 * update paltte after pressing ESC key
	 */
	public void updateESCAction() {
		if (_paletteRoot != null) {
			if (_paletteRoot.getDefaultEntry() != null) {
				getPaletteViewer().setActiveTool(_paletteRoot.getDefaultEntry());
				return;
			} else
				getPaletteViewer().setActiveTool(null);
		} 
		setActiveTool(getDefaultTool());
	}
	/**
	 * Sets the PalatteRoot for this EditDomain. If the EditDomain already knows about a
	 * PaletteViewer, this root will be set into the palette viewer also. Loads the default
	 * Tool after the root has been set.
	 * <p>
	 * It is recommended that the palette root not be set multiple times.  Some components
	 * (such as the PaletteCustomizerDialog for the PaletteViewer) might still hold on to the
	 * old root.  If the input has changed or needs to be refreshed, just remove all the 
	 * children from the root and add the new ones.
	 * 
	 * @param root the palette's root
	 */
	public void setPaletteRoot(PaletteRoot root) {
		if (_paletteRoot == root)
			return;
		_paletteRoot = root;
		if (getPaletteViewer() != null) {
			getPaletteViewer().setPaletteRoot(_paletteRoot);
			loadDefaultTool();
		}
	}
	/**
	 * Sets the <code>PaletteViewer</code> for this EditDomain
	 * @param palette the PaletteViewer
	 */
	public void setPaletteViewer(PaletteViewer palette) {
		if (palette == _paletteViewer)
			return;
		if (_paletteViewer != null)
			_paletteViewer.removePaletteListener(_paletteListener);
		_paletteViewer = palette;
		if (_paletteViewer != null) {
			palette.addPaletteListener(_paletteListener);
			if (_paletteRoot != null) {
				_paletteViewer.setPaletteRoot(_paletteRoot);
				loadDefaultTool();
			}
		}
	}
	/**
	 * @see org.eclipse.gef.EditDomain#mouseDown(org.eclipse.swt.events.MouseEvent, org.eclipse.gef.EditPartViewer)
	 */
	public void mouseDown(MouseEvent me, EditPartViewer viewer) {
		_editor.setCursorLocation(new Point(me.x, me.y));
		super.mouseDown(me, viewer);
	}
}
