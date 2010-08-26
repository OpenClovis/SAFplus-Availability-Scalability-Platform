/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/BuildTime.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
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

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
/**
 * @author shanth
 *
 * BuildTime Dialog.
 */
public class BuildTime extends PreferenceDialog
{
    private PreferenceManager    _preferenceManager;
    private IProject             _projectResource;
    private Model                _model;
    private static final String  DIALOG_TITLE = "Build Configurations";
    private static final Log LOG = Log.getLog(DataPlugin.getDefault());
    private String 			  _contextHelpId = "com.clovis.cw.help.build_config";
    /**
     * Constructor.
     * @param shell parent shell
     * @param pManager preference manager
     * @param proj Current Project
     * @throws IOException from addPreferenceNodes
     */
    public BuildTime(Shell shell, PreferenceManager pManager, IProject proj)
        throws IOException
    {
        super(shell, pManager);
        _preferenceManager    = pManager;
        _projectResource      = proj;
        addPreferenceNodes();
    }
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
        shell.setText(DIALOG_TITLE);
    }
    /**
     * Override okPressed to save Model.
     */
    protected void okPressed()
    {
        super.okPressed();
        if (_model != null) {
            try {
                _model.save(true);
            } catch (Exception e) {
                LOG.error("Model Saving Failed.", e);
            }
        }
    }
    /**
     * Unsubscribe events from Model.
     * @return super.close()
     */
    public boolean close()
    {
        if (_model != null) {
            _model.dispose();
        }
        return super.close();
    }
    /**
     * add preference nodes in to preference manager.
     * @throws IOException from createBuildTimeComponentObject
     */
    private void addPreferenceNodes() throws IOException
    {
        EObject eObj = createBuildTimeComponentObject();
        _model = new Model(eObj.eResource(), eObj).getViewModel();
        _preferenceManager.addToRoot(new PreferenceNode(DIALOG_TITLE,
          new BootConfigurationPage(
            BootTime.SYSTEM_DESCRIPTION, _model.getEObject().eContents())));
    }
    /**
     * Reads compile time configuration for ASP.
     * @return Top EObject from compileconfigs.xml
     * @throws IOException  In case of file IO issues.
     */
    private EObject createBuildTimeComponentObject()
        throws IOException
    {
        Resource resource = null;
        // This needs to be cleaned
        URL url = DataPlugin.getDefault().find(new Path("model" + File
                .separator + ICWProject
                .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME + File.separator
                + ICWProject.CW_COMPILE_TIME_COMPONENTS_FOLDER_NAME
                + File.separator + "Comps.ecore"));

        File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
        EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

        String dataFilePath = _projectResource.getLocation().toOSString()
            + File.separator
            + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
            + File.separator + "compileconfigs.xml";

        URI uri = URI.createFileURI(dataFilePath);
        File xmiFile = new File(dataFilePath);
        resource = xmiFile.exists() ? EcoreModels.getResource(uri)
                : EcoreModels.create(uri);
        List contents = resource.getContents();
        if (contents.isEmpty()) {
            EObject obj = EcoreUtils.createEObject((EClass) pack
                    .getEClassifier("ComponentsInfo"), true);
            resource.getContents().add(obj);
        }
        EObject obj   = (EObject) contents.get(0);
        List features = obj.eClass().getEAllStructuralFeatures();

        //If a new configuration component in added in Ecore later. It has
        //to be created in XMI.
        for (int i = 0; i < features.size(); i++) {
            EReference ref = (EReference) features.get(i);
            EObject refObj = (EObject) obj.eGet(ref);
            if (refObj == null) {
                obj.eSet(ref,
                      EcoreUtils.createEObject(ref.getEReferenceType(), true));
            }
        }
        EcoreModels.save(resource);
        return obj;
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
