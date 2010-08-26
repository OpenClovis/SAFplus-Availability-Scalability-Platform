/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/migration/MigrationManager.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project.migration;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.resources.ICommand;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.ui.IWorkbenchPage;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.MigrationConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * 
 * @author shubhada
 *
 * MigrationManager class which will take care of creating the 
 * new version of the resource to be migrated.
 */
public class MigrationManager implements MigrationConstants
{
    private String _oldVersion, _newVersion;
    private HashMap _projectProblemsMap = new HashMap();
    public static final String BACKUP_DIR_NAME = "Backup_Projects";
    private static final Log LOG = Log.getLog(DataPlugin.getDefault());
    
    /**
     * Constructor
     * @param oldVersion - existing version of Projects to be migrated 
     * @param newVersion - Version to which projects should be migrated
     * @param projects - Selected projects for migration
     */
    public MigrationManager(String oldVersion, String newVersion)
    {
        _oldVersion = oldVersion;
        _newVersion = newVersion;
        
    }
    /**
     * Calls the specific migration handler per resource 
     * (if any)
     * 
     * @param project - IProject
     * @param newResource - New Resource created
     * @param oldResource - old Resource to be migrated
     * @param changeInfoObj - Migration xmi object which holds migration
     * info related to a resource
     * @param ePack - EPackage of the new Ecore
     * @param retList - List of problems encountered during migration
     */
    private boolean callSpecificMigrationHandler(IProject project, Resource newResource, Resource oldResource,
            EObject changeInfoObj, EPackage ePack, List retList)
    {
        String specificMigrationMethod = (String) EcoreUtils.getValue(
                changeInfoObj, MIGRATION_SPECIFIC_METHOD);
        if (!specificMigrationMethod.equals("NONE")) {
            Class handlerClass = ClovisUtils.loadClass(specificMigrationMethod);
            try {
                Class[] argType = {
                		IProject.class,
                        Resource.class,
                        Resource.class,
                        EObject.class,
                        EPackage.class,
                        String.class,
                };
                if (handlerClass != null) {
                    Method met  = handlerClass.getMethod("migrateResource", argType);
                    Object[] args = {project, newResource, oldResource, changeInfoObj, ePack, _oldVersion};
                    met.invoke(null, args);
                    saveResource(newResource);
                    return true;
                }
            } catch (NoSuchMethodException e) {
                LOG.error("Class does not have migrateResource() method", e);
            } catch (Throwable th) {
                LOG.error("Unhandled error while calling migration handler:"
                        + specificMigrationMethod, th);
            }
        }
        return false;
        
    }
    /**
     * Calls the specific migration handler per resource 
     * (if any)
     * 
     * @param project - IProject
     * 
     */
    private boolean callMigrationHandler(IProject project, String methodName, List retList)
    {
        
        
        Class handlerClass = ClovisUtils.loadClass(methodName);
        try {
            Class[] argType = {
            		IProject.class,
            };
            if (handlerClass != null) {
                Method met  = handlerClass.getMethod("migrate", argType);
                Object[] args = {project};
                met.invoke(null, args);
                
                return true;
            }
        } catch (NoSuchMethodException e) {
            LOG.error("Class does not have migrate() method", e);
        } catch (Throwable th) {
            LOG.error("Unhandled error while calling migration handler:"
                    + methodName, th);
        }
        
        return false;
        
    }
    /**
     * @param resource - Resource to be saved
     */
    public static void saveResource(Resource resource)
    {
        try {
            EcoreModels.save(resource);
        } catch (IOException e) {
           LOG.error("Could not save the migrated resource", e);
        }

    }
    /**
     * Reads the old, new ecore and resource files and creates
     * the corresponding new eObjects and copies the data 
     * to them. 
     * 
     * @param selProjs - projects selected for migration
     * @param isCmd - indicate whether running through
     * commandline or through IDE
     */
    public void readFilesAndUpdate(IProject [] selProjs, boolean isCmd)
    {
        // get the migration info map
    	MigrationInfoReader reader = new MigrationInfoReader(
                _oldVersion, _newVersion);
        HashMap versionMigrationListMap = reader.getMigrationInfo();
        HashMap versionHandlerListMap = reader.getHandlerMap();
        if (!isCmd) {
        for (int i = 0; i < selProjs.length; i++) {
        	ProjectDataModel pdm = ProjectDataModel.
        		getProjectDataModel(selProjs[i]);
        	GenericEditorInput caInput = (GenericEditorInput) pdm.
        		getCAEditorInput();
        	GenericEditorInput compInput = (GenericEditorInput) pdm.
    			getComponentEditorInput();
        	IWorkbenchPage page = WorkspacePlugin
			            .getDefault()
			            .getWorkbench()
			            .getActiveWorkbenchWindow()
			            .getActivePage();
        	if (page != null) {
        		if (caInput != null) {
                    page.closeEditor(caInput.getEditor(), false);
                } 
        		if (compInput != null) {
                    page.closeEditor(compInput.getEditor(), false);
                } 
        	}
        	pdm.removeDependencyListeners();
        	ProjectDataModel.removeProjectDataModel(selProjs[i]);
        }
        }
        Iterator iterator = reader.getVersionOrderList().iterator();
        while (iterator.hasNext()) {
        	String version = (String) iterator.next();
        	List migrationList = (List) versionMigrationListMap.get(version);
        	List handlerList  = (List) versionHandlerListMap.get(version);
        	List pendingHandlers = new Vector();
        	List problemsList = new ArrayList();
        	Iterator handlerIterator = handlerList.iterator();
        	// Call the handlers which work not per resource
        	while (handlerIterator.hasNext()) {
        		EObject handlerObj = (EObject) handlerIterator.next();
        		boolean callAfterResourceMigration = ((Boolean) EcoreUtils.getValue(
        				handlerObj, "callAfterResourceMigration")).booleanValue();
        		String handler = EcoreUtils.getValue(
        				handlerObj, MigrationConstants.
        				MIGRATION_HANDLER_NAME).toString();
        		if (!callAfterResourceMigration) {
        			for (int i = 0; i < selProjs.length; i++) {
        				callMigrationHandler(selProjs[i], handler, problemsList);
        			}
        		} else {
        			pendingHandlers.add(handler);
        		}
        	}
        	// for each of the projects, execute migrate action for 
            //relevent resources
            for (int i = 0; i < selProjs.length; i++) {
            	for (int j = 0; j < migrationList.size(); j++) {
		            EObject changeInfoObj = (EObject) migrationList.get(j);
		            String oldResourcePath = (String) EcoreUtils.getValue(
		                    changeInfoObj, MIGRATION_OLD_RESOURCEPATH);
		            String oldEcorePath = (String) EcoreUtils.getValue(
		                    changeInfoObj, MIGRATION_OLD_ECOREPATH);
		            //read the old ecore file
		            readEcoreFile(oldEcorePath, false);
            
            
            
	                List probsList = new ArrayList();
	                Resource oldResource = getResource(oldResourcePath, selProjs[i], false);
		            if (oldResource != null) {
		                String newResourcePath = (String) EcoreUtils.getValue(
		                        changeInfoObj, MIGRATION_NEW_RESOURCEPATH);
		                String newEcorePath = (String) EcoreUtils.getValue(
		                        changeInfoObj, MIGRATION_NEW_ECOREPATH);
		                
		                EPackage ePack = readEcoreFile(newEcorePath, true);
		                Resource newResource = getResource(newResourcePath, selProjs[i], true);
		                if (!newResourcePath.equals(oldResourcePath)) {
		                    deleteResource(oldResourcePath, selProjs[i]);
		                }
				                copyToNewResource(selProjs[i], newResource, oldResource,
		                        changeInfoObj, ePack, probsList);
		                for (int k = 0; k < probsList.size(); k++) {
		                    EObject problem = (EObject) probsList.get(k);
		                    EcoreUtils.setValue(problem, "resource", newResource.
		                            getURI().path());
		                }
		                List problems = (List) _projectProblemsMap.get(selProjs[i]);
		                if (problems == null) {
		                    _projectProblemsMap.put(selProjs[i], probsList);
		                } else {
		                    problems.addAll(probsList);
		                }
		            }
            	}
            	 try {
     	       	 	//Update the project version
     	   		
     	        	final IProjectDescription description = selProjs[i].
     	   			getDescription();
     	           if (_newVersion != null) {
     	               description.setComment("Project Version:" + _newVersion);
     	            }

     	            // add new builder to project
     	            ICommand command = description.newCommand();
     	            command.setBuilderName(
     	                "com.clovis.cw.workspace.builders.ClovisBuilder");
     	            
     	            command.setBuilding(IncrementalProjectBuilder.AUTO_BUILD, false);            
     	            ICommand[] newCommands = new ICommand[1];
     	            newCommands[0] = command;
     	            description.setBuildSpec(newCommands);
     	            
     				selProjs[i].setDescription(description, null);
     				
     			 } catch (CoreException e) {
     				System.out.println(e.getMessage());
     			 }
        	}
        	handlerIterator = pendingHandlers.iterator();
        	// Call the pending handlers which have to be called after migration of resources
        	while (handlerIterator.hasNext()) {
        		String handler = (String) handlerIterator.next();
    			for (int i = 0; i < selProjs.length; i++) {
    				callMigrationHandler(selProjs[i], handler, problemsList);
    			}
        
        	}
        }
        
        
    }
    /**
     * 
     * @param project - IProject
     * @param newResource - New Resource to which objects should be copied
     * @param oldResource - Old Resource from which objects to be copied
     * @param changeInfoObj- Migration info object
     * @param ePack - EPackage of the new ecore file
     * @param retList - List of problems encountered during migration
     */
    private void copyToNewResource(IProject project, Resource newResource, Resource oldResource,
            EObject changeInfoObj, EPackage ePack, List retList)
    {
        EObject ePackInfoObj = (EObject) EcoreUtils.getValue(
                changeInfoObj, MIGRATION_PACKAGE_NAME);
        List topEClassObjects = (List) EcoreUtils.getValue(ePackInfoObj, "eClass");
        List newContents = newResource.getContents();
        EClass eClass = null; 
        List oldEClassList = new Vector();
        HashMap destSrcObjMap = new HashMap();
        List oldContents = oldResource.getContents();
//      Call specific migration handlers
        boolean hasHandler = callSpecificMigrationHandler(project, newResource, oldResource,
            changeInfoObj, ePack, retList);
        if (!hasHandler) {
	        for (int k = 0; k < topEClassObjects.size(); k++) {
	            
	            EObject topEClassObj = (EObject) topEClassObjects.get(k);
	            String newEClass = (String) EcoreUtils.getValue(
	                    topEClassObj, MIGRATION_NEW_FIELDNAME);
	            String oldEClass = (String) EcoreUtils.getValue(
	                    topEClassObj, MIGRATION_OLD_FIELDNAME);
	            List oldObjList = getResourceObj(oldEClass, oldContents);
	            if (!oldEClass.equals("NULL"))
	                oldEClassList.add(oldEClass);
	            
	            // If the eClass is not deleted then only create the new Object
	            if (!newEClass.equals("NULL")) {
	                eClass = (EClass) ePack.getEClassifier(newEClass);
	                //It is the case where a fresh new eClass is added to ecore
	                if (oldEClass.equals("NULL")) {
	                    EObject eObj = EcoreUtils.createEObject(eClass, true);
	                    newContents.add(eObj);
	                    destSrcObjMap.put(eObj, null);
	                } else {
	                    // add same number of new objects as in the old resource to the new resource.
	                    
	                    addToNewResource(oldObjList, newContents, eClass, destSrcObjMap);
	                }
	            }
	          
	        }
	        //Now also add those objects which didnt change and info
	        // is not maintained in migration xmi file
	        
	        for (int k = 0; k < oldContents.size(); k++) {
	            EObject oldObj = (EObject) oldContents.get(k);
	            
	            if (!oldEClassList.contains(oldObj.eClass().getName())) {
	                eClass = (EClass) ePack.getEClassifier(oldObj.eClass().getName());
	                
	                EObject eObj = EcoreUtils.createEObject(eClass, true);
	                destSrcObjMap.put(eObj, oldObj);
	                newContents.add(eObj);
	            }
	        }
	        //Now copy the data of top level object from old to new resource recursively
	        //till the last level
	        ResourceCopyHandler copyHandler = new ResourceCopyHandler(changeInfoObj);
	        copyHandler.copyResourceInfo(destSrcObjMap, retList);
	        saveResource(newResource);
	        // Where are old objects which are deleted from new resource
	        // !! We have to maintain them because at later point they might be required in copy
        }
        
    }
    /**
     * Adds the same number of objects to new resource 
     * as in the old resource
     * to new resource 
     * @param oldObjList - Old object list
     * @param newContents - New List
     * @param eClass - eClass of new objects
     * @param destSrcMap - The map with source and destination objects
     */
    private void addToNewResource(List oldObjList, List newContents,
            EClass eClass, HashMap destSrcMap)
    {
        for (int i = 0; i < oldObjList.size(); i++) {
            EObject oldObj = (EObject) oldObjList.get(i);
            EObject newObj = EcoreUtils.createEObject(eClass, true);
            newContents.add(newObj);
            destSrcMap.put(newObj, oldObj);
        }
        
    }
    /**
     * 
     * @param eClass - ECla
     * @param resObjects - Resource content objects
     * @return the List of objects of given eClass
     */
    private  List getResourceObj(String eClass, List resObjects)
    {
        List objList = new Vector();
        for (int i = 0; i < resObjects.size(); i++) {
            EObject oldObj = (EObject) resObjects.get(i);
            if (oldObj.eClass().getName().equals(eClass)) {
                objList.add(oldObj);
            }
        }
        return objList;
    }
    /**
     * Deletes the resource from file system
     *  @param resourcePath - Relative resource path
     * @param project - The project in which resource exists
     */
    private void deleteResource(String resourcePath, IProject project)
    {
        try {
            resourcePath = resourcePath.replace('/', File.separatorChar);

            //Now get the xmi file from the project 
            String dataFilePath = project.getLocation().toOSString()
                                  + File.separator + resourcePath;
           
            File xmiFile = new File(dataFilePath);
            xmiFile.delete();
            
           
        } catch (Exception exception) {
            LOG.warn("Failed to delete resource", exception);
        }
    }
    /**
     * 
     * @param resourcePath - Resource path relative to project location
     * @param project - Project for which resource has to be read.
     * @param createNew - If this flag is true, creates the New Resource
     * and returns else returns resource only if it exists already
     * @return the resource
     */
    private Resource getResource(String resourcePath, IProject project, boolean createNew)
    {
        try {
            resourcePath = resourcePath.replace('/', File.separatorChar);

            //Now get the xmi file from the project 
            String dataFilePath = project.getLocation().toOSString()
                                  + File.separator + resourcePath;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);
            if (createNew) {
                return EcoreModels.create(uri);
            } else {
                if (xmiFile.exists()) { 
                    Resource resource = EcoreModels.getUpdatedResource(uri);
                    return resource;
                }
            }
            
           
        } catch (Exception exception) {
            LOG.warn("Failed while loading resource", exception);
        }
        return null;
    }

    /**
     * Reads the ecore file
     * @param ecorePath - Relative path of ecore from the DataPlugin
     * @param - type - if true, new ecore is read, else old ecore is read
     */
    private EPackage readEcoreFile(String ecorePath, boolean type)
    {
        ecorePath = ecorePath.replace('/', File.separatorChar);
        //if (type) {
            URL url = FileLocator.find(DataPlugin.getDefault().getBundle(), new Path(ecorePath), null);
            try {
                File ecoreFile = new Path(Platform.resolve(url).getPath())
                        .toFile();
                EPackage ePackage = EcoreModels.getUpdated(ecoreFile.getAbsolutePath());
                return ePackage;
            } catch (IOException e) {
                LOG.warn("Cannot read changed ecore file", e);
            }
            
        /*} else {
            String ecoreLocation = _installLocation + ECLIPSE_PLUGIN_FOLDER_NAME +
                File.separator + CLOVIS_PLUGINS_PREFIX + _oldVersion + File.separator
                + ecorePath;
            
            File ecoreFile = new Path(ecoreLocation).toFile();
            try {
                EPackage ePackage = EcoreModels.getUpdated(ecoreFile.getPath());
                return ePackage;
            } catch (IOException e) {
                LOG.warn("Old ecore file cannot be read", e);
            }
        }*/
        return null;
    }
    
    /**
     * Writes the Migration Problems to the log file
     * @param selProjs - Selected projects which have to be migrated
     * @param logfile - FileWriter
     * @param manager - Migration Manager
     */
    public static void writeProblemsToLog(IProject [] selProjs,
            FileWriter logfile, MigrationManager manager)
    {
        for (int i = 0; i < selProjs.length; i++) {
            try {
                IProject selProj = selProjs[i];
                List problemsList = manager.getProblems(selProj);
                if (problemsList != null) {
	                logfile.write("---------------------------------------------------------------------------------------\n");
	                if (problemsList.size() > 0) {
	                    logfile.write(problemsList.size()
	                        + " problems encountered in project ["
	                        + selProj.getName()
	                        + "]. Migration could not be completed successfully");
	                } else {
	                    logfile.write("Migration successful on project ["
	                            + selProj.getName() + "]");
	                }
	                logfile.write("\n---------------------------------------------------------------------------------------");
	                for (int j = 0; j < problemsList.size(); j++) {
	                    EObject problem = (EObject) problemsList.get(j);
	                    
	                    EObject sourceObj = (EObject) EcoreUtils.getValue(
	                            problem, "source");
	                    String sourceObjName = "None";
	                    if (sourceObj != null) {
	                        sourceObjName = EcoreUtils.getName(sourceObj);
	                    }
	                    int level = ((EEnumLiteral) EcoreUtils.getValue(problem,
	                            "level")).getValue(); 
	                    String message = (String) EcoreUtils.getValue(problem, "message");
	                    if (level == 0) {
	                        logfile.write("\n\n[" + sourceObjName + "]"
	                                + " : ERROR - " + message);
	                    } else if (level == 1) {
	                        logfile.write("\n\n[" + sourceObjName + "]"
	                                + " : WARNING - " + message);
	                    } else {
	                        logfile.write("\n\n[" + sourceObjName + "]"
	                                + " : INFO - " + message);
	                    }
	                    
	                }
                }
                logfile.write("\n****************************************************************************************\n\n");
            } catch (IOException e) {
                LOG.error("Migration log cannot be written", e);
            }
           
        }
    }
    /**
     * Creates the backup of the files before migrating the project
     * so that in case the user needs them back, he can acccess them.
     * The backup files are stored under the directory "Backup_Projects"
     * in eclipse installed location
     * 
     * @param projects - Projects to be backed up
     *
     */
    public static void createBackupFiles(IProject [] projects)
    {
        IWorkspace workspace = ResourcesPlugin.getWorkspace();
        Path backupDirPath = new Path(Platform.getInstallLocation().
                getURL().getPath().concat(File.separator).concat(BACKUP_DIR_NAME));
        File backupDir = new File(backupDirPath.toOSString());
        if (!backupDir.exists()) {
            backupDir.mkdir();
            
        }
        
        for (int i = 0; i < projects.length; i++) {
            IProject project = projects[i];
            Path backupProjPath = (Path) backupDirPath.append("Copy of " + project.getName());
            File backupProjDir = new File(backupProjPath.toOSString());
            if (backupProjDir.exists()) {
                backupProjDir.delete();
        }
            backupProjDir.mkdir();
            IProject backProject = workspace.getRoot().
                getProject("Copy of " + project.getName());
            
            try {
                
                IProjectDescription backupProjDesc = workspace
                    .newProjectDescription(backProject.getName());
                IProjectDescription projDesc = project.getDescription();
                backupProjDesc.setLocation(backupProjPath);
                backupProjDesc.setBuildSpec(projDesc.getBuildSpec());
                backupProjDesc.setComment(projDesc.getComment());
                backupProjDesc.setNatureIds(projDesc.getNatureIds());
                backupProjDesc.setReferencedProjects(projDesc.getReferencedProjects());
                backupProjDesc.setDynamicReferences(projDesc.getDynamicReferences());
                project.copy(backupProjDesc, true, null);
                
            } catch (CoreException e) {
                // don't do anything
            }
        }
    }
    /**
     * 
     * @param srcVersion - Existing project version
     * @param destVersion - The new version of the project
     * @return true if destVersion is the newer version than srcVersion
     *          else return false; 
     */
    public static boolean isVersionValid(String srcVersion, String destVersion)
    {
        StringTokenizer srcTokenizer = new StringTokenizer(srcVersion, ".");
        StringTokenizer destTokenizer = new StringTokenizer(destVersion, ".");
        
    	while (srcTokenizer.hasMoreTokens()
    			&& destTokenizer.hasMoreTokens()) {
            int srcVal = Integer.parseInt(srcTokenizer.nextToken());
            int destVal = Integer.parseInt(destTokenizer.nextToken());
            if (srcVal < destVal) {
                return true;
            } else if (srcVal > destVal) {
                return false;
            } else {
                continue;
                    
            }
        }
    	if(destTokenizer.countTokens() > 0) {
    		return true;
    	}
        return false;
    }
    /**
     * 
     * @param proj - Project for which migration problems to be fetched
     * @return the List of migration problems encounted for the project 
     */
    public List getProblems(IProject proj)
    {
        return (List) _projectProblemsMap.get(proj);
    }
    
}
