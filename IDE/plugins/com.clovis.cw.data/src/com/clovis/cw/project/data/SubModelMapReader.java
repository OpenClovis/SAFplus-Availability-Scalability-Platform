package com.clovis.cw.project.data;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
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

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;

/**
 * 
 * @author shubhada
 * Class which reads the mapping data between different sub models
 */
public class SubModelMapReader
{
	private static final Log LOG = Log.getLog(DataPlugin.getDefault());
	private static boolean initialized;
	private static EClass mappingEClass = null;
	private static EPackage mappingPackage = null;
	private IProject _project = null;
	private Resource _mappingResource = null;
	private Model _mapModel = null;
	private HashMap _linkNameObjMap = new HashMap();
	private List _srcObjList = new ClovisNotifyingListImpl(); 
	private static final Hashtable MODEL_LINKID_VS_READER_HASH = new Hashtable();
	/**
	 * Constructor
	 * 
	 * @param project - IProject
	 * @param srcModel - Source model name
	 * @param targetModel - Target model name
	 */
	private SubModelMapReader(IProject project, String srcModel, String targetModel)
	{
		   _project = project;
		   MODEL_LINKID_VS_READER_HASH.put(project.toString() + "_" + srcModel + "_" + targetModel, this);
		   readMappingFile(srcModel, targetModel);
	}
	
