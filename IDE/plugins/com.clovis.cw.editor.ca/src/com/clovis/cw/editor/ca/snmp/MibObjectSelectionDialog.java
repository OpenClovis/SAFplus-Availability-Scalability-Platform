/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/MibObjectSelectionDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.snmp;

import java.io.File;
import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeNode;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.IEnvironmentListener;
import com.clovis.common.utils.ui.ClovisMessageHandler;
import com.clovis.common.utils.ui.table.TableUI;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.manageability.common.LoadedMibUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.util.MibParseException;
import com.ireasoning.util.MibTreeNode;

/**
 * 
 * @author shubhada
 * 
 * Dialog to capture selection of MIB objects to be imported 
 * 
 */
public class MibObjectSelectionDialog extends TitleAreaDialog
{
	private IProject _project = null;
    private Table _table = null;
    private TableUI _tableViewer = null;
    private Composite _mibObjComposite = null;
    private ScrolledComposite _scrolledComposite = null;
    private String _selMibName = "";
    private List _mibNodesList = new Vector();
    private static final String DIALOG_TITLE = "Select MIB Objects";
    
    // need trailing spaces to that switching button text will not clip label
    private static final String _selectAllText = "<Select All>    ";
    private static final String _deselectAllText = "<Deselect All>";
    private boolean _allSelected = false;
    
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    /**
     * constructor
     * 
     * @param parentShell - Parent Shell
     * @param project - IProject
     * 
     */
    public MibObjectSelectionDialog(Shell parentShell, IProject project)
    {
    	super(parentShell);
    	super.setShellStyle(SWT.CLOSE|SWT.MIN|SWT.MAX|SWT.RESIZE|SWT.SYSTEM_MODAL);
    	_project = project;
        
        
    }
    /**
     * @param parent - Parent Composite
     * Creates the controls in the Dialog Area
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 2;
        container.setLayout(glayout);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));

        
        int style = SWT.BORDER | SWT.H_SCROLL
        | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.MULTI
        | SWT.READ_ONLY;
        _table = new Table (container, style);
        ClassLoader loader = getClass().getClassLoader();
        _tableViewer = new TableUI(_table,
                ClovisMibUtils.getUiECLass(), loader, true);

        GridData gridData1 = new GridData();
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;
        gridData1.heightHint = container.getDisplay().getClientArea().height / 10;

        _table.setLayoutData(gridData1);
        _table.setLinesVisible(true);
        _table.setHeaderVisible(true);
        
        final ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        final List<String> loadedMibs = pdm.getLoadedMibs();
        container.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				LoadedMibUtils.setLoadedMibs(pdm, loadedMibs);
			}
		});
        
        ClovisNotifyingListImpl elist = getUpdatedTableInput(loadedMibs);
        _tableViewer.setInput(elist);
        _tableViewer.getNotifier().addListener(new IEnvironmentListener() {
            public void valueChanged(Object obj)
            {
                if (obj.equals("selection")) {
                    EObject node = (EObject) ((IStructuredSelection)
                    		_tableViewer.getSelection()).getFirstElement();
                    if (node != null) {
                    	_selMibName = EcoreUtils.getValue(node, "MibPath").toString();
                    	createMibObjSelectionGroup();
                    }
                }
            }
        });
        
        //Add Buttons for table.
        Composite btnComp = new Composite(container, SWT.NONE);
        RowLayout rowLayout = new RowLayout(SWT.VERTICAL);
		rowLayout.fill = true;
		btnComp.setLayout(rowLayout);
		Button loadBtn = new Button(btnComp, SWT.PUSH);
		loadBtn.setText("Load");
		loadBtn.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				FileDialog fileselDialog = new FileDialog(getShell(), SWT.MULTI);
				fileselDialog.setFilterPath(UtilsPlugin
						.getDialogSettingsValue("LOADMIB"));
				fileselDialog.open();
				String fileNames[] = fileselDialog.getFileNames();
				for (int i = 0; i < fileNames.length; i++) {
					UtilsPlugin.saveDialogSettings("LOADMIB", fileselDialog
							.getFilterPath());
					String fileName = fileselDialog.getFilterPath()
							+ File.separator + fileNames[i];
					loadedMibs.add(fileName);
					ClovisNotifyingListImpl elist = getUpdatedTableInput(loadedMibs);
			        _tableViewer.setInput(elist);
				}
			}
		});
		Button unLoadBtn = new Button(btnComp, SWT.PUSH);
		unLoadBtn.setText("Unload");
					
        ScrolledComposite sComposite = new ScrolledComposite(
        		container, SWT.BORDER | SWT.V_SCROLL | SWT.H_SCROLL);
        sComposite.setExpandHorizontal(true);
        sComposite.setExpandVertical(true);
        _scrolledComposite = sComposite;
                
        sComposite.setLayout(new GridLayout());
        GridData gds = new GridData(GridData.FILL_BOTH);
       
        sComposite.setLayoutData(gds);
        
        final Group mibGroup = new Group(sComposite, SWT.SHADOW_OUT);
        mibGroup.setText(_selMibName);
        mibGroup.setBackground(ColorConstants.white);
        _mibObjComposite = mibGroup;
        _tableViewer.setValue("mibObjectsGroup", _mibObjComposite);
        GridData gd = new GridData(GridData.FILL_BOTH);
        mibGroup.setLayoutData(gd);
        mibGroup.setLayout(new GridLayout());
        
        sComposite.setContent(mibGroup);
        createMibObjSelectionGroup();
        _scrolledComposite.setMinSize(_mibObjComposite.computeSize(400, 200));
        
        unLoadBtn.addSelectionListener(new SelectionAdapter() {       	
			public void widgetSelected(SelectionEvent e) {
				StructuredSelection sel = (StructuredSelection) _tableViewer
						.getSelection();
				if (!(sel.isEmpty())) {
					for (int i = 0; i < sel.toList().size(); i++) {
						String filename = (String)EcoreUtils.getValue((EObject) sel.toList().get(i), "MibPath");
						loadedMibs.remove(filename);
					}
					_tableViewer.refresh();
					ClovisNotifyingListImpl elist1 = getUpdatedTableInput(loadedMibs);
			        _tableViewer.setInput(elist1);
			        mibGroup.setText("");
			        Control objects[] = mibGroup.getChildren();
			        for (int i = 0; i< objects.length; i++) {
			        	Control obj = objects[i];
			        	obj.dispose();
			        }
			        mibGroup.redraw();
			        mibGroup.getParent().redraw();
				}
			}
		});
        Button importButton = new Button(container, SWT.PUSH);
        importButton.setText("Import");
        importButton.addSelectionListener(new SelectionAdapter() {       	
			public void widgetSelected(SelectionEvent e)
			{ 
				List selObjs = new Vector();
				String mibName = ((Group) _mibObjComposite).getText();
				if (_mibObjComposite != null) {
					String message = "The following tables/groups/notifications are imported in to clovis information model\n";
					message = message + "====================================================================\n";
					Control[] controls = _mibObjComposite.
						getChildren();
					for (int i = 0; i < controls.length; i++) {
		    			Button control = (Button) controls[i];
		    			if (control.getSelection() && control.getData() instanceof MibTreeNode) {
		    				MibTreeNode mibNode = (MibTreeNode) control.getData();
		    				selObjs.add(mibNode);
		    				if (mibNode.isSnmpV2TrapNode()) {
		    					message = message + mibNode.getName() + " - imported to alarm model\n";
		    				} else {
		    					message = message + mibNode.getName() + " - imported to resource model\n";
		    				}
		    			}
		    		}

					if (selObjs.size() > 0) {
						Display.getDefault()
								.syncExec(
										new MibImportThread(_project, mibName,
												selObjs));

						message = message
								+ "====================================================================\n";
						createMibObjSelectionGroup();
						MessageDialog.openInformation(getShell(),
								"MIB Object Selection", message);
					}
				}
			}
		});

        container.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						"com.clovis.cw.help.import_mib");
			}
		});
        setTitle("Select MIB objects");
        //setMessage(DIALOG_MESSAGE, MessageDialog.INFORMATION);
        return super.createDialogArea(parent);
    }
    
    /**
     * Create the mib objects(table/group/notification) group
     * to select the groups 
     * @param parent - Parent Composite.
     */
    private void createMibObjSelectionGroup()
    {
    		if (_mibObjComposite != null) {
	    		Control[] controls = _mibObjComposite.getChildren();
	    		for (int i = 0; i < controls.length; i++) {
	    			Control control = controls[i];
	    			control.dispose();
	    		}
    		}
	        
    		if (!_selMibName.equals("")) {
	        	parseMibObjects(_selMibName);
		        ((Group) _mibObjComposite).setText(_selMibName);
		        
		        // add a select all/deselect all item to the list
		        if (_mibNodesList.size() > 0)
		        {
		    		TreeNode selectAllNode = new TreeNode(null);
			    	final Button selectAllButton = new Button(_mibObjComposite, SWT.CHECK);
			    	selectAllButton.setText(_selectAllText);
			    	selectAllButton.setData(selectAllNode);
			    	selectAllButton.setBackground(ColorConstants.white);
			    	selectAllButton.addSelectionListener(new SelectionAdapter() {
						public void widgetSelected(SelectionEvent e)
						{ 
							if (_mibObjComposite != null) {
								Control[] controls = _mibObjComposite.getChildren();
								for (int i = 0; i < controls.length; i++) {
					    			Button control = (Button) controls[i];
					    			control.setSelection(!_allSelected);
					    		}
								
								if (controls.length > 0)
								{
									_allSelected = !_allSelected;
									if (_allSelected)
									{
										selectAllButton.setText(_deselectAllText);
									} else {
										selectAllButton.setText(_selectAllText);
									}
								}
								
							}
						}
			        });
			    	_allSelected = false;
		        }
		        
		        Iterator iterator = _mibNodesList.iterator();
			    while (iterator.hasNext()) {
			    	MibTreeNode mibNode = (MibTreeNode) iterator.next();
			    	Button mibObjButton = new Button(_mibObjComposite, SWT.CHECK);
			    	if (mibNode.isSnmpV2TrapNode()) {
			    		mibObjButton.setText(mibNode.getName() 
			    				+ " [" + mibNode.getOID().toString() + "] = Alarm");
			    	} else {
			    		mibObjButton.setText(mibNode.getName() 
			    				+ " [" + mibNode.getOID().toString() + "] = Resource");
			    	}
			    		
			    	mibObjButton.setData(mibNode);
			    	mibObjButton.setBackground(ColorConstants.white);
			    	
			    }
		    
	        }
	       
	        _scrolledComposite.setMinSize(_mibObjComposite.computeSize(SWT.DEFAULT, SWT.DEFAULT));
	        _mibObjComposite.layout();
	        
    	
    }
    /**
     * 
     * @param mibFileName - Mib file name to be parsed
     * @return the List of table/group/notification objects in MIB
     */
    private void parseMibObjects(String mibFileName)
    {
    	MibTreeNode node = null;
        try {
        	MibUtil.unloadAllMibs();
        	ClovisMibUtils.loadSystemMibs(_project);
        	MibUtil.setResolveSyntax(true);

        	node = MibUtil.parseMib(mibFileName, false);
            if (node == null) {
            	ClovisMessageHandler.displayPopupError(
                        getShell(), "Mib File Loading Error", "Could not load MIB file. Error occured while parsing MIB file");
                LOG.error("Error occured while parsing MIB file");
            }
        } catch (MibParseException e) {
        	ClovisMessageHandler.displayPopupError(
            		getShell(), "Mib File Loading Error", e.toString());
            LOG.error("Error occured while parsing MIB file", e);
        } catch (IOException e) {
        	ClovisMessageHandler.displayPopupError(getShell(), "Mib File Loading Error",
                    "Could not load the MibFile. IO Exception has occured" + e);
            LOG.error("IO Exception has occured", e);
        } catch (Exception e) {
        	ClovisMessageHandler.displayPopupError(getShell(), "Mib File Loading Error",
                    "Could not load the MibFile. Exception has occured" + e);
            LOG.error("Exception has occured", e);
        }
        _mibNodesList.clear();
        ClovisMibUtils.getMibObjects(_project, node, _mibNodesList);
	}
    /**
     * Returns Table input
     * @param loadedMibs
     * @return
     */
    private ClovisNotifyingListImpl getUpdatedTableInput(List<String> loadedMibs){
    	 ClovisNotifyingListImpl elist = new ClovisNotifyingListImpl();
         for(int i = 0; i < loadedMibs.size(); i++) {
         	String filename = loadedMibs.get(i);
         	MibTreeNode node = null;
 			try {
 				node = MibUtil.parseMib(filename, false);
 				if (node == null) {
 	            	ClovisMessageHandler.displayPopupError(
 	                        getShell(), "Mib File Loading Error", "Could not load MIB file. Error occured while parsing MIB file");
 	                LOG.error("Error occured while parsing MIB file");
 	                return elist;
 	            }
 			} catch (MibParseException e1) {
 				ClovisMessageHandler.displayPopupError(
 	            		getShell(), "Mib File Loading Error", e1.toString());
 	            LOG.error("Error occured while parsing MIB file", e1);
 				return elist;
 			} catch (IOException e1) {
 				ClovisMessageHandler.displayPopupError(
 	            		getShell(), "Mib File Loading Error", e1.toString());
 	            LOG.error("Error occured while parsing MIB file", e1);
 				return elist;
 			}
         	EObject uiObj = ClovisMibUtils.convertToUiObject(node);
         	EcoreUtils.setValue(uiObj, "MibPath", filename);
            EcoreUtils.setValue(uiObj, "MibSize", (new Long((new File(
                 filename)).length())).toString());
         	elist.add(uiObj);
         }
         return elist;
    }
	/**
     * @param shell - New Shell
     * Configures the shell properties
     */
    protected void configureShell(Shell shell)
    {
        super.configureShell(shell);
        
        shell.setText(DIALOG_TITLE);
        //shell.setSize(800, 800);
    }
	/*@Override
	protected Control createButtonBar(Composite parent) {
		return null;
	}*/
	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		createButton(parent, IDialogConstants.CANCEL_ID,
				IDialogConstants.CLOSE_LABEL, false);
	}

    /*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed() {
		MibUtil.unloadAllMibs();
		super.cancelPressed();
	}
}
