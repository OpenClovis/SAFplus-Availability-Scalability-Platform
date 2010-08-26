/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.TreeSet;

import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.swt.widgets.Display;
import org.w3c.dom.Document;
import org.w3c.dom.Node;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.action.OpenComponentEditorAction;
import com.clovis.cw.workspace.action.OpenResourceEditorAction;
import com.clovis.cw.workspace.project.FolderCreator;

/**
 * This class manages the entire migration process.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationManager implements MigrationConstants {

	private IProject _project;

	private String _currentVersion, _targetVersion;

	private int _currentUpdateVersion, _targetUpdateVersion;

	/**
	 * Constructor.
	 * 
	 * @param project
	 * @param targetVersion
	 * @param targetUpdateVersion
	 */
	public MigrationManager(IProject project) {
		_project = project;
		_targetVersion = DataPlugin.getProductVersion();
		_targetUpdateVersion = DataPlugin.getProductUpdateVersion();

		_currentVersion = MigrationUtils.getProjectVersion(project);
		_currentUpdateVersion = MigrationUtils.getProjectUpdateVersion(project);
	}

	/**
	 * Migrates the project.
	 */
	public void migrateProject() {

		try {
			boolean is22Model = false;
			if(_currentVersion.equals("2.2.0")) {
				is22Model = true;
			}

			if (MigrationUtils.isMigrationRequired(_currentVersion, "2.3.0",
					_currentUpdateVersion, 0)) {
				com.clovis.cw.workspace.project.migration.MigrationManager manager = new com.clovis.cw.workspace.project.migration.MigrationManager(
						_currentVersion, "2.3.0");
				IProject[] selProjs = { _project };
				manager.readFilesAndUpdate(selProjs, UtilsPlugin.isCmdToolRunning());
				_currentVersion = "2.3.0";
				_currentUpdateVersion = 0;
			}

			URL migrationFolderURL = FileLocator.find(DataPlugin.getDefault()
					.getBundle(), new Path(ICWProject.PLUGIN_MIGRATION_FOLDER),
					null);
			File migrationFolder = new File(FileLocator.resolve(
					migrationFolderURL).getPath());
			String migrationFiles[] = migrationFolder.list();

			HashMap<String, String> versionFileMap = new HashMap<String, String>();
			String file, version;
			for (int i = 0; i < migrationFiles.length; i++) {
				file = migrationFiles[i];

				if (file.startsWith("migration_") && file.endsWith(".xml")) {
					version = file.substring(file.indexOf("_") + 1, file
							.lastIndexOf("."));
					if (version.equals(_currentVersion)
							|| MigrationUtils.isMigrationRequired(
									_currentVersion, version, 0, 0)) {
						versionFileMap.put(version, file);
					}
				}
			}

			TreeSet<String> versionSet = new TreeSet<String>(
					new Comparator<String>() {
						public int compare(String o1, String o2) {
							if (MigrationUtils
									.isMigrationRequired(o1, o2, 0, 0)) {
								return -1;
							}
							return 1;
						}
					});
			versionSet.addAll(versionFileMap.keySet());

			loadMigrationEcore();

			String migrationFolderPath = migrationFolder.getAbsolutePath()
					+ File.separator;
			Iterator<String> versionIterator = versionSet.iterator();
			if (versionIterator.hasNext()) {
				migrate(migrationFolderPath
						+ versionFileMap.get(versionIterator.next()),
						_currentUpdateVersion + 1, -1);
			}
			while (versionIterator.hasNext()) {
				migrate(migrationFolderPath
						+ versionFileMap.get(versionIterator.next()), 1, -1);
			}

			unloadProjectDataModel();
			copyDefaultProjectFiles();
			updateProjectVersion();

			if (!UtilsPlugin.isCmdToolRunning()) {
				final boolean is22ModelFinal = is22Model;

				Display.getDefault().syncExec(new Runnable() {
					public void run() {

						OpenResourceEditorAction.openResourceEditor(_project);
						OpenComponentEditorAction.openComponentEditor(_project);

						if (is22ModelFinal) {
							MigrationUtils.autoArrangeEditors(_project);
						}
					}
				});
			}

		} catch (Exception e) {
			MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
					"Error migrating the project : '" + _project.getName()
							+ ".", e);
		}
	}

	/**
	 * Copies the default files required for the project.
	 */
	private void copyDefaultProjectFiles() {

		try {
			IFolder codeGenFolder = _project
					.getFolder(ICWProject.PROJECT_CODEGEN_FOLDER);

			if (codeGenFolder.exists()) {
				codeGenFolder.delete(true, null);
			}

			FolderCreator fd = new FolderCreator(_project);
			fd.createDefaultConfigFiles(true);
			fd.createDefaultModelFiles();
			fd.copyScript();
			fd.copyTemplates();
			fd.copyCodeGenFiles();

		} catch (Exception e) {
			MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
					"Failed to update dfault project files for the Project : "
							+ _project.getName() + ".", e);
		}
	}

	/**
	 * Unloads the project data model.
	 */
	private void unloadProjectDataModel() {
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);

		if (pdm != null) {
			pdm.removeDependencyListeners();
			ProjectDataModel.removeProjectDataModel(_project);
		}
	}

	/**
	 * Updates the project version to the latest.
	 */
	private void updateProjectVersion() {

		try {
			IProjectDescription description = _project.getDescription();
			description.setComment("Project Version:" + _targetVersion
					+ ":Update:" + _targetUpdateVersion);
			_project.setDescription(description, null);

		} catch (CoreException e) {
			MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
					"Error updating the project version for the Project : "
							+ _project.getName() + ".", e);
		}
	}

	/**
	 * Loads the migration ecore.
	 */
	private static void loadMigrationEcore() {

		try {
			URL migrationEcoreURL = FileLocator.find(DataPlugin.getDefault()
					.getBundle(), new Path(ICWProject.PLUGIN_MIGRATION_FOLDER
					+ File.separator + ICWProject.MIGRATION_XMI_DATA_FILENAME
					+ ".ecore"), null);

			File migrationEcoreFile = new Path(FileLocator.resolve(
					migrationEcoreURL).getPath()).toFile();
			EcoreModels.get(migrationEcoreFile.getAbsolutePath());

		} catch (IOException e) {
			MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
					"Error loading Migration Ecore.", e);
		}
	}

	/**
	 * Performs migration changes for the given file.
	 * 
	 * @param migrationFile
	 * @param startUpdateVersion
	 * @param endUpdateVersion
	 */
	@SuppressWarnings("unchecked")
	private void migrate(String migrationFile, int startUpdateVersion,
			int endUpdateVersion) {

		URI migrationFileUri = URI.createFileURI(migrationFile);
		Resource migrationResource = EcoreModels
				.getUpdatedResource(migrationFileUri);

		EObject migrationInfoObject = (EObject) migrationResource.getContents()
				.get(0);
		List<EObject> updateList = (List<EObject>) EcoreUtils.getValue(
				migrationInfoObject, "update");

		if (endUpdateVersion == -1) {
			endUpdateVersion = updateList.size();
		}

		for (int i = startUpdateVersion - 1; i < endUpdateVersion; i++) {
			performUpdate(updateList.get(i));
		}
	}

	/**
	 * Performs the update.
	 * 
	 * @param updateObj
	 */
	@SuppressWarnings("unchecked")
	private void performUpdate(EObject updateObj) {
		List<EObject> changeList = (List<EObject>) EcoreUtils.getValue(
				updateObj, "change");

		Iterator<EObject> iterator = changeList.iterator();
		while (iterator.hasNext()) {
			performChange(iterator.next());
		}
	}

	/**
	 * Performs the change.
	 * 
	 * @param changeObj
	 */
	@SuppressWarnings("unchecked")
	private void performChange(EObject changeObj) {

		try {
			Node rootNode = null;
			Document document = null;
			String sourceXML = EcoreUtils.getValue(changeObj, "sourceXML")
					.toString();

			if (!sourceXML.equals("")) {
				sourceXML = _project.getLocation().toOSString()
						+ File.separator
						+ sourceXML.replace("/", File.separator);

				document = MigrationUtils.buildDocument(sourceXML);
				if (document == null) {
					return;
				}
				rootNode = document.getDocumentElement();

				Object checkStringObj = EcoreUtils.getValue(changeObj, "checkString");
				if(checkStringObj != null) {
					String checkString = checkStringObj.toString().split("#")[0];
					int condition =  Integer.parseInt(checkStringObj.toString().split("#")[1]);

					switch(condition) {

					case CHECKSTRING_CONTAIN:
						if(!MigrationStringUtils.fileContainsString(sourceXML, checkString)) {
							return;
						}
						break;

					case CHECKSTRING_DOESNOT_CONTAIN:
						if(MigrationStringUtils.fileContainsString(sourceXML, checkString)) {
							return;
						}
						break;

					case CHECKELEMENT_CONTAIN:
						if(MigrationUtils.getNodesForPath(rootNode, checkString).size() == 0) {
							return;
						}
						break;

					case CHECKELEMENT_DOESNOT_CONTAIN:
						if(MigrationUtils.getNodesForPath(rootNode, checkString).size() != 0) {
							return;
						}
						break;
					}
				}
			}

			List<EObject> migrationStepList = (List<EObject>) EcoreUtils
					.getValue(changeObj, "migrationStep");

			Iterator<EObject> iterator = migrationStepList.iterator();
			while (iterator.hasNext()) {
				performMigrationStep(iterator.next(), rootNode);
			}

			if (!sourceXML.equals("")) {
				MigrationUtils.saveDocument(document, sourceXML);
			}
		} catch (Exception e) {
			MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
					"Error performing migration change : '"
							+ _project.getName() + ".", e);
		}
	}

	/**
	 * Performs migration change.
	 * 
	 * @param migrationStepObject
	 * @param rootNode
	 */
	private void performMigrationStep(EObject migrationStepObject, Node rootNode) {

		try {
			String details = EcoreUtils
					.getValue(migrationStepObject, "details").toString();
			String detailsArray[] = details.split("#");

			switch (Integer.parseInt(EcoreUtils.getValue(migrationStepObject,
					"type").toString())) {

			case TYPE_HANDLER:
				MigrationUtils.callHandler(_project, detailsArray);
				break;

			case TYPE_ADD_ATTR:
				String value = "";
				if(detailsArray.length == 3) {
					value = detailsArray[2];
				}
				MigrationUtils.addAttribute(rootNode, detailsArray[0],
						detailsArray[1], value);
				break;

			case TYPE_ADD_ELEMENT_CHILD:
				MigrationUtils.addElementAsChild(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_ADD_ELEMENT_SIBLING:
				MigrationUtils.addElementAsSibling(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_ADD_ELEMENT_ROOT:
				MigrationUtils.addElementAsRoot(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_ADD_TEXT:
				MigrationUtils.addText(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_RENAME_ATTR:
				MigrationUtils.renameAttribute(rootNode, detailsArray[0],
						detailsArray[1], detailsArray[2]);
				break;

			case TYPE_RENAME_ELEMENT:
				MigrationUtils.renameNode(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_CHANGEVAL_ATTR:
				MigrationUtils.changeAttributeValue(rootNode, detailsArray[0],
						detailsArray[1], detailsArray[2]);
				break;

			case TYPE_CHANGEVAL_ATTR_MATCH:
				MigrationUtils.changeAttributeValueMatch(rootNode,
						detailsArray[0], detailsArray[1], detailsArray[2],
						detailsArray[3]);
				break;

			case TYPE_CHANGEVAL_ELEMENT:
				MigrationUtils.changeNodeValue(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_CHANGEVAL_ELEMENT_MATCH:
				MigrationUtils.changeNodeValueMatch(rootNode, detailsArray[0],
						detailsArray[1], detailsArray[2]);
				break;

			case TYPE_REMOVE_ATTR:
				MigrationUtils.removeAttribute(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_REMOVE_ELEMENT:
				MigrationUtils.removeNode(rootNode, detailsArray[0]);
				break;

			case TYPE_MOVE_ATTR:
				MigrationUtils.moveAttribute(rootNode, detailsArray[0],
						detailsArray[1], detailsArray[2]);
				break;

			case TYPE_MOVE_ELEMENT_CHILD:
				MigrationUtils.moveNodeAsChild(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_MOVE_ELEMENT_SIBLING:
				MigrationUtils.moveNodeAsSibling(rootNode, detailsArray[0],
						detailsArray[1]);
				break;

			case TYPE_MOVE_ELEMENT_PATH:
				MigrationUtils.moveNodeForPath(rootNode, detailsArray[0],
						detailsArray[1], detailsArray[2]);
				break;

			case TYPE_MOVE_ATTRTOELEMENT_PATH:
				MigrationUtils.moveAttrToElementForPath(rootNode, detailsArray[0],
						detailsArray[1], detailsArray[2]);
				break;

			case TYPE_REMOVE_FILE:
				MigrationFileUtils.removeFile(_project, detailsArray[0]);
				break;

			case TYPE_REPLACE_STR_ALLFILES:
				MigrationStringUtils.replaceAll(MigrationUtils.getAllIDEFiles(
						_project).toArray(new String[] {}), detailsArray[0]
						.split(","), detailsArray[1].split(","));
				break;
			}

		} catch (Exception e) {
			MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
					"Error performing migration step for the project : "
							+ _project.getName() + ".", e);
		}
	}

	/**
	 * Creates the backup of the files before migrating the project so that in
	 * case the user needs them back, he can acccess them. The backup files are
	 * stored under the given directory
	 * 
	 * @param projects -
	 *            Projects to be backed up
	 * 
	 */
	public static boolean backupProjects(IProject[] projects, String backupDir) {

		boolean retVal = true;
		
		// first make sure that all of the projects can be backed up
		for (int i = 0; i < projects.length; i++)
		{
			IProject project = projects[i];

			File backup_dest = new File(backupDir + File.separator + project.getName());
			if (backup_dest.exists())
			{
				if (!backup_dest.isDirectory())
				{
					MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
							"The backup directory specified [" + backup_dest
									+ "] for project " + project.getName()
									+ " is not a directory.", null);
					retVal = false;
				} else {

					String[] files = backup_dest.list();
					if (files.length > 0)
					{
						MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
								"The backup directory specified ["
										+ backup_dest + "] for project "
										+ project.getName() + " is not empty.",
								null);
						retVal = false;
					}
				}
			} else {
				backup_dest.mkdirs();
			}
		}

		// now do the actual backup
		if (retVal) {
			for (int i = 0; i < projects.length; i++)
			{
				IProject project = projects[i];

				File backup_dest = new File(backupDir + File.separator + project.getName());
	            try {
	            	ClovisFileUtils.copyDirectory(project.getLocation().toFile(), backup_dest, false);
	            } catch (IOException e) {
	            	MigrationUtils.reportProblem(PROBLEM_SEVERITY_ERROR,
							"Error while creating the backup for the Project : "
									+ projects[i].getName() + ".", e);
					retVal = false;
				}
			}
		}
		return retVal;
	}
}
