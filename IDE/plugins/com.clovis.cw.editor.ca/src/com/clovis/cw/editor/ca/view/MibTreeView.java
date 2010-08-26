/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/view/MibTreeView.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.view;

import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

import com.clovis.cw.editor.ca.snmp.LoadedMibsTab;
import com.clovis.cw.editor.ca.snmp.TreeTab;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
/**
 * @author shubhada
 *
 * MibView displays loaded mibs in the form of a tree.
 */
public class MibTreeView extends ViewPart implements Environment
{
    private LoadedMibsTab _loadedmibsTab = null;

    private TreeTab _treeTab = null;
    /**
     * @param parent Composite
     */
    public void createPartControl(Composite parent)
    {
        GridData gdd = new GridData(GridData.FILL_BOTH);
        gdd.horizontalSpan = 1;
        gdd.grabExcessHorizontalSpace = true;
        gdd.grabExcessVerticalSpace = true;
        GridLayout layout = new GridLayout();
        layout.numColumns = 1;
        parent.setLayout(layout);

        CTabFolder tabRoot = new CTabFolder(parent, SWT.TOP | SWT.BORDER);

        tabRoot.setLayout(layout);
        tabRoot.setLayoutData(gdd);
        _loadedmibsTab = new LoadedMibsTab(tabRoot, SWT.NONE, this);
        _treeTab = new TreeTab(tabRoot, SWT.NONE);
        tabRoot.setSelection(0);
    }
    /**
     * setFocus method
     */
    public void setFocus()
    {
        _loadedmibsTab.getControl().setFocus();
    }
    /**
     * @return null
     */
    public EnvironmentNotifier getNotifier()
    {
        return null;
    }
    /**
     * Returns Parent Environment.
     * @return Parent Environment.
     */
    public Environment getParentEnv()
    {
        return null;
    }
    /**
     * @param property - key
     * @return the value for the key
     */
    public Object getValue(Object property)
    {
        if (property.toString().equalsIgnoreCase("treeviewer")) {
            return _treeTab.getTreeViewer();
        } else if (property.toString().equalsIgnoreCase("treeSelection")) {
            return (StructuredSelection) _treeTab.getTreeViewer()
                    .getSelection();
        } else if (property.toString().equalsIgnoreCase("tableviewer")) {
            return _loadedmibsTab.getTableViewer();
        } else if (property.toString().equalsIgnoreCase("tableSelection")) {
            return (StructuredSelection) _loadedmibsTab.getTableViewer()
                    .getSelection();
        } else if (property.toString().equalsIgnoreCase("shell")) {
            return this.getViewSite().getShell();
        } else if (property.toString().equalsIgnoreCase("classloader")) {
            return this.getClass().getClassLoader();
        }
        return null;
    }
    /**
     * 
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    public void setValue(Object property, Object value)
    {
             
    }
}
