/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/action/MoveUpAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table.action;

import org.eclipse.emf.common.util.EList;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.common.utils.ui.table.TableUI;

/**
 * @author shubhada
 *
 * Action class for moving a row up in the table.
 */
public class MoveUpAction extends IActionClassAdapter
{
    /**
     * makes the button disabled if the selection is empty
     * or if the selection is larger than 1.
     * @param Environment environment
     */
    public boolean isEnabled(Environment environment)
    {
        StructuredSelection sel =
            (StructuredSelection) environment.getValue("selection");
        TableViewer tableViewer =
            (TableViewer) environment.getValue("tableviewer");
        return ((!sel.isEmpty()) && (sel.size() == 1) &&
            tableViewer.getTable().getSelectionIndex() != 0);
    }

    /**
     * Swaps the selected item wth the above row.
     */
    public boolean run(Object[] args)
    {
        TableViewer tableViewer = (TableViewer) args[0];
        int selIndex = tableViewer.getTable().getSelectionIndex();
        if (selIndex != 0) {
            EList elist = (EList) tableViewer.getInput();
            elist.move(selIndex - 1, selIndex);
            tableViewer.getTable().setSelection(selIndex - 1);
            ((TableUI)tableViewer).selectionChanged(null);
        }
        return true;
    }
}
