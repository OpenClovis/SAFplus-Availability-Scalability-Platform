/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/TreeAttributesDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import java.util.Hashtable;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.EnvironmentNotifierImpl;
import com.clovis.common.utils.ui.list.ListView;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.protocol.snmp.SnmpOID;
import com.ireasoning.util.MibTreeNode;
/**
 * @author ravik
 *
 * Dialog area to displays TreeComposite and ListViewer. Allowing user to select
 * EObject Tree Leaf nodes and place them into a List.
 */
public class TreeAttributesDialog extends TitleAreaDialog
    implements Environment
{
    protected EList               _elist = null;
    protected EClass              _eClass = null;
    protected TreeAttributesTab   _treeTab = null;
    protected LoadedMibsTab       _loadedmibsTab = null;
    private EnvironmentNotifier _notifier = new EnvironmentNotifierImpl();
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    /**
     * TreeAttributesDialog Constructor.
     *
     * @param parentShell Shell
     * @param attrlist    AttributeList
     * @param attrclass   ECLass
     */
    public TreeAttributesDialog(Shell parentShell, EList attrlist,
            EClass attrclass)
    {
        super(parentShell);
        super.setShellStyle(SWT.CLOSE|SWT.MIN|SWT.MAX|SWT.RESIZE);
        _elist  = attrlist;
        _eClass = attrclass;
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
        _treeTab = new TreeAttributesTab(tabRoot, SWT.NONE);
        tabRoot.setSelection(0);
        EAnnotation ann = _eClass.getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("Help") != null ) 
			{	
				final String contextid = (String) ann.getDetails().get("Help");
				parent.addHelpListener(new HelpListener() {
					public void helpRequested(HelpEvent e) {
						PlatformUI.getWorkbench().getHelpSystem().displayHelp(
								contextid);
					}
				});       
			}
		}
        return parent;
    }

    /**
     * After OK button press copy the imported Attributes to our EList
     */
    protected void okPressed()
    {
    	copyToInternalObjects();
        super.okPressed();
    }

    /**
     * After OK button press copy the imported Attributes to our EList
     */
    protected void copyToInternalObjects()
    {
        // Set up MibUtil for processing
    	MibUtil.unloadAllMibs();
    	MibUtil.loadMib2();
    	MibUtil.setResolveSyntax(true);

    	// Get the list of paths for the mibs that were loaded
    	TableViewer tableViewer = _loadedmibsTab.getTableViewer();
    	TableItem[] tableItems = tableViewer.getTable().getItems();
    	String[] mibFilePaths = new String[tableItems.length];
    	for (int i = 0; i < tableItems.length; i++)
    	{
    		TableItem tableItem = tableItems[i];
    		EObject foo = (EObject)tableItem.getData();
    		EStructuralFeature bar = foo.eClass().getEStructuralFeature("MibPath");
    		mibFilePaths[i] = (String)foo.eGet(bar);
    	}
    	
    	// Parse the mibs that were loaded into a single mib tree
        MibTreeNode topNode = null;
    	try {
    		topNode = MibUtil.parseMibs(mibFilePaths);
    	} catch (Exception mpe) {
    		mpe.printStackTrace();
    	}

    	if (topNode != null)
    	{
	    	// Go through each of the mib attributes that have been selected for import
	    	ListView listViewer = _treeTab.getListViewer();
	        List listinput = ((List) listViewer.getInput());
	        for (int i = 0; i < listinput.size(); i++) {
	            EObject objInput = (EObject) listinput.get(i);
	            String oid = (String)  EcoreUtils.getValue(objInput, "OID");
	            if (oid.charAt(0) == '.') oid = oid.substring(1);
	
	            // Find the mib node associated with the attribute
	            MibTreeNode node = null;
    			node = topNode.searchByOID(new SnmpOID(oid));
	        	if (node != null)
	        	{
	                // Create the attribute and set its properties
	        		EObject eObject  = EcoreUtils.createEObject(_eClass, true);
	                String name = (String) node.getName();
	                String mibType = node.getSyntaxType();
	                byte mibCoreType = node.getRealSyntaxType();
	                String defVal = node.getDefVal();
	                EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_NAME, name);
	                EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_IS_IMPORTED, "true");
	                EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_OID, oid);
	
		        	if (mibType != null && !mibType.equals("")) {
		            	String size = node.getSyntax().getSize();
		        		Hashtable enumValues = node.getSyntax().getSyntaxMap();
		            	ClovisMibUtils.setTypeAndSize(mibType, mibCoreType, size, enumValues, defVal, eObject);
		            }
	
		            boolean isIndexed = node.isIndexNode();
		            String access = node.getAccess();
		            ClovisMibUtils.setDefaultPropertiesForMibAttributes(isIndexed, access, eObject);
		            
		            _elist.add(eObject);
	        	} else {
	        		LOG.error("Couldn't locate the attribute with OID [" + oid + "] in the loaded mibs.");
	        	}
	        }
    	} else {
    		LOG.error("There was an error parsing one of the following mibs selected for loading.");
        	for (int i = 0; i < mibFilePaths.length; i++)
        	{
        		LOG.error("   " + mibFilePaths[i]);
        	}
    	}
    }

    /**
     * @return the notifier.
     */
    public EnvironmentNotifier getNotifier()
    {
        return _notifier;
    }
    /**
     * Set Value for Env.
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    public void setValue(Object property, Object value)
    {
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
            return _treeTab.getTreeViewer().getSelection();
        } else if (property.toString().equalsIgnoreCase("tableviewer")) {
            return _loadedmibsTab.getTableViewer();
        } else if (property.toString().equalsIgnoreCase("tableSelection")) {
            return _loadedmibsTab.getTableViewer().getSelection();
        } else if (property.toString().equalsIgnoreCase("shell")) {
            return this.getShell();
        } else if (property.toString().equalsIgnoreCase("classloader")) {
            return this.getClass().getClassLoader();
        }
        return null;
    }
}
