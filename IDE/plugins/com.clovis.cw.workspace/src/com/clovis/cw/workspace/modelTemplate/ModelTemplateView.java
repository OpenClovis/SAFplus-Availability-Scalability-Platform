/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.ExpandBar;
import org.eclipse.swt.widgets.ExpandItem;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeColumn;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.ClassAssociationEditor;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.ComponentEditor;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GECommandStack;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.clovis.cw.workspace.ClovisNavigator;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.action.OpenComponentEditorAction;
import com.clovis.cw.workspace.action.OpenResourceEditorAction;
import com.clovis.cw.workspace.natures.SystemProjectNature;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * Support for viewing the structure of the model template, using them in the
 * project and import/export of them.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateView extends ViewPart implements
		ModelTemplateConstants {

	private EObject _modelTemplateObject;

	private Image _resourceIcon;

	private Image _componentIcon;

	private HashMap<String, String> _modelTemplateNameFileMap = new HashMap<String, String>();

	private org.eclipse.swt.widgets.List _resourceTemplateListControl;

	private org.eclipse.swt.widgets.List _componentTemplateListControl;

	private List<String> _resourceTemplateList = new ArrayList<String>();

	private List<String> _componentTemplateList = new ArrayList<String>();

	private TreeViewer _modelTemplateTreeViewer;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException {
		super.init(site);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose() {
		_resourceIcon.dispose();
		_componentIcon.dispose();

		super.dispose();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(final Composite parent) {
		GridLayout parentLayout = new GridLayout(3, true);
		parentLayout.marginWidth = parentLayout.marginHeight = parentLayout.horizontalSpacing = 0;
		parent.setLayout(parentLayout);

		loadModelTemplateDataStructures();

		Composite expandBarComposite = new Group(parent, SWT.SHADOW_ETCHED_IN);
		expandBarComposite.setLayoutData(new GridData(GridData.FILL_BOTH));
		GridLayout expandBarCompositeLayout = new GridLayout();
		expandBarCompositeLayout.marginWidth = expandBarCompositeLayout.marginHeight = 0;
		expandBarComposite.setLayout(expandBarCompositeLayout);

		ExpandBar expandBar = new ExpandBar(expandBarComposite, SWT.BORDER);
		expandBar.setBackground(Display.getCurrent().getSystemColor(
				SWT.COLOR_WHITE));
		expandBar.addExpandListener(new ModelTemplateViewExpandListener());
		expandBar.setLayoutData(new GridData(GridData.FILL_BOTH));

		Composite resourceComposite = new Composite(expandBar, SWT.NONE);
		resourceComposite.setLayoutData(new GridData(GridData.FILL_BOTH));
		GridLayout resourceCompositeLayout = new GridLayout();
		resourceCompositeLayout.marginWidth = resourceCompositeLayout.marginHeight = 0;
		resourceComposite.setLayout(resourceCompositeLayout);

		_resourceTemplateListControl = new org.eclipse.swt.widgets.List(
				resourceComposite, SWT.H_SCROLL | SWT.V_SCROLL);
		_resourceTemplateListControl.setLayoutData(new GridData(
				GridData.FILL_BOTH));
		_resourceTemplateListControl.setItems(_resourceTemplateList
				.toArray(new String[] {}));
		_resourceTemplateListControl
				.addSelectionListener(new ModelTemplateViewSelectionListener(
						this));

		ExpandItem resourceItem = new ExpandItem(expandBar, SWT.NONE);
		resourceItem.setText(MODEL_TYPE_RESOURCE + " Template");
		resourceItem.setHeight(resourceComposite.computeSize(SWT.DEFAULT,
				SWT.DEFAULT).y);
		resourceItem.setControl(resourceComposite);
		resourceItem.setImage(getIconImage(MODEL_TYPE_RESOURCE));

		Composite componentComposite = new Composite(expandBar, SWT.NONE);
		componentComposite.setLayoutData(new GridData(GridData.FILL_BOTH));
		GridLayout componentCompositeLayout = new GridLayout();
		componentCompositeLayout.marginWidth = componentCompositeLayout.marginHeight = 0;
		componentComposite.setLayout(componentCompositeLayout);

		_componentTemplateListControl = new org.eclipse.swt.widgets.List(
				componentComposite, SWT.H_SCROLL | SWT.V_SCROLL);
		_componentTemplateListControl.setLayoutData(new GridData(
				GridData.FILL_BOTH));
		_componentTemplateListControl.setItems(_componentTemplateList
				.toArray(new String[] {}));
		_componentTemplateListControl
				.addSelectionListener(new ModelTemplateViewSelectionListener(
						this));

		ExpandItem componentItem = new ExpandItem(expandBar, SWT.NONE);
		componentItem.setText(MODEL_TYPE_COMPONENT + " Template");
		componentItem.setHeight(componentComposite.computeSize(SWT.DEFAULT,
				SWT.DEFAULT).y);
		componentItem.setControl(componentComposite);
		componentItem.setImage(getIconImage(MODEL_TYPE_COMPONENT));

		Composite modelTemplateDetailsComposite = new Group(parent,
				SWT.SHADOW_ETCHED_IN);
		GridData detailsCompositeGridData = new GridData(GridData.FILL_BOTH);
		detailsCompositeGridData.horizontalSpan = 2;
		modelTemplateDetailsComposite.setLayoutData(detailsCompositeGridData);
		GridLayout detailsCompositeLayout = new GridLayout();
		detailsCompositeLayout.marginWidth = detailsCompositeLayout.marginHeight = 0;
		modelTemplateDetailsComposite.setLayout(detailsCompositeLayout);

		Label label = new Label(modelTemplateDetailsComposite, SWT.NONE);
		label.setText("Selected Template");
		label.pack();

		_modelTemplateTreeViewer = new TreeViewer(
				modelTemplateDetailsComposite, SWT.BORDER);
		Tree tree = _modelTemplateTreeViewer.getTree();
		tree.setLayoutData(new GridData(GridData.FILL_BOTH));
		tree.setHeaderVisible(true);
		tree.setLinesVisible(true);

		TreeColumn keyColumn = new TreeColumn(tree, SWT.NONE);
		keyColumn.setText("Key");
		keyColumn.setWidth(200);
		TreeColumn valColumn = new TreeColumn(tree, SWT.NONE);
		valColumn.setText("Value");
		valColumn.setWidth(200);

		_modelTemplateTreeViewer
				.setContentProvider(new KeyValueTreeContentProvider());
		_modelTemplateTreeViewer
				.setLabelProvider(new KeyValueTreeLabelProvider());
		_modelTemplateTreeViewer.setInput(null);

		contributeToToolBars();
	}

	/**
	 * Adds icons and actions to the toolbar.
	 */
	private void contributeToToolBars() {
		IToolBarManager toolBarManager = getViewSite().getActionBars()
				.getToolBarManager();

		ModelTemplateUseAction useAction = new ModelTemplateUseAction(this);
		toolBarManager.add(useAction);

		ModelTemplateImportAction importAction = new ModelTemplateImportAction(
				getSite().getShell(), this);
		toolBarManager.add(importAction);

		ModelTemplateExportAction exportAction = new ModelTemplateExportAction(
				getSite().getShell());
		toolBarManager.add(exportAction);

		ModelTemplateRefreshAction refreshAction = new ModelTemplateRefreshAction(
				this);
		toolBarManager.add(refreshAction);
	}

	/**
	 * Refreshes the model template view to reflect it with the file system
	 * files.
	 */
	public void refreshModelTemplateView() {
		loadModelTemplateDataStructures();
		_resourceTemplateListControl.setItems(_resourceTemplateList
				.toArray(new String[] {}));
		_componentTemplateListControl.setItems(_componentTemplateList
				.toArray(new String[] {}));
		_modelTemplateTreeViewer.setInput(null);
		
	}

	/**
	 * Loads the lists and maps having model template details.
	 */
	private void loadModelTemplateDataStructures() {
		clearModelTemplateDataStructures();

		File modelTemplateFolder = new File(MODEL_TEMPLATE_FOLDER_PATH);
		if (modelTemplateFolder.exists()) {

			File templateDirs[] = modelTemplateFolder.listFiles(new FileFilter() {
				public boolean accept(File file) {

					if (file.isDirectory()) {
						return true;
					}
					return false;
				}
			});

			List<String> templateFiles = new ArrayList<String>();
			for (File templateDir : templateDirs) {

				String files[] = templateDir.list(new FilenameFilter() {
					public boolean accept(File dir, String name) {

						if (ModelTemplateUtils
								.isModelTemplateFile(name)) {
							return true;
						}
						return false;
					}
				});

				templateFiles.addAll(Arrays.asList(files));
			}

			for (String fileName : templateFiles) {

				String templateName = fileName.substring(0, fileName
						.lastIndexOf("_"));

				if (ModelTemplateUtils.getModelTemplateTypeFromFile(fileName)
						.equals(MODEL_TYPE_RESOURCE)) {
					_resourceTemplateList.add(templateName);
					_modelTemplateNameFileMap.put(templateName, fileName);

				} else if (ModelTemplateUtils.getModelTemplateTypeFromFile(
						fileName).equals(MODEL_TYPE_COMPONENT)) {
					_componentTemplateList.add(templateName);
					_modelTemplateNameFileMap.put(templateName, fileName);
				}
			}
		}
	}

	/**
	 * Clears the lists and maps having model template details.
	 */
	private void clearModelTemplateDataStructures() {
		_modelTemplateNameFileMap.clear();
		_resourceTemplateList.clear();
		_componentTemplateList.clear();
	}

	/**
	 * Returns the icon for the Model Template type.
	 * 
	 * @param type
	 *            the type of the model template
	 * @return the image
	 */
	private Image getIconImage(String type) {
		URL iconURL;

		if (type.equals(MODEL_TYPE_RESOURCE)) {
/*			iconURL = FileLocator.find(
					WorkspacePlugin.getDefault().getBundle(), new Path("icons"
							+ File.separator + "resource_editor.gif"), null);
*/
			iconURL = WorkspacePlugin.getDefault().find(
					new Path("icons" + File.separator + "resource_editor.gif"));
			return _resourceIcon = ImageDescriptor.createFromURL(iconURL)
					.createImage();

		} else if (type.equals(MODEL_TYPE_COMPONENT)) {
/*			iconURL = FileLocator.find(
					WorkspacePlugin.getDefault().getBundle(), new Path("icons"
							+ File.separator + "component_editor.gif"), null);
*/
			iconURL = WorkspacePlugin.getDefault()
					.find(
							new Path("icons" + File.separator
									+ "component_editor.gif"));
			return _componentIcon = ImageDescriptor.createFromURL(iconURL)
					.createImage();
		}

		return null;
	}

	/**
	 * Loads the moedlTemplate Object with the given modeltemplate and updates
	 * the view by showing its structure.
	 * 
	 * @param modelTemplate
	 */
	public void selectModelTemplate(String modelTemplate) {
		_modelTemplateObject = readModelTemplate(_modelTemplateNameFileMap
				.get(modelTemplate));
		selectModelTemplate(_modelTemplateObject);
	}

	/**
	 * Updates model template view with the given object.
	 * 
	 * @param modelTemplateObj
	 */
	public void selectModelTemplate(EObject modelTemplateObj) {
		List<EObject> list = new ArrayList<EObject>();
		list.add(modelTemplateObj);

		_modelTemplateTreeViewer.setInput(list);
		_modelTemplateTreeViewer.refresh();
	}

	/**
	 * Reads the model template object from the file.
	 * 
	 * @param modelTemplate
	 * @return the model template object
	 */
	private EObject readModelTemplate(String modelTemplate) {
		File modelTemplateFolder = new File(MODEL_TEMPLATE_FOLDER_PATH);
		EObject modelTemplateObject = null;

		if (modelTemplateFolder.exists()) {

			try {
/*				URL modelTemplateEcoreURL = FileLocator.find(DataPlugin
						.getDefault().getBundle(), new Path(
						ICWProject.PLUGIN_MODELS_FOLDER + File.separator
								+ ICWProject.MODEL_TEMPLATE_ECORE_FILENAME),
						null);
*/
				URL modelTemplateEcoreURL = DataPlugin.getDefault().find(
						new Path(ICWProject.PLUGIN_MODELS_FOLDER
								+ File.separator
								+ ICWProject.MODEL_TEMPLATE_ECORE_FILENAME));
/*				File modelTemplateEcoreFile = new Path(FileLocator.resolve(
						modelTemplateEcoreURL).getPath()).toFile();
*/
				File modelTemplateEcoreFile = new Path(Platform.resolve(
						modelTemplateEcoreURL).getPath()).toFile();
				EcoreModels.get(modelTemplateEcoreFile.getAbsolutePath());

				String modelTemplateFilePath = modelTemplateFolder.getPath()
						+ File.separator
						+ modelTemplate.substring(0, modelTemplate
								.lastIndexOf("_")) + File.separator
						+ modelTemplate;
				URI modelTemplateFileUri = URI
						.createFileURI(modelTemplateFilePath);

				if(!new File(modelTemplateFilePath).exists()) {
					if (MessageDialog
							.openQuestion(
									getSite().getShell(),
									"File System has been modified",
									"The corresponding file has been deleted from the file system.\n\n"
											+ "Do you want to refresh the model template view to make it in sync with file system?")) {

						refreshModelTemplateView();
					}
					return null;
				}

				Resource modelTemplateResource = EcoreModels
						.getUpdatedResource(modelTemplateFileUri);
				modelTemplateObject = (EObject) modelTemplateResource
						.getContents().get(0);

			} catch (IOException e) {
				WorkspacePlugin.LOG.error(
						"Unable to read Model Template File.", e);
			}
		}

		return modelTemplateObject;
	}

	/**
	 * Handles the use of model template object into the editor.
	 */
	public void useModelTemplate() {
		IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();
		if (page != null) {

			ClovisNavigator navigator = ((ClovisNavigator) page
					.findView("com.clovis.cw.workspace.clovisWorkspaceView"));
			if(navigator == null) {
				MessageDialog.openWarning(getSite().getShell(),
						"Empty Project Selection",
						"Open Clovis Workspace View and Select OpenClovis Project to use model template in it.");
				return;
			}

			ISelection selection = navigator.getTreeViewer().getSelection();
			IProject project = null;

			if (selection instanceof IStructuredSelection) {
				IStructuredSelection sel = (IStructuredSelection) selection;

				if (sel.getFirstElement() instanceof IResource) {
					IResource resource = (IResource) sel.getFirstElement();
					project = resource.getProject();
				}
			}

			try {
				if (project != null
						&& project.exists()
						&& project.isOpen()
						&& project
								.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {

					if (_modelTemplateObject == null) {
						MessageDialog.openWarning(getSite().getShell(),
								"Empty Selection",
								"Select model template to use.");
						return;
					}

					String modelTemplateType = EcoreUtils.getValue(
							_modelTemplateObject,
							ModelTemplateConstants.FEATURE_MODEL_TYPE)
							.toString();
					ProjectDataModel pdm = ProjectDataModel
							.getProjectDataModel(project);
					GenericEditorInput geInput = null;

					EObject entityObj = (EObject) EcoreUtils.getValue(
							_modelTemplateObject,
							ModelTemplateConstants.FEATURE_ENTITIES);
					entityObj = EcoreCloneUtils.cloneEObject(entityObj);
					EObject editorEntityObj = null;

					if (modelTemplateType
							.equals(ModelTemplateConstants.MODEL_TYPE_RESOURCE)) {
						editorEntityObj = (EObject) EcoreUtils
								.getValue(
										entityObj,
										ModelTemplateConstants.FEATURE_RESOURCE_INFORMATION);

						OpenResourceEditorAction.openResourceEditor(project);
						geInput = (GenericEditorInput) pdm.getCAEditorInput();

						List<String> duplicateList = getDuplicateEntitiesForResourceTemplate(
								geInput, entityObj, pdm);
						if (duplicateList.size() != 0) {
							displayDuplicateMessage(duplicateList);
							return;
						}

						FormatConversionUtils.convertToEditorSupportedData(editorEntityObj,
								editorEntityObj, modelTemplateType + " Editor");
						ModelTemplateUseCommand command = new ModelTemplateUseCommand(
								geInput.getEditor().getEditorModel(), editorEntityObj);
						GECommandStack commandStack = (GECommandStack) geInput
								.getEditor().getCommandStack();
						commandStack.execute(command);

						addEntitiesForResourceTemplate(geInput, entityObj, pdm);

					} else if (modelTemplateType
							.equals(ModelTemplateConstants.MODEL_TYPE_COMPONENT)) {
						editorEntityObj = (EObject) EcoreUtils
								.getValue(
										entityObj,
										ModelTemplateConstants.FEATURE_COMPONENT_INFORMATION);

						boolean resEntityAvailable = false;
						GenericEditorInput resEditorInput = null;
						if (getChildObjectList((EObject) EcoreUtils.getValue(
								entityObj, ModelTemplateConstants.FEATURE_RESOURCE_INFORMATION))
								.size() != 0) {

							resEntityAvailable = true;
							OpenResourceEditorAction
									.openResourceEditor(project);
							resEditorInput = (GenericEditorInput) pdm
									.getCAEditorInput();
						}

						OpenComponentEditorAction.openComponentEditor(project);
						geInput = (GenericEditorInput) pdm
								.getComponentEditorInput();
						EditorModel geModel = geInput.getEditor()
								.getEditorModel();
						List<EObject> compEditorEntities = ComponentDataUtils
								.getEntityList((EObject) geModel.getEList()
										.get(0));

						FormatConversionUtils.convertToEditorSupportedData(
								editorEntityObj, editorEntityObj,
								modelTemplateType + " Editor");
						List<EObject> templateCompEntities = getChildObjectList(editorEntityObj);
						List<EObject> editorCompDupEntities = ModelTemplateUtils
								.getEObjectsHavingNames(
										compEditorEntities,
										ModelTemplateUtils
												.getNameListFromEObjList(templateCompEntities));

						removeDuplicateConnectionsFromTemplate(
								templateCompEntities,
								ModelTemplateUtils
										.getNameListFromEObjList(editorCompDupEntities));

						if (editorCompDupEntities.size() == 0
								&& (!resEntityAvailable || getDuplicateEntitiesForResourceTemplate(
										resEditorInput, entityObj, pdm).size() == 0)) {
							if (!MessageDialog
									.openConfirm(
											getSite().getShell(),
											"Undo not supported",
											"This operation will make changes to editor model for which undo support is not available."
													+ " This operation will also not allow to undo earlier changes.")) {
								return;
							}
							for (EObject templateCompEntity : templateCompEntities) {
								geModel.addEObject(templateCompEntity);
							}

						} else {
							boolean overwrite = MessageDialog
									.openConfirm(
											getSite().getShell(),
											"Overwrite duplicate entities",
											"Some entities in editor are having same name as the entities in model template."
													+ " Overwrite will replace them in editor."
													+ "\nSelect 'Cancel' to cancel the operation and change the entity names in editor."
													+ "\n\nNote: This operation will make changes to editor model for which undo support is not available."
													+ " This operation will also not allow to undo earlier changes.");

							if (!overwrite) {
								return;
							}

							EObject editorCompEntity;
							List<String> featuresToSkip = new ArrayList<String>();
							featuresToSkip.add(ModelConstants.RDN_FEATURE_NAME);

							for (EObject templateCompEntity : templateCompEntities) {
								editorCompEntity = ClovisUtils
										.getObjectFrmName(
												editorCompDupEntities,
												EcoreUtils
														.getName(templateCompEntity));
								if (editorCompEntity == null) {
									geModel.addEObject(templateCompEntity);
								} else {
									EcoreUtils.copyEObject(templateCompEntity,
											editorCompEntity, featuresToSkip);
								}
							}
						}

						addEntitiesForComponentTemplate(geInput, entityObj, pdm, resEditorInput);
						copyModelTemplateSource(project);
					}

				} else {
					MessageDialog.openWarning(getSite().getShell(),
							"Empty Project Selection",
							"Select OpenClovis Project to use model template in it.");
				}

			} catch (CoreException e) {
				WorkspacePlugin.LOG.warn(
						"Could not initialize the selected project", e);
			}
		}
	}

	/**
	 * Returns the list of entity names those are duplicate in resource template
	 * and the model.
	 * 
	 * @param geInput
	 * @param entityObj
	 * @param pdm
	 * @return the duplicate entity list
	 */
	@SuppressWarnings("unchecked")
	private List<String> getDuplicateEntitiesForResourceTemplate(
			GenericEditorInput geInput, EObject entityObj, ProjectDataModel pdm) {
		List<String> duplicateList = new ArrayList<String>();

		List<EObject> resList = ResourceDataUtils.getResourcesList(geInput
				.getEditor().getEditorModel().getEList());
		List<String> resNameList = ModelTemplateUtils
				.getNameListFromEObjList(resList);

		EObject resEntityObj = (EObject) EcoreUtils.getValue(entityObj,
				ModelTemplateConstants.FEATURE_RESOURCE_INFORMATION);
		duplicateList.addAll(ModelTemplateUtils
				.getDuplicateEObjNameList(ModelTemplateUtils
						.getEObjListFromChildReferences(resEntityObj),
						resNameList));

		EObject alarmInfo = pdm.getAlarmProfiles().getEObject();
		List<EObject> alarmList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmInfo);
		List<String> alarmNameList = ModelTemplateUtils
				.getNameListFromEObjList(alarmList);

		EObject alarmEntityObj = (EObject) EcoreUtils.getValue(entityObj,
				ModelTemplateConstants.FEATURE_ALARM_INFORMATION);

		duplicateList.addAll(ModelTemplateUtils.getDuplicateEObjNameList(
				ModelTemplateUtils
						.getEObjListFromChildReferences(alarmEntityObj),
				alarmNameList));

		return duplicateList;
	}

	/**
	 * Displays the duplicate entries to the user.
	 * 
	 * @param duplicateList
	 */
	private void displayDuplicateMessage(List<String> duplicateList) {
		String msg = "";

		Iterator<String> itr = duplicateList.iterator();
		while (itr.hasNext()) {
			msg += itr.next() + ", ";
		}

		msg = msg.substring(0, msg.length() - 2) + ".";
		MessageDialog
				.openWarning(
						getSite().getShell(),
						"Duplicate Entries",
						"Following entities are having duplicate entries.\n\n"
								+ msg
								+ "\n\nRename them in the model in order to use the template.");
	}

	/**
	 * Adds the entities from the resource template into the model.
	 * 
	 * @param geInput
	 * @param entityObj
	 * @param pdm
	 */
	private void addEntitiesForResourceTemplate(GenericEditorInput geInput,
			EObject entityObj, ProjectDataModel pdm) {
		EObject alarmEntityObj = (EObject) EcoreUtils.getValue(entityObj,
				ModelTemplateConstants.FEATURE_ALARM_INFORMATION);
		List<EObject> alarmEntityList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmEntityObj);
		ModelTemplateUtils.setRDN(alarmEntityList, false);
		EObject alarmInfo = pdm.getAlarmProfiles().getEObject();
		ClovisUtils.addObjectsToModel(alarmEntityList, alarmInfo);
		pdm.getAlarmProfiles().save(true);

		EObject alarmRuleEntityObj = (EObject) EcoreUtils.getValue(entityObj,
				ModelTemplateConstants.FEATURE_ALARM_RULE_INFORMATION);
		List<EObject> alarmRuleEntityList = ModelTemplateUtils
				.getEObjListFromChildReferences(alarmRuleEntityObj);
		EObject alarmRuleInfo = ((ClassAssociationEditor) geInput.getEditor())
				.getAlarmRuleViewModel().getEObject();
		ClovisUtils.addObjectsToModel(alarmRuleEntityList, alarmRuleInfo);

		EObject resAlarmMapEntityObj = (EObject) EcoreUtils.getValue(entityObj,
				ModelTemplateConstants.FEATURE_RESOURCE_ALARM_MAP_INFORMATION);
		EObject resAlarmLinkEntityObj = (EObject) ((List) EcoreUtils.getValue(
				resAlarmMapEntityObj, "link")).get(0);
		List resAlarmMapList = (List) EcoreUtils.getValue(resAlarmLinkEntityObj,
				"linkDetail");
		EObject resAlarmMapInfo = ((ClassAssociationEditor) geInput.getEditor())
				.getLinkViewModel().getEObject();
		EObject resAlarmLinkInfoObj = (EObject) ((List) EcoreUtils.getValue(
				resAlarmMapInfo, "link")).get(0);
		List resAlarmMapInfoList = (List) EcoreUtils.getValue(resAlarmLinkInfoObj,
		"linkDetail");
		resAlarmMapInfoList.addAll(resAlarmMapList);
//		ClovisUtils.addObjectsToModel(resAlarmMapList, resAlarmLinkInfoObj);
	}

	/**
	 * Adds the entities from the component template into the model.
	 * 
	 * @param geInput
	 * @param entityObj
	 * @param pdm
	 * @param resEditorInput
	 */
	private void addEntitiesForComponentTemplate(GenericEditorInput geInput,
			EObject entityObj, ProjectDataModel pdm,
			GenericEditorInput resEditorInput) {

		EObject resEntityObj = (EObject) EcoreUtils.getValue(entityObj,
				ModelTemplateConstants.FEATURE_RESOURCE_INFORMATION);
		FormatConversionUtils.convertToEditorSupportedData(resEntityObj,
				resEntityObj, FormatConversionUtils.RESOURCE_EDITOR);
		List<EObject> templateCompEntities = getChildObjectList(resEntityObj);

		EditorModel resModel = resEditorInput.getEditor().getEditorModel();
		for (EObject templateCompEntity : templateCompEntities) {
			resModel.addEObject(templateCompEntity);
		}

		EObject compResMapEntityObj = (EObject) EcoreUtils
				.getValue(
						entityObj,
						ModelTemplateConstants.FEATURE_COMPONENT_RESOURCE_MAP_INFORMATION);
		EObject compResLinkEntityObj = (EObject) ((List) EcoreUtils.getValue(
				compResMapEntityObj, "link")).get(0);
		List<EObject> compResMapLinkDetailEntityList = (List<EObject>) EcoreUtils.getValue(compResLinkEntityObj,
				"linkDetail");

		EObject compResMapInfo = ((ComponentEditor) geInput.getEditor())
				.getLinkViewModel().getEObject();
		EObject compResLinkInfoObj = (EObject) ((List) EcoreUtils.getValue(
				compResMapInfo, "link")).get(0);
		List compResMapLinkDetailList = (List) EcoreUtils.getValue(
				compResLinkInfoObj, "linkDetail");

		for (EObject compResMapLinkDetailEntity : compResMapLinkDetailEntityList) {
			EObject compResMapLinkDetail = ClovisUtils
					.getEobjectWithFeatureVal(compResMapLinkDetailList,
							"linkSource", EcoreUtils.getValue(
									compResMapLinkDetailEntity, "linkSource")
									.toString());

			if (compResMapLinkDetail == null) {
				compResMapLinkDetailList.add(EcoreCloneUtils
						.cloneEObject(compResMapLinkDetailEntity));

			} else {
				List<String> compResMapLinkTargetEntityList = (List<String>) EcoreUtils
						.getValue(compResMapLinkDetailEntity, "linkTarget");
				List<String> compResMapLinkTargetList = (List<String>) EcoreUtils
						.getValue(compResMapLinkDetail, "linkTarget");

				for (String linkTargetEntity : compResMapLinkTargetEntityList) {
					if (!compResMapLinkTargetList.contains(linkTargetEntity)) {
						compResMapLinkTargetList.add(linkTargetEntity);
					}
				}
			}
		}

		addEntitiesForResourceTemplate(resEditorInput, entityObj, pdm);
	}

	/**
	 * Copies model template source into project.
	 * 
	 * @param project
	 */
	private void copyModelTemplateSource(IProject project) {
		File modelTemplateSourceFolder = new File(
				ModelTemplateConstants.MODEL_TEMPLATE_FOLDER_PATH
						+ File.separator
						+ EcoreUtils.getName(_modelTemplateObject)
						+ "_"
						+ EcoreUtils.getValue(_modelTemplateObject,
								ModelTemplateConstants.FEATURE_MODEL_TYPE)
						+ File.separator + "src");
		if (!modelTemplateSourceFolder.exists()) {
			return;
		}

		String projectSourceLocation = CwProjectPropertyPage
				.getSourceLocation(project);
		if (projectSourceLocation.equals("")) {
			MessageDialog
					.openWarning(
							getSite().getShell(),
							"Model template source not copied",
							"Project source location not configured. Configure source location or manually copy source from model template later.");
			return;
		}

		File sourceFolder = new File(projectSourceLocation + File.separator
				+ "app");
		try {
			ClovisFileUtils.copyDirectory(modelTemplateSourceFolder,
					sourceFolder, false);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Creates the object list from the top level object.
	 * 
	 * @param topEditorObj
	 * @return 
	 */
	private List<EObject> getChildObjectList(EObject topEditorObj) {

		List refList = topEditorObj.eClass().getEAllReferences();
		List<EObject> childObjList = new ArrayList<EObject>();
		for (int i = 0; i < refList.size(); i++) {
			EReference ref = (EReference) refList.get(i);

			Object val = topEditorObj.eGet(ref);
			if (val != null) {

				if (val instanceof EObject) {
					childObjList.add((EObject) val);

				} else if (val instanceof List) {

					List valList = (List) val;
					for (int j = 0; j < valList.size(); j++) {
						childObjList.add((EObject) valList.get(j));
					}
				}
			}
		}

		return childObjList;
	}

	/**
	 * Removes connection objects for the model template which are duplicate in
	 * editor.
	 * 
	 * @param templateCompEntities
	 *            Components in the model template
	 * @param duplicateCompEntityNames
	 *            List of duplicate objects between model template and editor.
	 */
	@SuppressWarnings("unchecked")
	private void removeDuplicateConnectionsFromTemplate(
			List<EObject> templateCompEntities,
			List<String> duplicateCompEntityNames) {

		List<String> dupRDNList = ClovisUtils.getFeatureValueList(
				ModelTemplateUtils.getEObjectsHavingNames(templateCompEntities,
						duplicateCompEntityNames),
				ModelConstants.RDN_FEATURE_NAME);
		String source, target;
		EObject templateCompEntity;

		Iterator<EObject> itr = templateCompEntities.iterator();
		while (itr.hasNext()) {
			templateCompEntity = itr.next();

			if (templateCompEntity.eClass().getName().equals(
					ComponentEditorConstants.AUTO_NAME)) {

				source = EcoreUtils.getValue(templateCompEntity,
						ComponentEditorConstants.CONNECTION_START).toString();
				target = EcoreUtils.getValue(templateCompEntity,
						ComponentEditorConstants.CONNECTION_END).toString();

				if (dupRDNList.contains(source) && dupRDNList.contains(target)) {
					itr.remove();
				}
			}
		}
	}
}
