/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Abstract Dialog implementation to capture the information required to create the model template and creates the same.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class CreateModelTemplateDialog extends TitleAreaDialog
		implements ModelTemplateConstants {

	protected EObject _modelTemplateObject;

	protected String _modelType;

	protected StructuredSelection _selection;;

	protected IProject _project;

	protected static final Log LOG = Log.getLog(CaPlugin.getDefault());

	private String DIALOG_DETAIL = "Model Template Details";

	private String DIALOG_TITLE = "Create Model Template";

	/**
	 * Constructor.
	 * 
	 * @param parentShell
	 * @param project
	 * @param selection
	 * @param modelType
	 */
	protected CreateModelTemplateDialog(Shell parentShell, IProject project,
			StructuredSelection selection, String modelType) {

		super(parentShell);
		_project = project;
		_selection = selection;
		_modelType = modelType;
	}

	/**
	 * Creates the new model template object from the ecore.
	 */
	protected void createModelTemplateObject() {
		_modelTemplateObject = ModelTemplateUtils.createEObjFromEcore("model" + File.separator
				+ ICWProject.MODEL_TEMPLATE_ECORE_FILENAME, "template");

		BaseEditPart bep = (BaseEditPart) _selection.getFirstElement();
		NodeModel nodeModel = (NodeModel) bep.getModel();
		EcoreUtils.setValue(_modelTemplateObject, FEATURE_MODEL_TEMPLATE_NAME,
				EcoreUtils.getName(nodeModel.getEObject()));

		EcoreUtils.setValue(_modelTemplateObject, FEATURE_RELEASE_VERSION,
				DataPlugin.getProductVersion());
		EcoreUtils.setValue(_modelTemplateObject, FEATURE_UPDATE_VERSION,
				String.valueOf(DataPlugin.getProductUpdateVersion()));
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {

		Composite container = new Composite(parent, SWT.NONE);
		container.setLayoutData(new GridData(GridData.FILL_BOTH));
		GridLayout containerLayout = new GridLayout();
		container.setLayout(containerLayout);

		setTitle(DIALOG_TITLE);
		getShell().setText(DIALOG_DETAIL);

		FormView formView = new FormView(container, SWT.NONE,
				_modelTemplateObject, getClass().getClassLoader(), this);
		formView.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		final String contextid = "com.clovis.cw.help.modeltemplate_create";
		container.addHelpListener(new HelpListener() {

			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						contextid);
			}
		});
		
		return container;
	}

    /*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TrayDialog#createButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createButtonBar(Composite parent) {
		Label buttonBarSeparator = new Label(parent, SWT.HORIZONTAL
				| SWT.SEPARATOR);
		buttonBarSeparator.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

		return super.createButtonBar(parent);
	}

	/**
	 * Polpulates the model template object with the configuration details captured from the user.
	 */
	protected abstract void populateModelTemplate();

	/**
	 * Adds the given entity object into the model template object.
	 * @param eObj
	 * @param featureName
	 */
	protected void addEntity(EObject eObj, String featureName) {
		if (_modelTemplateObject == null)
			return;

		EObject entitiesObj = (EObject) EcoreUtils.getValue(
				_modelTemplateObject, ModelTemplateConstants.FEATURE_ENTITIES);

		EStructuralFeature feature = entitiesObj.eClass()
				.getEStructuralFeature(featureName);
		entitiesObj.eSet(feature, eObj);
	}

	/**
	 * Saves the model template into the file.
	 * 
	 * @return true if successful, false otherwise
	 */
	@SuppressWarnings("unchecked")
	protected boolean saveModelTemplate() {
		Resource modelTemplateResource = null;
		try {
			String templateName = EcoreUtils.getName(_modelTemplateObject);
			String dataFilePath = ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH
					+ File.separator + templateName	+ File.separator
					+ templateName + "_" + _modelType + MODEL_TEMPLATE_FILE_EXT;

			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);
			if (xmlFile.exists()) {

				if (MessageDialog.openQuestion(getShell(),
						"Model Template Already Exist",
						"Do you want to replace the original template")) {

					xmlFile.delete();
					modelTemplateResource = EcoreModels.create(uri);

				} else {
					return false;
				}

			} else {
				modelTemplateResource = EcoreModels.create(uri);
			}

			modelTemplateResource.getContents().add(_modelTemplateObject);
			EcoreModels.save(modelTemplateResource);

		} catch (Exception e) {
			LOG.error("Error Saving Model Template Resource.", e);
			return false;
		}

		return true;
	}

	/**
	 * Updates the model template view by reflecting this newly created template
	 * into it.
	 */
	protected void updateModelTemplateView() {
		IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();

		if (page != null) {
			try {
				IViewPart part = page
						.showView("com.clovis.cw.workspace.modelTemplateView");

				if (part != null) {
					((ModelTemplateView) part).refreshModelTemplateView();
					((ModelTemplateView) part).selectModelTemplate(_modelTemplateObject);
				}

			} catch (PartInitException e) {
				LOG.error("Model Template View can not be initialized.", e);
			}
		}
	}

	// May be useful when implementing component template
	@SuppressWarnings("unchecked")
	private void getConnectionChildren(EObject obj, List containerList,
			String connectionType, String featureName,
			List<EObject> childrenList) {
		Object connObj = EcoreUtils.getValue(obj, connectionType);
		if (connObj != null) {
			if (connObj instanceof List) {
				List<EObject> connList = (List<EObject>) connObj;
				Iterator<EObject> itr = connList.iterator();
				while (itr.hasNext()) {
					EObject conn = itr.next();
					String targetName = EcoreUtils.getValue(conn, featureName)
							.toString();
					EObject targetObj = ClovisUtils.getObjectFrmName(
							containerList, targetName);
					if (!childrenList.contains(targetObj)) {
						childrenList.add(targetObj);
						getConnectionChildren(targetObj, containerList,
								connectionType, featureName, childrenList);
					}
				}
			} else {
				String targetName = EcoreUtils.getValue((EObject) connObj,
						featureName).toString();
				EObject targetObj = ClovisUtils.getObjectFrmName(containerList,
						targetName);
				if (!childrenList.contains(targetObj)) {
					childrenList.add(targetObj);
					getConnectionChildren(targetObj, containerList,
							connectionType, featureName, childrenList);
				}
			}
		}
	}
}
