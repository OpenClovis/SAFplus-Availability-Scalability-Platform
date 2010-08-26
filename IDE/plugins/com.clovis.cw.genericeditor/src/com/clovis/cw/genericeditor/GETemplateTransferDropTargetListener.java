/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.dnd.TemplateTransferDropTargetListener;
import org.eclipse.gef.requests.CreationFactory;

/**
 * @author pushparaj
 *
 * Listener to handle drop and drop nodes from
 * palette
 */
public class GETemplateTransferDropTargetListener
extends TemplateTransferDropTargetListener
{
    private GenericEditor _editor;

    /**
     * Constructs a listener on the specified viewer.
     * @param editor Editor
     */
    public GETemplateTransferDropTargetListener(GenericEditor editor)
    {
        super(editor.getViewer());
        _editor = editor;
    }
    /**
     * @see org.eclipse.gef.dnd.TemplateTransferDropTargetListener#getFactory(
     * java.lang.Object)
     */
    protected CreationFactory getFactory(Object template)
    {
        return _editor.getCreationFactory((EObject) template);
    }
}