	//	Initialize
    static {
        if (!initialized) {
        	mappingPackage = readEcoreFiles();
        	mappingEClass = (EClass) mappingPackage.getEClassifier("mapInformation");
        	initialized = true;
        }
    }
    /**
     * Gets the Model Mapping reader for given source and target model.
     * @param project Project - IProject
     * @return Model Mapping reader.
     */
    public static SubModelMapReader getSubModelMappingReader(IProject project,
    		String srcModel, String targetModel)
    {
    	SubModelMapReader reader = (SubModelMapReader)
    		MODEL_LINKID_VS_READER_HASH.get(project.toString() + "_" + srcModel + "_" + targetModel);
        return (reader != null) ? reader : new SubModelMapReader(project, srcModel, targetModel);
    }
    /**
     * Gets the Model Mapping reader for given source and target model.
     * @param project Project - IProject
     * @return Model Mapping reader.
     */
    public static void removeSubModelMappingReader(IProject project,
    		String srcModel, String targetModel)
    {
    	if (MODEL_LINKID_VS_READER_HASH.containsKey(
    			project.toString() + "_" + srcModel + "_" + targetModel)) {
    		MODEL_LINKID_VS_READER_HASH.remove(
    				project.toString() + "_" + srcModel + "_" + targetModel);
    	}
        
    }
	/**
    *
    * Reads the Ecore File.
    */
   private static EPackage readEcoreFiles()
   {
       try {
           URL cpmURL = DataPlugin.getDefault().find(new Path("model"
                   + File.separator + ICWProject.SUBMODEL_MAPPING_ECORE_FILENAME));
           File ecoreFile = new Path(Platform.resolve(cpmURL).getPath())
                   .toFile();
           return EcoreModels.get(ecoreFile.getAbsolutePath());
          
       } catch (IOException ex) {
           LOG.error("Model mapping ecore file cannot be read", ex);
       }
       return null;
   }
    /**
     * Reads the mapping file in which all the link information of
     * submodels is present
     *  
     * @param srcModel - Source Model name
     * @param targetModel - Target Model name
     */
	private void readMappingFile(String srcModel, String targetModel)
	{
		try {
            
            String dataFilePath = _project.getLocation().toOSString()
                                  + File.separator
                                  + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                                  + File.separator
                                  + srcModel + "_" + targetModel + "_map.xml" ;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            _mappingResource = xmiFile.exists() ? EcoreModels.
                    getUpdatedResource(uri) : EcoreModels.create(uri);
                    
            if (_mappingResource.getContents().isEmpty()) {
	           	 EObject mapObj = EcoreUtils.createEObject(mappingEClass, true);
	           	 _mappingResource.getContents().add(mapObj);
	           	 //EcoreUtils.addListener(mapObj, _trackListener, -1);
	           	 EcoreUtils.setValue(mapObj, "sourceModel", srcModel + "_model");
	           	 EcoreUtils.setValue(mapObj, "targetModel", targetModel + "_model");
	           	
            }
            _mapModel = new Model(_mappingResource, (EObject)
            		_mappingResource.getContents().get(0));
            initMap();
        } catch (Exception exception) {
            LOG.error("Error while loading sub model mapping data", exception);
        }
		
	}
	/**
	 * Initialize the link map
	 *
	 */
	private void initMap()
	{
		EObject mapObj = (EObject) _mapModel.getEObject();
		List links = (List) EcoreUtils.getValue(mapObj, "link");
		for (int i = 0; i < links.size(); i++) {
			EObject link = (EObject) links.get(i);
			_linkNameObjMap.put(EcoreUtils.getName(link), link);
			
		}
		
	}
	/**
	 * 
	 * @return the Map Eclass
	 */
	public EClass getMappingClass()
	{
		return mappingEClass;
	}
	/**
	 * 
	 * @return The EPackage
	 */
	public EPackage getMappingPackage()
	{
		return mappingPackage;
	}
	/**
	 * Returns all the link targets for given link object and linkSource 
	 * 
	 * @param link - Link object
	 * @param linkSourceRdn - Link Source rdn
	 * @return the link targets
	 */
	public static List getLinkTargetObjects(EObject link, String linkSourceRdn)
	{
		if (link != null) {
			List linkDetails = (List) EcoreUtils.getValue(link, "linkDetail");
			for (int j = 0; j < linkDetails.size(); j++) {
				EObject linkDetailObj = (EObject) linkDetails.get(j);
				String linkSrc = (String) EcoreUtils.getValue(
						linkDetailObj, "linkSourceRdn");
				if (linkSrc != null && linkSrc.equals(linkSourceRdn)) {
					List linkTargets = (List) EcoreUtils.getValue(
							linkDetailObj, "linkTarget");
					return linkTargets;
				}
			}
		}
		return null;
	}
	/**
	 * Returns all the link targets for given linkname and linkSource 
	 * 
	 * @param linkName - Link Name
	 * @param linkSource - Link Source
	 * @return the link targets
	 */
	public List getLinkTargetObjects(String linkName, String linkSource)
	{
		EObject link = (EObject) _linkNameObjMap.get(linkName);
		if (link != null) {
			List linkDetails = (List) EcoreUtils.getValue(link, "linkDetail");
			for (int j = 0; j < linkDetails.size(); j++) {
				EObject linkDetailObj = (EObject) linkDetails.get(j);
				String linkSrc = (String) EcoreUtils.getValue(
						linkDetailObj, "linkSource");
				if (linkSrc.equals(linkSource)) {
					List linkTargets = (List) EcoreUtils.getValue(
							linkDetailObj, "linkTarget");
					return linkTargets;
				}
			}
		}
		return null;
	}
	/**
	 * 
	 * @param linkName - Link Name
	 * @return the Link EObject matching the given link name
	 */
	public EObject getLinkObject(String linkName)
	{
		return (EObject) _linkNameObjMap.get(linkName); 
	}
	/**
	 * 
	 * @param mapObj - Map Object
	 * @param linkName - Link Name
	 * @return the matched link object with linkName 
	 */
	public static EObject getLinkObject(EObject mapObj, String linkName)
	{
		List links = (List) EcoreUtils.getValue(mapObj, "link");
		for (int i = 0; i < links.size(); i++) 
		{
			EObject link = (EObject) links.get(i);
			String linkType = EcoreUtils.getValue(link, "linkType").toString();
			if (linkType.equals(linkName)) {
				return link;
			}
		}
		return null;
	}
	/**
	 * 
	 * @param linkName - Link name
	 * @return the newly created link
	 */
	public EObject createLink(String linkName)
	{
		EObject linkObj = (EObject) _linkNameObjMap.get(linkName);
		if (linkObj == null) {
			EObject mapObj = (EObject) _mapModel.getEObject();
			EReference linkRef = (EReference) mapObj.eClass().
				getEStructuralFeature("link");
			linkObj = EcoreUtils.createEObject(linkRef.getEReferenceType(), true);
			EcoreUtils.setValue(linkObj, "linkType", linkName);
			List links = (List) mapObj.eGet(linkRef);
			links.add(linkObj);
			_linkNameObjMap.put(linkName, linkObj);
		}
		return linkObj;
	}
	/**
	 * Adds the LinkDetils object corresponding to given link source and link name
	 * if it does not exist already. 
	 * 
	 * @param linkName - Link name
	 * @param linkSource - Link Source name
	 * @return the Link Target list 
	 */
	public List createLinkTargets(String linkName, String linkSource)
	{
		EObject link = (EObject) _linkNameObjMap.get(linkName);
		if (link != null) {
			EReference linkDetailRef = (EReference) link.eClass().
				getEStructuralFeature("linkDetail");
			List linkDetails = (List) link.eGet(linkDetailRef);
			for (int j = 0; j < linkDetails.size(); j++) {
				EObject linkDetailObj = (EObject) linkDetails.get(j);
				String linkSrc = (String) EcoreUtils.getValue(
						linkDetailObj, "linkSource");
				if (linkSrc.equals(linkSource)) {
					return (List) EcoreUtils.getValue(
							linkDetailObj, "linkTarget");
				}
			}
			EObject linkDetailObj = EcoreUtils.createEObject(linkDetailRef.
					getEReferenceType(), true);
			EcoreUtils.setValue(linkDetailObj, "linkSource", linkSource);
			linkDetails.add(linkDetailObj);
			return (List) EcoreUtils.getValue(linkDetailObj, "linkTarget");
			
		}
		return null;
	}
	
