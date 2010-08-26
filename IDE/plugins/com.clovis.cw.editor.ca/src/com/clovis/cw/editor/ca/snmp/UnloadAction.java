/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/UnloadAction.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.menu.IActionClassAdapter;

/**
 * @author ravik
 *
 * Action Class to Unload the mib from the tree.
 */
public class UnloadAction extends IActionClassAdapter
{

    /**
     * makes the button disabled if the selection is empty
     * @param Environment environment
     */
   /* public boolean isEnabled(Environment environment)
    {
        IStructuredSelection sel =
            (IStructuredSelection) environment.getValue("selection");
        return !sel.isEmpty();
    }*/
    /**
     * @param args
     *            0 - TableViewer 1 - TreeViewer
     * @return true if action runs successfully else false.
     */
    public boolean run(Object[] args)
    {
        TableViewer tableViewer = (TableViewer) args[0];
        TreeViewer treeViewer = (TreeViewer) args[1];
        StructuredSelection sel = (StructuredSelection) tableViewer
                .getSelection();
        ClovisNotifyingListImpl mibnames = MibFilesReader.getInstance().
        getFileNames();
        if (!(sel.isEmpty())) {
            for (int i = 0; i < sel.toList().size(); i++) {
                    String filename = ClovisMibUtils
                           .getFileName((EObject) sel.toList().get(i));

                    mibnames.remove(filename);
                    tableViewer.refresh();
                }
            }
        ClovisNotifyingListImpl input = MibFilesReader.getInstance().
        getTreeInput();
        if (treeViewer != null) {
        	treeViewer.setInput(input);
        }
        Group mibObjectsgroup = (Group) args[2];
        mibObjectsgroup.setText("");
        Control objects[] = mibObjectsgroup.getChildren();
        for (int i = 0; i< objects.length; i++) {
        	Control obj = objects[i];
        	obj.dispose();
        }
        mibObjectsgroup.redraw();
        mibObjectsgroup.getParent().redraw();
        return true;
    }
}
