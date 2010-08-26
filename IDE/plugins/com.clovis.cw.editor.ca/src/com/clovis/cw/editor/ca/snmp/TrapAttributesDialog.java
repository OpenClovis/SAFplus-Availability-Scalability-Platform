/*******************************************************************************
 * ModuleName  : plugins
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/TrapAttributesDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.snmp;

import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.list.ListView;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
/**
 * @author ravik
 *
 * Dialog area to displays TreeComposite and ListViewer. Allowing user to select
 * EObject Tree Leaf nodes and place them into a List.
 */
public class TrapAttributesDialog extends TreeAttributesDialog
    implements Environment
{
    
    /**
     * TrapAttributesDialog Constructor.
     *
     * @param parentShell Shell
     * @param attrlist    AttributeList
     * @param attrclass   ECLass
     */
    public TrapAttributesDialog(Shell parentShell, EList attrlist,
            EClass attrclass)
    {
        super(parentShell, attrlist, attrclass);
    }
    /**
     * Callback method to create Dialog contents.
     *
     * @param parent
     *            Composite
     * @return Control
     */
    protected Control createDialogArea(Composite parent)
    {
    	getShell().setText("MIB Attribute Selection");
        setTitle("MIB Attribute Selection");
        GridData gdd = new GridData(GridData.FILL_BOTH);

        GridLayout layout = new GridLayout();
        layout.horizontalSpacing = 3;
        parent.setLayout(layout);

        CTabFolder tabRoot = new CTabFolder(parent, SWT.TOP | SWT.BORDER);
        tabRoot.setLayout(layout);
        tabRoot.setLayoutData(gdd);

        _loadedmibsTab = new LoadedMibsTab(tabRoot, SWT.NONE, this);
        _treeTab = new TrapAttributesTab(tabRoot, SWT.NONE);
        tabRoot.setSelection(0);
        return parent;
    }

    /**
     * After OK button press copy the imported Attributes to our EList
     */
    protected void copyToInternalObjects()
    {
    	ListView listViewer = _treeTab.getListViewer();
        List listinput = ((List) listViewer.getInput());
        for (int i = 0; i < listinput.size(); i++) {
            EObject eObject  = EcoreUtils.createEObject(_eClass, true);
            EObject objInput = (EObject) listinput.get(i);
            String name = (String) EcoreUtils.getValue(objInput, "Name");
            String oid = (String)  EcoreUtils.getValue(objInput, "OID");
            String description = (String) EcoreUtils.getValue(objInput, "Description");
            description = description.replaceAll("\n", " ");
            if (oid.charAt(0) == '.') {
                oid = oid.substring(1);
            }
            EcoreUtils.setValue(eObject, ClassEditorConstants.ALARM_ID, name);
            EcoreUtils.setValue(eObject, "OID", oid);
            EcoreUtils.setValue(eObject, "Description", description);

            _elist.add(eObject);
        }
    }
}
