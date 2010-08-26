/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/BootTime.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.NodeComponentModel;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * @author shanth
 *
 * BootTime configuration dialog.
 */
public class BootTime extends PreferenceDialog
{
    private PreferenceManager   _preferenceManager;
    private IProject            _projectResource;
    private ProjectDataModel _pdm = null; 
    private List                _nodeList;
    private Model                _buildModel;
    private Vector              _nodeComponentsVModel;
    private static BootTime instance    = null;
    private static final String  DIALOG_TITLE = "ASP Component Configuration";
    public  static final String SYSTEM_DESCRIPTION = "System Configuration";
    public  static final String BUILD_DESCRIPTION = "Build Configuration";
    public  static final String BOOT_DESCRIPTION = "Boot Configuration";
    private static final Log LOG = Log.getLog(DataPlugin.getDefault());
    private String 			  _contextHelpId = "com.clovis.cw.help.boot_config";
    private DependencyListener _dependencyListener;
    
    /**
     * Gets current instance of Dialog
     * @return current instance of Dialog
     */
    public static BootTime getInstance()
    {
        return instance;
    }
    /**
     * Close the dialog.
     * Remove static instance
     * @return super.close()
     */
    public boolean close()
    {
        if (_nodeComponentsVModel != null) {
			for (int i = 0; i < _nodeComponentsVModel.size(); i++) {
				Vector nodeVector = (Vector) _nodeComponentsVModel.get(i);

				for (int j = 0; j < nodeVector.size(); j++) {
					EcoreUtils.removeListener(((Model) nodeVector.get(j))
							.getEList(), _dependencyListener, -1);
				}
			}
		}

        instance = null;
        return super.close();
    }
    /**
     * Open the dialog.
     * Set static instance to itself
     * @return super.open()
     */
    public int open()
    {
        instance = this;
        return super.open();
    }
    /**
     * Constructor.
     * @param parentShell parent shell
     * @param pManager preference manager
     * @param nodes Node EObject List
     * @param projectResource Current Project
     */
    public BootTime(Shell parentShell, PreferenceManager pManager,
            List nodes, IProject projectResource)
    {
        super(parentShell, pManager);
        _preferenceManager = pManager;
        _projectResource = projectResource;
        _nodeList = nodes;
        _nodeComponentsVModel = new Vector(_nodeList.size());
         _pdm = ProjectDataModel
        .getProjectDataModel(_projectResource);
        addPreferenceNodes();
    }
    /**
     * @see org.eclipse.jface.preference.PreferenceDialog#createContents(org.eclipse.swt.widgets.Composite)
     */
    protected Control createContents(Composite parent) {
		Control control = super.createContents(parent);
		control.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						_contextHelpId);
			}
		});
		return control;
	}
    /**
     * Configure Shell.
     * @param shell shell
     */
    protected void configureShell(Shell shell)
    {
        super.configureShell(shell);
        shell.setText(_projectResource.getName() + " - " +DIALOG_TITLE);
    }
    /**
     * Override okPressed to save view model.
     */
    protected void okPressed()
    {
        super.okPressed();
        if (_buildModel != null) {
            try {
                _buildModel.save(true);
            } catch (Exception e) {
                LOG.error("Model Saving Failed.", e);
            }
        }
        for (int i = 0; i < _nodeComponentsVModel.size(); i++) {
            Vector nodeVector = (Vector) _nodeComponentsVModel.get(i);
            for (int j = 0; j < nodeVector.size(); j++) {
                ((Model) nodeVector.get(j)).save(true);
            }
        }
    }
    /**
     * Add preference nodes in the page.
     * @throws IOException
     */
    private void addPreferenceNodes()
    {
        /**** This code needs to be cleaned *****/


        _dependencyListener = new DependencyListener(_pdm, DependencyListener.VIEWMODEL_OBJECT);
        ArrayList systemCompList = new NodeComponentModel(
                _projectResource, "")
                .getSystemComponentResources();
        Vector systemViewModels = new Vector(systemCompList.size());
        for (int i = 0; i < systemCompList.size(); i++) {
            EObject tmpCompEObject = (EObject) systemCompList.get(i);
            NotifyingList tmpNList = new ClovisNotifyingListImpl();
            tmpNList.addAll(tmpCompEObject.eContents());

            Model tmpCompInfoModel = new Model(tmpCompEObject.eResource(),
                    tmpNList, tmpCompEObject.eClass().getEPackage());
            Model viewModel = tmpCompInfoModel.getViewModel();
            systemViewModels.add(viewModel);
    		EcoreUtils.addListener(viewModel.getEList(), _dependencyListener, -1);
        }
        List inputList = new Vector();
        for (int i = 0; i < systemViewModels.size(); i++) {
            inputList.add(((Model) systemViewModels.get(i)).
                    getEList().get(0));
        }
        EObject eObj = (EObject) _pdm.getBuildTimeComponentList().get(0);
        _buildModel = new Model(eObj.eResource(), eObj).getViewModel();
        _preferenceManager.addToRoot(new PreferenceNode(BUILD_DESCRIPTION,
          new BootConfigurationPage(
                  BUILD_DESCRIPTION, _buildModel.getEObject().eContents())));
        
        PreferenceNode bootNode = new PreferenceNode(BOOT_DESCRIPTION,
              new BootConfigurationPage(BOOT_DESCRIPTION, inputList));
        _preferenceManager.addToRoot(bootNode);
        _nodeComponentsVModel.add(systemViewModels);
        /*Vector tmpCompVector;
        for (int i = 0; i < _nodeList.size(); i++) {
            EObject tmpNode = (EObject) _nodeList.get(i);
            //NodeComponentModel creation for each Node needs to be replaced
            // with a function call
            ArrayList tmpArrayList = new NodeComponentModel(
                    _projectResource, EcoreUtils.getName(tmpNode))
                    .getBootTimeComponentResources();

            tmpCompVector = new Vector(tmpArrayList.size());
            for (int j = 0; j < tmpArrayList.size(); j++) {
                EObject tmpCompEObject = (EObject) tmpArrayList.get(j);
                NotifyingList tmpNList = new ClovisNotifyingListImpl();
                tmpNList.addAll(tmpCompEObject.eContents());

                Model tmpCompInfoModel = new Model(tmpCompEObject.eResource(),
                        tmpNList, tmpCompEObject.eClass().getEPackage());
                tmpCompVector.add(tmpCompInfoModel.getViewModel());
            }
            List list = new Vector();
            for (int j = 0; j < tmpCompVector.size(); j++) {
                list.add(((Model) tmpCompVector.get(j)).getEList().get(0));
            }
            systemNode.add(new PreferenceNode(DIALOG_TITLE,
                new BootConfigurationPage(EcoreUtils.getName(tmpNode), list)));
            _nodeComponentsVModel.add(tmpCompVector);
        }*/
    }
    
    /**
     *
     * @return the Resource List from Resource Editor
     */
    public List getResourceList()
    { 
        return _pdm.getCAModel().getEList();
    }
    /**
     * 
     * @return the List of all SAF nodes in component editor
     */
    public List getNodesList()
    {
        return _nodeList;
    }
    /**
    *
    * @return the View Model List
    */
   public Vector getBootConfigList()
   {
       return  _nodeComponentsVModel;
   }
   /***** This code needs to be removed in future ******/
   protected Control createHelpControl(Composite parent) {
		Image helpImage = JFaceResources.getImage(DLG_IMG_HELP);
		if (helpImage != null) {
			return createHelpImageButton(parent, helpImage);
		}
		return createHelpLink(parent);
   }
	private ToolBar createHelpImageButton(Composite parent, Image image) {
       ToolBar toolBar = new ToolBar(parent, SWT.FLAT | SWT.NO_FOCUS);
       ((GridLayout) parent.getLayout()).numColumns++;
		toolBar.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		final Cursor cursor = new Cursor(parent.getDisplay(), SWT.CURSOR_HAND);
		toolBar.setCursor(cursor);
		toolBar.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				cursor.dispose();
			}
		});		
       ToolItem item = new ToolItem(toolBar, SWT.NONE);
		item.setImage(image);
		item.setToolTipText(JFaceResources.getString("helpToolTip")); //$NON-NLS-1$
		item.addSelectionListener(new SelectionAdapter() {
           public void widgetSelected(SelectionEvent e) {
           	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
           }
       });
		return toolBar;
	}
	private Link createHelpLink(Composite parent) {
		Link link = new Link(parent, SWT.WRAP | SWT.NO_FOCUS);
       ((GridLayout) parent.getLayout()).numColumns++;
		link.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		link.setText("<a>"+IDialogConstants.HELP_LABEL+"</a>"); //$NON-NLS-1$ //$NON-NLS-2$
		link.setToolTipText(IDialogConstants.HELP_LABEL);
		link.addSelectionListener(new SelectionAdapter() {
           public void widgetSelected(SelectionEvent e) {
           	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
           }
       });
		return link;
	}
	/***** This code needs to be removed in future ******/
}
