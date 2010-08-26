/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/TreeTab.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.tree.TreeNode;
import com.clovis.common.utils.ui.tree.TreeUI;

/**
 * @author shubhada
 *
 * Tab which has Tree to show contents of loaded mibs
 */
public class TreeTab extends CTabItem
{
    private TreeComposite _treeComp = null;
    /**
     * @param parent CTabfolder
     * @param style  int
     */
    public TreeTab(CTabFolder parent, int style)
    {
        super(parent, style);
        setControl(createContents(parent));
        setText("MIB Tree");

    }
    /**
     *
     * @param arg0 parent CTabFolder
     * @return Composite
     */
    protected Composite createContents(CTabFolder arg0)
    {
        Composite baseComposite = new Composite(arg0, SWT.NONE);
        baseComposite.setLayoutData(new GridData(GridData.FILL_BOTH));

        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 1;
        baseComposite.setLayout(gridLayout);

        _treeComp = new TreeComposite(baseComposite, SWT.NONE);

        GridData gridData1 = new GridData(GridData.FILL_BOTH);

        //gridData1.horizontalAlignment = GridData.BEGINNING;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.verticalAlignment = GridData.FILL;

        _treeComp.setLayoutData(gridData1);
        _treeComp.getTree().addSelectionChangedListener(new
                SelectionChangedListener());

        Composite buttonComposite = new Composite(baseComposite, SWT.NONE);
        buttonComposite.setLayoutData(new GridData());
        buttonComposite.setLayout(new GridLayout());


        return baseComposite;
    }
    /**
     *
     * @return TreeViewer
     */

    public TreeUI getTreeViewer()
    {
        return _treeComp.getTree();
    }

    /**
     * @author shubhada
     *
     * Selection Changed Listener on Tree
     */
    class SelectionChangedListener implements
            ISelectionChangedListener
     {

        /**
         * Selection changed implementation
         *
         * @param event
         *            SelectionChangedEvent
         */

        public void selectionChanged(SelectionChangedEvent event)
        {

            if (event.getSource() instanceof TreeUI) {
                StructuredSelection sel = (StructuredSelection) _treeComp.
                getTree().getSelection();
                TreeNode node = (TreeNode) sel.getFirstElement();
                if (node != null && node.getValue() instanceof EObject) {
                    EObject eobj = (EObject) node.getValue();
                    _treeComp.getDescriptionText().setEditable(true);
                    _treeComp.getDescriptionText().setText("Description :  "
                            + (String) EcoreUtils.getValue(eobj, "Description")
                            + "\n\nOID :  "
                            + (String) EcoreUtils.getValue(eobj, "OID"));
                    _treeComp.getDescriptionText().setEditable(false);
                }
                    
            }
        }
     }
        
}
