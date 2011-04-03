/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/ProjectDataModel.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.ui.IEditorInput;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.clovis.common.utils.ClovisDomUtils;
import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.ClovisMessageHandler;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.project.utils.FormatConversionUtils;

/**
 * @author ashish
 *
 * This ProjectDataModel is the model for every Clovis Project.
 * This model will have information about the various editors it
 * supports and the data files that would be needed to populate
 * the various editors in this project.
 */
public class ProjectDataModel
{
    private IContainer             _project;

    private static final Log   LOG     = Log.getLog(DataPlugin.getDefault());

    private static final Hashtable PROJECT_VS_MODEL_HASH = new Hashtable();

    private Model                  _caModel;

    private Model                  _componentModel;

    private Model                  _alarmProfiles;
    
    private Model                  _componentResourceMapModel;
    
    private Model                  _resourceAlarmMapModel;

    private Model _alarmAssociationModel;
    
    private Model _resourceAssociationModel;

    private Model _nodeProfiles;
    
    private Model _eoDefinitions;

    private Model _eoConfiguration;
    
    private IEditorInput    _caEditorInput;

    private IEditorInput    _compEditorInput;
    
    private IEditorInput    _manageabilityEditorInput;
    
    private DependencyListener _dependencyListener = new DependencyListener(this, DependencyListener.MODEL_OBJECT);
    
    private ModelTrackListener _trackListener;
    
    //Template Group
    //private Model _compTemplateModel;
	//private TemplateGroupListener _templateGroupListener;
	    
    //private Model _trackList;
    
    private AlarmMappingModelTrackListener _alarmMapModelTrackListener;
    
    private ResourceMappingModelTrackListener _resourceMapModelTrackListener;
    
    private TrackingModel _trackingModel;
    
    //  Template Group
    private Model _compTemplateModel;
	private TemplateGroupListener _templateGroupListener;
    
	private TemplateMappingModelTrackListener _templateModelTrackListener;
	
	private AMFModelTrackListener _amfModelTrackListener;
	
	//Alarm Rule 
	private	Model	_alarmRules;
	
	private ArrayList<String>    _loadedMibs = new ArrayList<String>();
	
	//PM Config Model
	private Model _pmConfigModel;
	
	private boolean _modifiedAfterValidation = true;
	
	private List _modelProblems;
	
