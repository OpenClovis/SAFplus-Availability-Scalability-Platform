package com.clovis.cw.editor.ca.manageability.common;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TreeItem;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.cw.editor.ca.snmp.ClovisMibUtils;
import com.clovis.cw.editor.ca.snmp.MibImportManager;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.util.MibTreeNode;

/**
 * Composite for Loading and UnLoading Buttons
 * @author Pushparaj
 *
 */
public class LoadMibComposite extends Composite {

	protected ArrayList<String> _loadedMibs;
	protected ResourceTypeBrowserUI _treeViewer;
	protected ProjectDataModel _pdm;	
	public LoadMibComposite(Composite parent, int style,
			ArrayList<String> loadedMibs, ProjectDataModel pdm) {
		super(parent, style);
		_loadedMibs = loadedMibs;
		_pdm = pdm;
		setControls();
	}
	/**
	 * Creates Load and UnLoad Buttons
	 */
	protected void setControls() {
		GridLayout gridLayout = new GridLayout();
		gridLayout.numColumns = 2;
		setLayout(gridLayout);
		GridData d1 = new GridData(GridData.FILL_HORIZONTAL);
		setLayoutData(d1);
		Button loadButton = new Button(this, SWT.BORDER);
		loadButton.setText("Load Mib(s)...");
		loadButton.addSelectionListener(new LoadHandler());
		Button unLoadButton = new Button(this, SWT.BORDER);
		unLoadButton.setText("UnLoad Mib(s)");
		unLoadButton.addSelectionListener(new UnLoadHandler());
		addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				LoadedMibUtils.setLoadedMibs(_pdm, _loadedMibs);
			}
		});
	}
	
	/** 
	 * Set the Resource Type Browser
	 * @param treeViewer
	 */
	public void setResourceTypeBrowser(ResourceTypeBrowserUI treeViewer) {
		_treeViewer = treeViewer;
	}
	
	/**
	 * Handler class for Loading MIB
	 * @author Pushparaj
	 *
	 */
	class LoadHandler extends SelectionAdapter {
		public LoadHandler() {
			
		}

		public void widgetSelected(SelectionEvent e) {
			int style = SWT.MULTI;
			FileDialog fileselDialog = new FileDialog(new Shell(), style);
			fileselDialog.setFilterPath(UtilsPlugin
					.getDialogSettingsValue("LOADMIB"));
			fileselDialog.open();
			String fileNames[] = fileselDialog.getFileNames();
			UtilsPlugin.saveDialogSettings("LOADMIB", fileselDialog
					.getFilterPath());
			for (int i = 0; i < fileNames.length; i++) {
				String fileName = fileselDialog.getFilterPath()
						+ File.separator + fileNames[i];
				if (!_loadedMibs.contains(fileName)) {
					MibTreeNode node = LoadedMibUtils.loadMib(_pdm, fileName,
							_treeViewer);
					if (node != null) {
						_loadedMibs.add(fileName);
						importNotificationObjects(fileNames[i], node);
					}
				}
			}
		}
	}
	/**
	 * Handler class for Unloading Mib
	 * 
	 * @author Pushparaj
	 * 
	 */
	class UnLoadHandler extends SelectionAdapter {
		public void widgetSelected(SelectionEvent e) {
			unLoadMibs();
		}
	}
	/**
	 * Unloads the selected Mibs
	 */
	protected void unLoadMibs() {
		TreeItem root = _treeViewer.getTree().getItem(0);
		TreeItem items[] = root.getItems();
		for (int i = 0; i < items.length; i++) {
			if(items[i].getChecked()) {
				_treeViewer.getTree().setSelection(items[i]);
				ResourceTreeNode node = (ResourceTreeNode)items[i].getData();
				String mibFileName = node.getMibFileName();
				_loadedMibs.remove(mibFileName);
				_treeViewer.removeRootMibNode(node);
			}
		}
	}
	/**
	 * Import all the notifications 
	 * @param mibFileName
	 * @param node
	 */
	private void importNotificationObjects(String mibFileName, MibTreeNode node) {
		List<MibTreeNode> trapList = new ArrayList<MibTreeNode>();
		getMibNotificationObjects(_pdm.getProject(), node, trapList);
		MibImportManager manager = new MibImportManager(_pdm
				.getProject(), null);
		manager.convertMibObjToClovisObj(mibFileName, trapList);
	}
	
	/**
	 * Fetches node for valid notification from the mib
	 * 
	 * @param node -
	 *            MibTreeNode
	 * @param project -
	 *            IProject
	 * @param trapNodesList
	 */
    private void getMibNotificationObjects(IProject project, MibTreeNode node,
    		List<MibTreeNode> trapNodesList)
    {
    	if (node != null) {
    		if(node.getStatus() != null && (node.getStatus().equals("obsolete") || node.getStatus().equals("deprecated"))) {
    			// No need to parse depricated / obsolete;
    		} else if (node.isSnmpV2TrapNode() && !ClovisMibUtils.isDuplicateAlarm((String) node.getName(), project)) {
	    		trapNodesList.add(node);
	    	}
    	}
    	List children = node.getChildNodes();
		for (int i = 0; i < children.size(); i++) {
			MibTreeNode child = (MibTreeNode) children.get(i);
			getMibNotificationObjects(project, child, trapNodesList);
		}
    }
}
