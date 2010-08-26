/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEPaletteViewerProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import org.eclipse.gef.EditDomain;
import org.eclipse.gef.ui.palette.PaletteViewer;
import org.eclipse.gef.ui.palette.PaletteViewerProvider;
import org.eclipse.swt.widgets.Composite;

public class GEPaletteViewerProvider extends PaletteViewerProvider 
{
	private GEPaletteViewer pViewer = null;

	public GEPaletteViewerProvider(EditDomain graphicalViewerDomain,
			GEPaletteViewer viewer) {
		super(graphicalViewerDomain);
		pViewer = viewer;
	}

	/**
	 * Creates a PaletteViewer on the given Composite
	 * 
	 * @param parent
	 *            the control for the PaletteViewer
	 * @return the newly created PaletteViewer
	 */
	public PaletteViewer createPaletteViewer(Composite parent) {
		pViewer.createControl(parent);
		configurePaletteViewer(pViewer);
		hookPaletteViewer(pViewer);
		return pViewer;
	}
}
