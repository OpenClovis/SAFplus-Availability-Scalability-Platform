/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/list/ListContentProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.list;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.IStructuredContentProvider;
/**
 * @author shubhada
 *
 *  ContentProvider for List
 */
public class ListContentProvider implements IStructuredContentProvider
{
    /**
     * Get elements.
     * Converts the list into array and return.
     * @param inputElement Object
     * @return Object []
     */
    public Object[] getElements(Object inputElement)
    {
        return ((NotifyingList) inputElement).toArray();
    }
    /**
     * Does nothing.
     */
    public void dispose() { }
    /**
     * does nothing
     * @param viewer   Viewer
     * @param old      Object
     * @param newInput Object
     */
    public void inputChanged(Viewer viewer, Object old, Object newInput) { }
}
