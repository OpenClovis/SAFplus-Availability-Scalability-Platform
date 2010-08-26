/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/TableContentProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table;

import java.util.List;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.IStructuredContentProvider;
/**
 * ContentProvider for this viewer.
 * @author Nadeem
 */
public class TableContentProvider implements IStructuredContentProvider
{
    /**
     * Get All elements.
     * Converts the list into array and return
     * @param parent Parent Element.
     * @return Array for of the list.
     */
    public Object[] getElements (Object parent)
    {
        return ((List) parent).toArray();
    }
    /**
     * Dispose. Does nothing.
     */
    public void dispose() { }
    /**
     * Input changed callback.
     * Refreshes the view.
     * @param viewer   Viewer instance
     * @param old      Old Input
     * @param newInput New Input
     */
    public void inputChanged(Viewer viewer, Object old, Object newInput) { }
}