    /**
     * Private Constructor. Use getProjectDataModel(IContainer) to get the
     * instance.
     *
     * @param project
     *            Project.
     */
    private ProjectDataModel(IContainer project)
    {
        _project = project;
        PROJECT_VS_MODEL_HASH.put(_project, this);
        createModelTrackList();
        copyConfigFilesToTempDir((IProject) _project);
        //init();
    }
    /**
     * Returns List which contains model changes.
     * @return NotifyingList
     */
    public TrackingModel getTrackingModel() {
		return _trackingModel;
	}
    /**
     * Returns IContainer
     * @return Project
     */
    public IProject getProject()
    {
    	return (IProject) _project;
    }
    /**
     * returns IOC Config List
     * @return IOC Config List
     */    
    public List getIOCConfigList()
    {
    	 List iocList = null;
		try {
			// reading the ecore here is written here mainly because
			// without ecore being in memory, resource cannot be read
			URL url = DataPlugin.getDefault()
			 .find(new Path("model" + File.separator + ICWProject
			                 .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME
			                 + File.separator + ICWProject
			                 .CW_SYSTEM_COMPONENTS_FOLDER_NAME
			                 + File.separator + ICWProject.IOCBOOT_ECORE_FILENAME));
			File ecoreFile = new Path(Platform.resolve(url).getPath())
			 	.toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());
    	    EcoreModels.getUpdated(ecoreFile.getAbsolutePath());
			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + ICWProject.IOCBOOT_XML_DATA_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			Resource iocResource = xmlFile.exists() ? EcoreModels.getUpdatedResource(uri)
					: EcoreModels.create(uri);
			if (iocResource.getContents().isEmpty()) {
				EClass bootConfigClass = (EClass) pack.getEClassifier("BootConfig");
				EObject bootConfigObj = EcoreUtils.createEObject(bootConfigClass, true);
				iocResource.getContents().add(bootConfigObj);
			}
			iocList = iocResource.getContents();
		} catch (IOException e) {
			//e.printStackTrace();
		}
		return iocList;
    }
    /**
     * returns IDL Config List
     * @return IDL Config List
     */    
    public List getIDLConfigList()
    {
         List idlList = null;
         try {
             URL idlURL = DataPlugin.getDefault().find(new Path("model"
                     + File.separator + ICWProject.IDL_ECORE_FILENAME));
             File ecoreFile = new Path(Platform.resolve(idlURL).getPath())
                     .toFile();
             EcoreModels.get(ecoreFile.getAbsolutePath());
             String dataFilePath = _project.getLocation().toOSString()
             + File.separator + ICWProject.CW_PROJECT_IDL_DIR_NAME
             + File.separator + ICWProject.IDL_XML_DATA_FILENAME;
             URI uri = URI.createFileURI(dataFilePath);
             File xmiFile = new File(dataFilePath);

             Resource idlResource = xmiFile.exists()
                 ? EcoreModels.getUpdatedResource(uri) : EcoreModels.create(uri);
             idlList = idlResource.getContents();
             return idlList;
         } catch (IOException ex) {
             LOG.error("IDL File cannot be read", ex);
         }
         return idlList;
    }
    /**
     * Reads compile time configuration for ASP.
     * @return Top EObject from compileconfigs.xml
     * @throws IOException  In case of file IO issues.
     */
    public List getBuildTimeComponentList()
    {
        Resource resource = null;
        // This needs to be cleaned
        try {
        URL url = DataPlugin.getDefault().find(new Path("model" + File
                .separator + ICWProject
                .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME + File.separator
                + ICWProject.CW_COMPILE_TIME_COMPONENTS_FOLDER_NAME
                + File.separator + "Comps.ecore"));

        File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
        EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

        String dataFilePath = _project.getLocation().toOSString()
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
        return resource.getContents();
        } catch (IOException e) {
            LOG.error("Error while reading build time components data", e);
        }
        return null;
    }
    /**
     * Get Model for Resource Editor.
     *
     * @return EList for Resource
     */
    public Model getResourceModel()
    {
        return getCAModel();
    }
    /**
     * Get Model for Resource Editor.
     *
     * @return EList for Resource
     */
    public Model getCAModel()
    {
        if (_caModel == null) {
            createCAModel();
        }
        return _caModel;
    }
    /**
     * Get Model for Component Editor.
     *
     * @return EList for Component
     */
    public Model getComponentModel()
    {
        if (_componentModel == null) {
        	if(_eoConfiguration == null)
        		createEOConfiguration();
            createComponentModel();
        }
        return _componentModel;
    }
    /**
     * Returns Alarm Profiles
     *
     * @return Notifying List
     */
    public Model getAlarmProfiles()
    {
        if (_alarmProfiles == null) {
            createAlarmProfiles();
        }
        return _alarmProfiles;
    }
    
    
	/**
     * Returns Alarm Rules
     *
     * @return Notifying List
     */
    public Model getAlarmRules()
    {
        if (_alarmRules == null) {
            createAlarmRules();
        }
        return _alarmRules;
    }

    /**
	 * Returns Memory Configuration
	 * 
	 * @return the Memory Configuration
	 */
	public Model getEODefinitions() {
        if (_eoDefinitions == null) {
            createEODefinitions();
        }
		return _eoDefinitions;
	}

	/**
	 * Returns EO Configuration
	 * 
	 * @return the EO Configuration
	 */
	public Model getEOConfiguration() {
        if (_eoConfiguration == null) {
            createEOConfiguration();
        }
		return _eoConfiguration;
	}

	/**
	 * Returns Node Profiles
	 * 
	 * @return the Node profiles List
	 */
	public Model getNodeProfiles() {
        if (_nodeProfiles == null) {
            createNodeProfiles();
        }
		return _nodeProfiles;
	}

	
    /**
     * Returns Component-Resource map model
     *
     * @return Notifying List
     */
    public Model getComponentResourceMapModel()
    {
        if (_componentResourceMapModel == null) {
            createComponentResourceMap();
        }
        return _componentResourceMapModel;
    }
	/**
     * Returns Resource-Alarm map model
     * 
     * @return Notifying List
     */
    public Model getResourceAlarmMapModel()
    {
        if (_resourceAlarmMapModel == null) {
            createResourceAlarmMap();
        }
        return _resourceAlarmMapModel;
    }

    /**
	 * Returns Alarm Association model.
	 * 
	 * @return
	 */
	public Model getAlarmAssociationModel() {
		if (_alarmAssociationModel == null) {
			createAlarmAssociation();
		}
		return _alarmAssociationModel;
	}
	/**
	 * Returns Resource Association model.
	 * 
	 * @return
	 */
	public Model getResourceAssociationModel() {
		if (_resourceAssociationModel == null) {
			createResourceAssociation();
		}
		return _resourceAssociationModel;
	}
	/** Returns PM Config Model
	 * 
	 * @return
	 */
	public Model getPMConfigModel() {
		if(_pmConfigModel == null)
			createPMConfigModel();
		return _pmConfigModel;
	}
	
    /**
	 * 
	 * @return all the Node Instance Objects defined in the Node Profile
	 */
    public static List getNodeInstListFrmNodeProfile(List nodeProfileObjects)
    {
        List nodeObjs = new Vector();
        EObject amfObj = (EObject) nodeProfileObjects.get(0);
        EReference nodeInstsRef = (EReference) amfObj.eClass()
            .getEStructuralFeature("nodeInstances");
        EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
        if (nodeInstsObj != null) {
            EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
                .getEStructuralFeature("nodeInstance");
            nodeObjs = (List) nodeInstsObj.eGet(nodeInstRef);
        }
        return nodeObjs;
    }
    /**
     * 
     * @return all the SG Instance Objects defined in the Node Profile
     */
    public static List getSGInstListFrmNodeProfile(List nodeProfileObjects)
    {
        List sgObjs = new Vector();
        EObject amfObj = (EObject) nodeProfileObjects.get(0);
        EReference sgInstsRef = (EReference) amfObj.eClass()
            .getEStructuralFeature("serviceGroups");
        EObject sgInstsObj = (EObject) amfObj.eGet(sgInstsRef);
        if (sgInstsObj != null) {
            EReference sgInstRef = (EReference) sgInstsObj.eClass().
            getEStructuralFeature("serviceGroup");
            sgObjs = (List) sgInstsObj.eGet(sgInstRef);
        }
        return sgObjs;
    }
    
	/**
     * Loads CA Model.
     */
    public void createCAModel()
    {
        if (_caModel != null) {
            EcoreUtils.removeListener(_caModel.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_caModel.getEList(), _trackListener, -1);
        }
        URL caURL = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.RESOURCE_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(caURL).getPath())
                    .toFile();
            EPackage pack = EcoreModels.getUpdated(ecoreFile.getAbsolutePath());

            //Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + ICWProject.RESOURCE_XML_DATA_FILENAME;

            IPath backupFilePath = new Path(
                    ICWProject.CW_PROJECT_MODEL_DIR_NAME)
                    .append(ICWProject.BACKUP_RESOURCE_XML_DATA_FILENAME);

            _project.refreshLocal(IResource.DEPTH_INFINITE, null);

            if (_project.exists(backupFilePath)) {
                IFolder modelFolder = _project.getFolder(new Path(
                        ICWProject.CW_PROJECT_MODEL_DIR_NAME));
                if (ClovisMessageHandler.displayPopupQuestion(null, "Backup file Exists",
                        "Backup model exists, do you want to use it?")) {
                    IPath bpath = new Path(
                            ICWProject.BACKUP_RESOURCE_XML_DATA_FILENAME);
                    IPath opath = new Path(
                            ICWProject.RESOURCE_XML_DATA_FILENAME);
                    if (modelFolder.exists(opath)) {
                        modelFolder.getFile(opath).delete(true, false, null);
                    }
                    modelFolder.getFile(bpath).move(opath, true, null);
                } else {
                    _project.getFile(backupFilePath).delete(true, false, null);
                }
            }
            File xmiFile = new File(dataFilePath);
            URI uri = URI.createFileURI(dataFilePath);
            Resource resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
            NotifyingList list = (NotifyingList) resource.getContents();
            ClovisUtils.initializeDependency(list);
            _caModel = new Model(resource, list, pack);
            if (!_caModel.getEList().isEmpty()) {
            	FormatConversionUtils.convertToEditorSupportedData((EObject) _caModel.getEList().get(0),
            			(EObject) _caModel.getEList().get(0), "Resource Editor");
            }
            EcoreUtils.addListener(_caModel.getEList(), _dependencyListener, -1);
            EcoreUtils.addListener(_caModel.getEList(), _trackListener, -1);
        } catch (Exception exception) {
            LOG.error("Error while Loading Resource Editor Model", exception);
        }
    }
    /**
     * Creates Component Model
     *
     */
    public void createComponentModel()
    {
    	if (_componentModel != null) {
            EcoreUtils.removeListener(_componentModel.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_componentModel.getEList(), _trackListener, -1);
            EcoreUtils.removeListener(_componentModel.getEList(), _templateGroupListener, -1);
        }

        URL url = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.COMPONENT_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
            EPackage pack = EcoreModels.getUpdated(ecoreFile.getAbsolutePath());

            //Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + ICWProject.COMPONENT_XMI_DATA_FILENAME;

            IPath backupFilePath = new Path(
                    ICWProject.CW_PROJECT_MODEL_DIR_NAME)
                    .append(ICWProject.BACKUP_COMPONENT_XMI_DATA_FILENAME);

            _project.refreshLocal(IResource.DEPTH_INFINITE, null);

            if (_project.exists(backupFilePath)) {
                IFolder modelFolder = _project.getFolder(new Path(
                        ICWProject.CW_PROJECT_MODEL_DIR_NAME));
                if (ClovisMessageHandler.displayPopupQuestion(null, "Backup file Exists",
                        "Backup model exists, do you want to use it?")) {
                    IPath bpath = new Path(
                            ICWProject.BACKUP_COMPONENT_XMI_DATA_FILENAME);
                    IPath opath = new Path(
                            ICWProject.COMPONENT_XMI_DATA_FILENAME);
                    if (modelFolder.exists(opath)) {
                        modelFolder.getFile(opath).delete(true, false, null);
                    }
                    modelFolder.getFile(bpath).move(opath, true, null);
                } else {
                    _project.getFile(backupFilePath).delete(true, false, null);
                }
            }
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            Resource resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
            NotifyingList list = (NotifyingList) resource.getContents();
            _componentModel = new Model(resource, list, pack);
            /*EcoreUtils.addListener((NotifyingList) list,
                    new NodeChangeListener((IProject) _project), 2);*/
            if (!_componentModel.getEList().isEmpty()) {
            	FormatConversionUtils.convertToEditorSupportedData((EObject) _componentModel.getEList().get(0),
            			(EObject) _componentModel.getEList().get(0), "Component Editor");
            }
            EcoreUtils.addListener(_componentModel.getEList(), _dependencyListener, -1);
            EcoreUtils.addListener(_componentModel.getEList(), _trackListener, -1);
            if(_templateGroupListener == null)
            	readTemplateGroupModel();
            EcoreUtils.addListener(_componentModel.getEList(), _templateGroupListener, -1);
        } catch (Exception exception) {
            LOG.error("Error while Loading Component Editor Model", exception);
        }
    }
    /**
     * Loads Alarm Profiles
     *
     */
    private void createAlarmProfiles()
    {
        URL url = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.ALARM_PROFILES_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
            EPackage pack = EcoreModels.getUpdated(ecoreFile.getAbsolutePath());

            //Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + ICWProject.ALARM_PROFILES_XMI_DATA_FILENAME;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            Resource resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
             EClass alarmInfoClass = (EClass) pack.getEClassifier("alarmInformation");
             if (resource.getContents().isEmpty()) {
            	 EObject alarmInfoObj = EcoreUtils.createEObject(alarmInfoClass, true);
            	 resource.getContents().add(alarmInfoObj);
             }
            _alarmProfiles = new Model(resource, (NotifyingList) resource
                    .getContents(), pack);
            EcoreUtils.addListener(_alarmProfiles.getEList(), _dependencyListener, -1);
            EcoreUtils.addListener(_alarmProfiles.getEList(), _trackListener, -1);
        } catch (Exception exception) {
            LOG.error("Error while Loading Alarm Profiles", exception);
        }
    }
    
    /**
     * Loads Alarm Rules (Generation and suppression rules)
     *
     */
    private void createAlarmRules()
    {
    	
        URL url = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.ALARM_RULES_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
            EPackage pack = EcoreModels.getUpdated(ecoreFile.getAbsolutePath());

            //Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + ICWProject.ALARM_RULES_XML_DATA_FILENAME;
            
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            Resource resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
                    
             EClass alarmRuleInfoClass = (EClass) pack.getEClassifier("alarmRuleInformation");
             if (resource.getContents().isEmpty()) {
            	 EObject alarmInfoObj = EcoreUtils.createEObject(alarmRuleInfoClass, true);
            	 resource.getContents().add(alarmInfoObj);
             }
            _alarmRules = new Model(resource, (NotifyingList) resource
                    .getContents(), pack);
                        
        } catch (Exception exception) {
            LOG.error("Error while Loading Alarm Rules", exception);
        }
    }
   
    
    /**
     * Loads memory configuration.
     */
    public void createEODefinitions() {
        if (_eoDefinitions != null) {
            EcoreUtils.removeListener(_eoDefinitions.getEList(), _dependencyListener, -1);
        }
        URL url = DataPlugin.getDefault().find(
				new Path("model" + File.separator
						+ ICWProject.MEMORYCONFIG_ECORE_FILENAME));

		try {
			File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + ICWProject.MEMORYCONFIG_XML_DATA_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmiFile = new File(dataFilePath);

			Resource resource = xmiFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);
			if (resource.getContents().isEmpty()) {
				EClass memoryConfigurationClass = (EClass) pack.getEClassifier("MemoryConfiguration");
				EObject memoryConfigurationObj = EcoreUtils.createEObject(memoryConfigurationClass, true);
				resource.getContents().add(memoryConfigurationObj);
			}
            NotifyingList list = (NotifyingList) resource.getContents();
            ClovisUtils.initializeDependency(list);
			_eoDefinitions = new Model(resource, (NotifyingList) resource
					.getContents(), pack);
			EcoreUtils.addListener(_eoDefinitions.getEList(),
					_dependencyListener, -1);
		} catch (Exception exception) {
			LOG.error("Error while Loading Memory Configuration", exception);
		}
    }

    /**
     * Loads eo configuration.
     */
    public void createEOConfiguration() {
        if (_eoConfiguration != null) {
            EcoreUtils.removeListener(_eoConfiguration.getEList(), _dependencyListener, -1);
        }
        URL url = DataPlugin.getDefault().find(
				new Path("model" + File.separator
						+ ICWProject.EOCONFIG_ECORE_FILENAME));

		try {
			File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + ICWProject.EOCONFIG_XML_DATA_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmiFile = new File(dataFilePath);

			Resource resource = xmiFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);
			if (resource.getContents().isEmpty()) {
				EClass eoListClass = (EClass) pack.getEClassifier("EOList");
				EObject eoListObj = EcoreUtils.createEObject(eoListClass, true);
				resource.getContents().add(eoListObj);
			}
			_eoConfiguration = new Model(resource, (NotifyingList) resource
					.getContents(), pack);
			EcoreUtils.addListener(_eoConfiguration.getEList(),
					_dependencyListener, -1);
		} catch (Exception exception) {
			LOG.error("Error while Loading EO Configuration", exception);
		}
    }

    /**
     * Loads Node Profiles.
     */
    private void createNodeProfiles() {
        URL url = DataPlugin.getDefault().find(
				new Path("model" + File.separator
						+ ICWProject.CPM_ECORE_FILENAME));

		try {
			File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + ICWProject.CPM_XML_DATA_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmiFile = new File(dataFilePath);

			Resource resource = xmiFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);
			if (resource.getContents().isEmpty()) {
				EClass amfClass = (EClass) pack.getEClassifier("amfConfig");
				EObject amfObj = EcoreUtils.createEObject(amfClass, true);
				resource.getContents().add(amfObj);
			}
			_nodeProfiles = new Model(resource, (NotifyingList) resource
					.getContents(), pack);
			EcoreUtils.addListener(_nodeProfiles.getEList(),
					_dependencyListener, -1);
			_amfModelTrackListener = new AMFModelTrackListener(this);
			EcoreUtils.addListener(_nodeProfiles.getEList(), _amfModelTrackListener, -1);
		} catch (Exception exception) {
			LOG.error("Error while Loading Node Profiles", exception);
		}
    }
    /**
     * Creates the Component to Resource Map model
     *
     */
    private void createComponentResourceMap()
    {
    	SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		(IProject) _project,"component", "resource");
        EObject linkObj = reader.getLinkObject(
        		"associatedResource");
        if (linkObj == null) {
        	linkObj = reader.createLink(
        			"associatedResource");
        }
        _componentResourceMapModel = reader.getMapModel();

        EcoreUtils.addListener(_componentResourceMapModel.getEList(), _resourceMapModelTrackListener, 6);
	}
    /**
     * Creates the Resource to Alarm Map model
     *
     */
    private void createResourceAlarmMap()
    {
    	SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		(IProject) _project, "resource", "alarm");
        EObject linkObj = reader.getLinkObject(
        		"associatedAlarm");
        if (linkObj == null) {
        	linkObj = reader.createLink(
        			"associatedAlarm");
        }
        _resourceAlarmMapModel = reader.getMapModel();
        EcoreUtils.addListener(_resourceAlarmMapModel.getEList(), _alarmMapModelTrackListener, 6);
		
	}

    /**
     * Creates Alarm Association model.
     */
    private void createAlarmAssociation() {
		URL url = FileLocator.find(DataPlugin.getDefault().getBundle(),
				new Path("model" + File.separator
						+ ICWProject.ALARM_ASSOCIATION_ECORE_FILENAME), null);

		try {
			File ecoreFile = new Path(FileLocator.resolve(url).getPath())
					.toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_MODEL_DIR_NAME
					+ File.separator
					+ ICWProject.ALARM_ASSOCIATION_XML_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			Resource resource = xmlFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);
			if (resource.getContents().isEmpty()) {
				EClass associationInfoClass = (EClass) pack
						.getEClassifier("associationInfo");
				EObject associationInfoObj = EcoreUtils.createEObject(
						associationInfoClass, true);
				resource.getContents().add(associationInfoObj);
			}

			_alarmAssociationModel = new Model(resource,
					(NotifyingList) resource.getContents(), pack);
			EcoreUtils.addListener(_alarmAssociationModel.getEList(),
					_dependencyListener, -1);

		} catch (Exception exception) {
			LOG
					.error("Error while Loading Alarm Association model.",
							exception);
		}
	}
    
    /**
     * Creates PM config model
     */
    private void createPMConfigModel() {
    	URL url = FileLocator.find(DataPlugin.getDefault().getBundle(),new Path("model" + File
                .separator + ICWProject
                .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME + File.separator
                + ICWProject.CW_COMPILE_TIME_COMPONENTS_FOLDER_NAME
                + File.separator + "Comps.ecore"), null);
    	try {
			File ecoreFile = new Path(FileLocator.resolve(url).getPath())
					.toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());
			String dataFilePath = _project.getLocation().toOSString()
            + File.separator
            + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
            + File.separator + "compileconfigs.xml";

			URI uri = URI.createFileURI(dataFilePath);
			File xmiFile = new File(dataFilePath);
			Resource resource = xmiFile.exists() ? EcoreModels.getResource(uri)
                : EcoreModels.create(uri);
			List contents = resource.getContents();
			if (contents.isEmpty()) {
				EObject obj = EcoreUtils.createEObject((EClass) pack
                    .getEClassifier("ComponentsInfo"), true);
				resource.getContents().add(obj);
				EcoreModels.save(resource);
			}
			EObject pmObject = (EObject) EcoreUtils.getValue((EObject)contents.get(0), "PM");
			_pmConfigModel = new Model(resource, pmObject);
    	} catch (Exception exception) {
			LOG
			.error("Error while Loading PM config model.",
					exception);
    	}
    }
    /**
     * Creates Resource Association model.
     */
    private void createResourceAssociation() {
		URL url = FileLocator.find(DataPlugin.getDefault().getBundle(),
				new Path("model" + File.separator
						+ ICWProject.RESOURCE_ASSOCIATION_ECORE_FILENAME), null);

		try {
			File ecoreFile = new Path(FileLocator.resolve(url).getPath())
					.toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_MODEL_DIR_NAME
					+ File.separator
					+ ICWProject.RESOURCE_ASSOCIATION_XML_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			Resource resource = xmlFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);
			if (resource.getContents().isEmpty()) {
				EClass associationInfoClass = (EClass) pack
						.getEClassifier("associationInfo");
				EObject associationInfoObj = EcoreUtils.createEObject(
						associationInfoClass, true);
				resource.getContents().add(associationInfoObj);
			}

			_resourceAssociationModel = new Model(resource,
					(NotifyingList) resource.getContents(), pack);
		} catch (Exception exception) {
			LOG
					.error("Error while Loading Resource Association model.",
							exception);
		}
	}    
    /**
	 * Creates and Return List with model changes
	 * 
	 * @param pdm
	 *            ProjectDataModel
	 * @return EList
	 */
	private void createModelTrackList()
    {
        URL url = DataPlugin.getDefault().find(
                new Path("model" + File.separator + "modelchanges.ecore"));
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
            EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());
            //Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + "modelchanges.xmi";
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            Resource resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
            Model model = new Model(resource, (NotifyingList) resource
                    .getContents(), pack);
            _trackingModel = new TrackingModel(model);
            _trackListener = new ModelTrackListener(_trackingModel, this);
            _alarmMapModelTrackListener = new AlarmMappingModelTrackListener(this);
            _resourceMapModelTrackListener = new ResourceMappingModelTrackListener(this);
        } catch (Exception exception) {
        	LOG.error("Error while Loading Model Changes file", exception);
        }
    }


    /**
     * Gets ProjectDataModel if it is loaded for given project. This method
     *  is not null safe. If no datamodel has been loaded for the given project
     *  then this method will return null.
     * @param project Project
     * @return Project dataModel for project, null if it has not yet been loaded.
     */
    public static ProjectDataModel getExistingProjectDataModel(IContainer project)
    {
    	ProjectDataModel projectDM = (ProjectDataModel) PROJECT_VS_MODEL_HASH
                .get(project);
    	return projectDM;
    }

    /**
     * Gets ProjectDataModel for given projects.
     * @param project Project
     * @return Project dataModel for project.
     */
    public static ProjectDataModel getProjectDataModel(IContainer project)
    {
    	ProjectDataModel projectDM = (ProjectDataModel) PROJECT_VS_MODEL_HASH
                .get(project);
        return (projectDM != null) ? projectDM : new ProjectDataModel(project);
    }
    /**
     * Removes the PDM Instance from the HashMap so that
     * Next time when that project is accessed, it will create
     * new PDM and reads the data from the xmi again.
     * @param project Project
     */
    public static void removeProjectDataModel(IContainer project)
    {
        if (PROJECT_VS_MODEL_HASH.containsKey(project)) {
            PROJECT_VS_MODEL_HASH.remove(project);
        }
        removeSubModelMapReader((IProject) project);
    }
    /**
     * Removes the SubModelMapReader from hash map
     * @param project - IProject
     */
    private static void removeSubModelMapReader(IProject project)
    {
    	SubModelMapReader.removeSubModelMappingReader(
    			project,"component", "resource");
    	SubModelMapReader.removeSubModelMappingReader(
    			project,"resource", "alarm");
		
	}
	/**
     * Gets "caEditorInput" for this project.
     * @return _caEditorInput for the project.
     */
    public  IEditorInput getCAEditorInput()
    {

        return _caEditorInput;
    }
    /**
     * Sets "caEditorInput" for this project.
     * @param caEditorInput IEditorInput
     */
    public void setCAEditorInput(IEditorInput caEditorInput)
    {
         _caEditorInput = caEditorInput;
    }

    /**
     * Gets "compEditorInput" for this project.
     * @return _compEditorInput for the project.
     */
    public  IEditorInput getComponentEditorInput()
    {

        return _compEditorInput;
    }
    /**
     * Sets "compEditorInput" for this project.
     * @param compEditorInput IEditorInput
     */
    public void setComponentEditorInput(IEditorInput compEditorInput)
    {
         _compEditorInput = compEditorInput;
    }
    
    /**
     * Gets ManageabilityEditorInput for this project.
     * @return _manageabilityEditorInput for the project.
     */
    public  IEditorInput getManageabilityEditorInput()
    {

        return _manageabilityEditorInput;
    }
    /**
     * Sets "_manageabilityEditorInput" for this project.
     * @param ManageabilityEditorInput IEditorInput
     */
    public void setManageabilityEditorInput(IEditorInput manageabilityEditorInput)
    {
         _manageabilityEditorInput = manageabilityEditorInput;
    }
    /**
     * Returns Model TrackListener
     * @return Listener
     */
    public ModelTrackListener getModelTrackListener() {
		return _trackListener;
	}
    /**
     * Loads Template Group MapInfo
     *
     */
    private void readTemplateGroupModel()
    {
        URL url = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.TEMPLATEGROUP_MAPPING_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
            EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());
           
            //Now get the xml file from the project which may or
            //may not have data
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + ICWProject.TEMPLATEGROUP_MAPPING_XML_FILENAME;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            Resource resource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
            
                                
            if(resource.getContents().isEmpty()){
            	EObject obj = pack.getEFactoryInstance().create((EClass) pack.getEClassifier("componentMapInformation"));
            	resource.getContents().add(obj);
            	EcoreModels.save(resource);
            }
            _compTemplateModel = new Model(resource, (NotifyingList) resource
                    .getContents(), pack);
            _templateGroupListener = new TemplateGroupListener(resource);
            _templateModelTrackListener = new TemplateMappingModelTrackListener(this);
            EcoreUtils.addListener(_compTemplateModel.getEList(), _templateModelTrackListener, 4);
        } catch (Exception exception) {
            LOG.error("Error while Loading Template Group Map XML!", exception);
        }
    }
    /**
     * Returns Component Template Model which contains comp
     * template mapping
     * @return Model 
     */
    public Model getComponentTemplateModel()
    {
    	if(_compTemplateModel == null)
    		readTemplateGroupModel();
    	return _compTemplateModel;
    }
    /**
     * Removes the Dependency Listeners put on the model
     *
     */
    public void removeDependencyListeners()
    {
        if (_caModel != null) {
            EcoreUtils.removeListener(_caModel.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_caModel.getEList(), _trackListener, -1);
        }
        if (_componentModel != null) {
            EcoreUtils.removeListener(_componentModel.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_componentModel.getEList(), _trackListener, -1);
            EcoreUtils.removeListener(_componentModel.getEList(), _templateGroupListener, -1);
        }
        if (_alarmProfiles != null) {
            EcoreUtils.removeListener(_alarmProfiles.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_alarmProfiles.getEList(), _trackListener, -1);
        }
        if(_componentResourceMapModel != null) {
        	EcoreUtils.removeListener(_componentResourceMapModel.getEList(), _resourceMapModelTrackListener, 6);
        }
        if(_resourceAlarmMapModel != null) {
        	EcoreUtils.removeListener(_resourceAlarmMapModel.getEList(), _alarmMapModelTrackListener, 6);
        }
        if (_nodeProfiles != null) {
            EcoreUtils.removeListener(_nodeProfiles.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_nodeProfiles.getEList(), _amfModelTrackListener, -1);
        }
        if (_eoDefinitions != null) {
            EcoreUtils.removeListener(_eoDefinitions.getEList(), _dependencyListener, -1);
        }
        if (_eoConfiguration != null) {
            EcoreUtils.removeListener(_eoConfiguration.getEList(), _dependencyListener, -1);
        }
    }

	/**
	 * Copies all the config files to the temp directory. Versioning info is
	 * being removed during the copy.
	 */
	public static void copyConfigFilesToTempDir(IProject project) {
		File tempConfigDir = project.getFolder(
				new Path(ICWProject.PROJECT_TEMP_DIR + Path.SEPARATOR
						+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME)).getLocation()
				.toFile();
		if (tempConfigDir.exists())
			ClovisFileUtils.deleteDirectory(tempConfigDir);
		tempConfigDir.mkdirs();

		File configDir = project.getFolder(
				new Path(ICWProject.CW_PROJECT_CONFIG_DIR_NAME)).getLocation()
				.toFile();
		File[] configFiles = configDir.listFiles(new FileFilter() {
			public boolean accept(File pathname) {
				if (pathname.toString().endsWith(".xml")) {
					return true;
				}
				return false;
			}
		});

		for (File file : configFiles) {
			removeVersioningFromFile(file.getAbsolutePath(), tempConfigDir
					+ File.separator + file.getName());
		}
	}

	/**
	 * Removes the versioning from source file and saves it to the target.
	 * 
	 * @param source
	 * @param target
	 */
	private static void removeVersioningFromFile(String source, String target) {
		Node node = ClovisDomUtils.buildDocument(source).getDocumentElement();
		NodeList childList;

		for (int i = 0; i < 2; i++) {
			childList = node.getChildNodes();
			for (int j = 0; j < childList.getLength(); j++) {
				if (childList.item(j).getNodeType() == Node.ELEMENT_NODE) {
					node = childList.item(j);
					break;
				}
			}
		}

		Document document = ClovisDomUtils.buildDocument("");
		document.appendChild(document.importNode(node, true));
		ClovisDomUtils.saveDocument(document, target);
	}
	/**
	 * Returns Loaded Mibs for this project
	 * @return
	 */
	public ArrayList getLoadedMibs() {
		if (_loadedMibs.size() == 0) {
			try {
				String fileNames = getProject().getPersistentProperty(
						new QualifiedName("", "MIB_FILE_NAMES"));
				if (fileNames == null || fileNames.equals(""))
					return _loadedMibs;
				String names[] = fileNames.split(",");
				for (int i = 0; i < names.length; i++) {
					_loadedMibs.add(names[i]);
				}
			} catch (CoreException e) {
				e.printStackTrace();
			}
		}
		return _loadedMibs;
	}
	/**
	 * Sets modified flag
	 * @param modified
	 */
	public void setModified(boolean modified) {
		_modifiedAfterValidation = modified;
	}
	/**
	 * Returns modified flag
	 * @return
	 */
	public boolean isModified() {
		return _modifiedAfterValidation;
	}
	/**
	 * Returns model problems
	 * @return
	 */
	public List getModelProblems() {
		return _modelProblems;
	}
	/**
	 * Set model problems
	 * @param problems
	 */
	public void setModelProblems(List problems) {
		_modelProblems = problems;
	}
}