	/**
	 * Adds the LinkDetils object corresponding to given link source and link name
	 * if it does not exist already. 
	 * 
	 * @param link - Link object
	 * @param linkSource - Link Source name
	 * @param linkSourceRdn - RDN of link source object
	 * @return the Link Target list 
	 */
	public static List createLinkTargets(EObject link,
			String linkSource, String linkSourceRdn)
	{
		if (link != null) {
			EReference linkDetailRef = (EReference) link.eClass().
				getEStructuralFeature("linkDetail");
			List linkDetails = (List) link.eGet(linkDetailRef);
			for (int j = 0; j < linkDetails.size(); j++) {
				EObject linkDetailObj = (EObject) linkDetails.get(j);
				String linkSrc = (String) EcoreUtils.getValue(
						linkDetailObj, "linkSource");
				if (linkSrc.equals(linkSource)) {
					return (List) EcoreUtils.getValue(
							linkDetailObj, "linkTarget");
				}
			}
			EObject linkDetailObj = EcoreUtils.createEObject(linkDetailRef.
					getEReferenceType(), true);
			EcoreUtils.setValue(linkDetailObj, "linkSource", linkSource);
			EcoreUtils.setValue(linkDetailObj, "linkSourceRdn", linkSourceRdn);
			linkDetails.add(linkDetailObj);
			return (List) EcoreUtils.getValue(linkDetailObj, "linkTarget");
			
		}
		return null;
	}
	
	/**
	 * initializes the rdn of the link source objects
	 * 
	 * @param objList - List of link source objects
	 */
	public void initializeLinkRDN(List objList)
	{
		_srcObjList = objList;
		
		Iterator iterator = _linkNameObjMap.keySet().iterator();
		while (iterator.hasNext()) {
			String linkName = (String) iterator.next();
			EObject linkObj = (EObject) _linkNameObjMap.get(linkName);
			if (linkObj != null) {
				List linkDetails = (List) EcoreUtils.getValue(linkObj, "linkDetail");
				for (int i = 0; i < linkDetails.size(); i++) {
					EObject linkDetailObj = (EObject) linkDetails.get(i);
					String linkSrc = (String) EcoreUtils.getValue(
							linkDetailObj, "linkSource");
					Iterator iter = _srcObjList.iterator();
					while (iter.hasNext()) {
						EObject obj = (EObject) iter.next();
						String rdn = EcoreUtils.getValue(obj, ModelConstants.
								RDN_FEATURE_NAME).toString();
						String name = EcoreUtils.getName(obj);
						if (linkSrc.equals(name)) {
							EcoreUtils.setValue(linkDetailObj, "linkSourceRdn", rdn);
						}
					}
				}
			}
		}
		
	}
	/**
	 * 
	 * @return the map model
	 */
	public Model getMapModel()
	{
		return _mapModel;
	}
	
}
