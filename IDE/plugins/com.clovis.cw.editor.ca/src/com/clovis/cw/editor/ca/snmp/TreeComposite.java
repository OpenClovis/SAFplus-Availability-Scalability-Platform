/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/TreeComposite.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.tree.TreeUI;
import com.clovis.cw.editor.ca.CaPlugin;

/**
 * @author ravik Composite which has a tree and buttons.
 */
public class TreeComposite extends Composite
{
    private Tree _tree = null;

    private TreeUI _treeViewer = null;

    private Text _desc;

     /**
     * Contructor
     *
     * @param parent
     *            Composite
     * @param style
     *            int
     */
    public TreeComposite(Composite parent, int style)
    {
        super(parent, style);
        buildTree();

    }

    /**
     * Creates Tree and puts buttons to the composite.
     */
    private void buildTree()
    {
        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 2;
        this.setLayout(gridLayout);
        GridData d1 = new GridData(GridData.FILL_BOTH);
        d1.horizontalSpan = 2;
        this.setLayoutData(d1);
        int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL
                | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;

        _tree = new Tree(this, style);
        ClassLoader loader = getClass().getClassLoader();
        _treeViewer = new TreeUI(_tree, loader);

        GridData gridData1 = new GridData();
        gridData1.horizontalSpan = 2;
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;
        gridData1.widthHint = 250;
        gridData1.heightHint = 200;

        _tree.setLayoutData(gridData1);

        Group dGroup = new Group(this, SWT.SHADOW_OUT);
        dGroup.setText("Details");
        GridData gd = new GridData();
        gd.horizontalSpan = 2;
        gd.horizontalAlignment = GridData.FILL;
        gd.verticalAlignment = GridData.FILL;
        gd.grabExcessVerticalSpace = true;
        dGroup.setLayoutData(gd);

        GridLayout groupLayout = new GridLayout();
        dGroup.setLayout(groupLayout);
        _desc = new Text(dGroup, SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL
                | SWT.BORDER);
        _desc.setText("Description :  " + "\n\nOID :  ");
        _desc.setEditable(false);
        GridData gdDoc = new GridData();
        gdDoc.horizontalSpan = 2;
        gdDoc.grabExcessHorizontalSpace = true;
        gdDoc.grabExcessVerticalSpace = true;
        gdDoc.verticalAlignment = GridData.FILL;
        gdDoc.horizontalAlignment = GridData.FILL;
        _desc.setLayoutData(gdDoc);
        ClovisNotifyingListImpl input = MibFilesReader.getInstance().
        getTreeInput();
        _treeViewer.setInput(input);

    }

    /**
     *
     * @return treeViewer instance.
     */

    public TreeUI getTree()
    {
        return _treeViewer;
    }
    /**
    *
    * @return Description Text instance.
    */
    public Text getDescriptionText()
    {
        return _desc;
    }
}
