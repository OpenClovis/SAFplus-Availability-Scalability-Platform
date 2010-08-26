/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/AmfDefinitionsReader.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.Vector;

import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;


/**
 * @author shubhada
 *
 * Amf Definitions Reader Class which reads the clAmfConfig.xml file.
 */
public class AmfDefinitionsReader
{
    private EPackage            _amfDefPackage = null;
    private EClass              _amfDefClass = null;
    private Resource            _amfDefResource = null;
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    private static final String AMFDEF_XML_FILE_NAME = "clAmfDefinitions.xml";
    private static final String
        AMFDEF_ECORE_FILE_NAME = "amfDefinitions.ecore";
    private static final String NODEMAP_FILE_NAME = "nodeCopyMap.xmi";
    private static final String SGMAP_FILE_NAME = "sgCopyMap.xmi";
    private static final String SUMAP_FILE_NAME = "suCopyMap.xmi";
    private static final String SIMAP_FILE_NAME = "siCopyMap.xmi";
    private static final String COMPMAP_FILE_NAME = "compCopyMap.xmi";
    private static final String TIMEOUTMAP_FILE_NAME = "timeoutCopyMap.xmi";
    private static final String HEALTHCHECKMAP_FILE_NAME = "healthCheckCopyMap.xmi";
    private static final String CSIMAP_FILE_NAME = "csiCopyMap.xmi";
    
    private ComponentDataUtils _compUtils = null;
    /**
     * constructor
     *@param resource - Project Resource
     *
     */
    public AmfDefinitionsReader(IResource resource)
    {
        readEcoreFiles();
        readNodeProfiles(resource);
    }
    /**
     *
     * Reads the Ecore File.
     */
    private void readEcoreFiles()
    {
        try {
            URL cpmURL = DataPlugin.getDefault().find(new Path("model"
                    + File.separator + AMFDEF_ECORE_FILE_NAME));
            File ecoreFile = new Path(Platform.resolve(cpmURL).getPath())
                    .toFile();
            _amfDefPackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            _amfDefClass = (EClass) _amfDefPackage.getEClassifier("amfTypes");
        } catch (IOException ex) {
            LOG.error("Amf Denition Ecore File cannot be read", ex);
        }
    }
     /**
     * read the Node Profiles from the XMI files in the Workspace.
     * @param project - Project Resource
     */
    private void readNodeProfiles(IResource project)
    {
        try {
//          Now get the CPM config xmi file from the node dir under project.
            String dataFilePath = project.getLocation().toOSString()
                + File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + AMFDEF_XML_FILE_NAME;
            URI uri = URI.createFileURI(dataFilePath);
            /*File xmlFile = new File(dataFilePath);
            _amfDefResource = xmlFile.exists()
            ? EcoreModels.getResource(uri) : EcoreModels.create(uri);*/
            _amfDefResource = EcoreModels.create(uri);
            List amfList = _amfDefResource.getContents();
            if (amfList.isEmpty()) {
                EObject amfObj = EcoreUtils.createEObject(_amfDefClass, true);
                amfList.add(amfObj);
            }
            EObject amfObj = (EObject) amfList.get(0);
            EReference nodeTypesRef = (EReference) _amfDefClass
                .getEStructuralFeature("nodeTypes");
            EObject nodeTypesObj = (EObject) amfObj.eGet(nodeTypesRef);
            if (nodeTypesObj == null) {
                nodeTypesObj = EcoreUtils.createEObject(nodeTypesRef.
                        getEReferenceType(), true);
                amfObj.eSet(nodeTypesRef, nodeTypesObj);
            }
            EReference sgTypesRef = (EReference) _amfDefClass
            .getEStructuralFeature("sgTypes");
            EObject sgTypesObj = (EObject) amfObj.eGet(sgTypesRef);
            if (sgTypesObj == null) {
                sgTypesObj = EcoreUtils.createEObject(sgTypesRef.
                    getEReferenceType(), true);
                amfObj.eSet(sgTypesRef, sgTypesObj);
            }
            EReference suTypesRef = (EReference) _amfDefClass
            .getEStructuralFeature("suTypes");
            EObject suTypesObj = (EObject) amfObj.eGet(suTypesRef);
            if (suTypesObj == null) {
                suTypesObj = EcoreUtils.createEObject(suTypesRef.
                getEReferenceType(), true);
                amfObj.eSet(suTypesRef, suTypesObj);
            }
            EReference siTypesRef = (EReference) _amfDefClass
            .getEStructuralFeature("siTypes");
            EObject siTypesObj = (EObject) amfObj.eGet(siTypesRef);
            if (siTypesObj == null) {
                siTypesObj = EcoreUtils.createEObject(siTypesRef.
                getEReferenceType(), true);
                amfObj.eSet(siTypesRef, siTypesObj);
            }
            EReference compTypesRef = (EReference) _amfDefClass
            .getEStructuralFeature("compTypes");
            EObject compTypesObj = (EObject) amfObj.eGet(compTypesRef);
            if (compTypesObj == null) {
                compTypesObj = EcoreUtils.createEObject(compTypesRef.
                getEReferenceType(), true);
                amfObj.eSet(compTypesRef, compTypesObj);
            }
            EReference csiTypesRef = (EReference) _amfDefClass
            .getEStructuralFeature("csiTypes");
            EObject csiTypesObj = (EObject) amfObj.eGet(csiTypesRef);
            if (csiTypesObj == null) {
                csiTypesObj = EcoreUtils.createEObject(csiTypesRef.
                getEReferenceType(), true);
                amfObj.eSet(csiTypesRef, csiTypesObj);
            }
        } catch (Exception e) {
            LOG.error("Error reading Node Profile XMI File", e);
        }
    }
    /**
     * Writes the AmfDefs file after initializing the Objects
     * @param model Model of Component Editor
     */
    public void writeAmfDefinitions(Model model)
    {
    	_compUtils = new ComponentDataUtils(model.getEList());
        EObject amfObj = (EObject) getAMFDefResource().
        getContents().get(0);
        EReference nodeTypesRef = (EReference) _amfDefClass
            .getEStructuralFeature("nodeTypes");
        EReference nodeTypeRef = (EReference) nodeTypesRef
            .getEReferenceType().getEStructuralFeature("nodeType");
        EObject nodeTypesObj = (EObject) amfObj.eGet(nodeTypesRef);
        List nodesList = (List) nodeTypesObj.eGet(nodeTypeRef);
        nodesList.clear();
        List nodeList = getfilterList(model.getEList(),
                ComponentEditorConstants.NODE_NAME);
        for (int i = 0; i < nodeList.size(); i++) {
            EObject nodeObj = (EObject) nodeList.get(i);
            EObject nObj = EcoreUtils.createEObject(nodeTypeRef.
                    getEReferenceType(), true);
            nodesList.add(nObj);
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + NODEMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(nodeObj, nObj, list);
            } catch (Exception exception) {
                LOG.error("Error while reading NodeCopyMap data file",
                        exception);
            }
        }
        EReference sgTypesRef = (EReference) _amfDefClass
        .getEStructuralFeature("sgTypes");
        EReference sgTypeRef = (EReference) sgTypesRef
            .getEReferenceType().getEStructuralFeature("sgType");
        EObject sgTypesObj = (EObject) amfObj.eGet(sgTypesRef);
        List sgList = (List) sgTypesObj.eGet(sgTypeRef);
        sgList.clear();
        List serviceGrpList = getfilterList(model.getEList(),
                ComponentEditorConstants.SERVICEGROUP_NAME);
        for (int i = 0; i < serviceGrpList.size(); i++) {
            EObject sgObj = (EObject) serviceGrpList.get(i);
            EObject sObj = EcoreUtils.createEObject(sgTypeRef.
                    getEReferenceType(), true);
            sgList.add(sObj);
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + SGMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(sgObj, sObj, list);
                
                //SPECIAL HANDLING
                // don't write alpha factor if it is the default value
                Object value = EcoreUtils.getValue(sObj, "alphaFactor");
                if (value != null)
                {
            		EAttribute attr = (EAttribute)sObj.eClass().getEStructuralFeature("alphaFactor");
            		Integer defVal = (Integer)attr.getDefaultValue();
            		if (defVal.equals(value))
                	{
                		EcoreUtils.clearValue(sObj, "alphaFactor");
                	}
                }
                
            } catch (Exception exception) {
               LOG.error("Error while reading SGCopyMap data file", exception);
            }
        }
        EReference suTypesRef = (EReference) _amfDefClass
        .getEStructuralFeature("suTypes");
        EReference suTypeRef = (EReference) suTypesRef
            .getEReferenceType().getEStructuralFeature("suType");
        EObject suTypesObj = (EObject) amfObj.eGet(suTypesRef);
        List suList = (List) suTypesObj.eGet(suTypeRef);
        suList.clear();
        List serviceUnitList = getfilterList(model.getEList(),
                ComponentEditorConstants.SERVICEUNIT_NAME);
        for (int i = 0; i < serviceUnitList.size(); i++) {
            EObject suObj = (EObject) serviceUnitList.get(i);
            EObject sObj = EcoreUtils.createEObject(suTypeRef.
                    getEReferenceType(), true);
            suList.add(sObj);
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + SUMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(suObj, sObj, list);
            } catch (Exception exception) {
               LOG.error("Error while reading SUCopyMap data file", exception);
            }
            
            // get collection of components under the SU
            ArrayList<EObject> components = new ArrayList<EObject>();
    		components = getComponentsInSu(model.getEList(), suObj);
    		int noOfComponents = components.size();

    		// set the number of components
    		EcoreUtils.setValue(sObj, "numComponents", String.valueOf(noOfComponents));
            
    		// set the whether or not the SU is pre-instantiable
    		//  to be pre-instantiable is must have at least one SA aware
    		//  component as a child
    		boolean isPreinstantiable = false;
            for (int j=0; j<components.size(); j++)
            {
            	EObject component = components.get(j);
            	EClass compClass = component.eClass();
            	if (compClass.getName().equals("SAFComponent"))
            	{
            		isPreinstantiable = true;
            	}
            }
            if (isPreinstantiable)
            {
            	EcoreUtils.setValue(sObj, "isPreinstantiable", "CL_TRUE");
            } else {
            	EcoreUtils.setValue(sObj, "isPreinstantiable", "CL_FALSE");
            }
        }
        initializeObjects(model);

        saveResource(getAMFDefResource());
    }

    /**
     * get list of Components contained in a SU
     * @param list List of EObjects in Component editor
     * @param suObj EObject corresponding to the SU.
     * @return list of components contained in the SU
     */
    private ArrayList getComponentsInSu(EList list, EObject suObj) {
		ArrayList<EObject> components = new ArrayList<EObject>();
		String suObjKey = (String) EcoreUtils.getValue(suObj,
				ModelConstants.RDN_FEATURE_NAME);
		EObject rootObject = (EObject) list.get(0);
		EList connList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(ComponentEditorConstants.AUTO_REF_NAME));
		for (int i = 0; i < connList.size(); i++) {
			EObject editorObj = (EObject) connList.get(i);
			String type = (String) (String) EcoreUtils.getValue(editorObj,
					ComponentEditorConstants.CONNECTION_TYPE);
			if (type.equals(ComponentEditorConstants.CONTAINMENT_NAME)) {
				String srcKey = (String) EcoreUtils.getValue(editorObj,
						ComponentEditorConstants.CONNECTION_START);
				if (srcKey != null && srcKey.equals(suObjKey)) {
					EObject targetObj = _compUtils.getTarget(editorObj);
					EList superClasses = targetObj.eClass().getESuperTypes();
					for (int j = 0; j < superClasses.size(); j++) {
						EClass eclass = (EClass) superClasses.get(j);
						if (eclass.getName().equals("Component")) {
							components.add(targetObj);
						}
					}
				}
			}
		}
		return components;
	}

    /**
	 * initilize objects from component editor data
	 * 
	 * @param model
	 *            Model of Component Editor
	 */
    private void initializeObjects(Model model)
    {
        EClass amfClass = getAmfDefClass();
        EObject amfObj = (EObject) getAMFDefResource().
        getContents().get(0);
        EReference siTypesRef = (EReference) amfClass
        .getEStructuralFeature("siTypes");
        EReference siTypeRef = (EReference) siTypesRef
            .getEReferenceType().getEStructuralFeature("siType");
        EObject siTypesObj = (EObject) amfObj.eGet(siTypesRef);
        List siList = (List) siTypesObj.eGet(siTypeRef);
        siList.clear();
        List serviceInstList = getfilterList(model.getEList(),
                ComponentEditorConstants.SERVICEINSTANCE_NAME);
        for (int i = 0; i < serviceInstList.size(); i++) {
            EObject siObj = (EObject) serviceInstList.get(i);
            EObject sObj = EcoreUtils.createEObject(siTypeRef.
                    getEReferenceType(), true);
            siList.add(sObj);
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + SIMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(siObj, sObj, list);
            } catch (Exception exception) {
               LOG.error("Error while reading SICopyMap data file", exception);
            }
        }
        EReference compTypesRef = (EReference) amfClass
        .getEStructuralFeature("compTypes");
        EReference compTypeRef = (EReference) compTypesRef
            .getEReferenceType().getEStructuralFeature("compType");
        EObject compTypesObj = (EObject) amfObj.eGet(compTypesRef);
        List compList = (List) compTypesObj.eGet(compTypeRef);
        compList.clear();
        List componentList = getfilterList(model.getEList(),
                ComponentEditorConstants.COMPONENT_NAME);
        for (int i = 0; i < componentList.size(); i++) {
            EObject compObj = (EObject) componentList.get(i);
            EObject cObj = EcoreUtils.createEObject(compTypeRef.
                    getEReferenceType(), true);
            compList.add(cObj);
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + COMPMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(compObj, cObj, list);
            } catch (Exception exception) {
             LOG.error("Error while reading CompCopyMap data file", exception);
            }
            EStructuralFeature feature =
                compObj.eClass().getEStructuralFeature("environmentVariable");
            EStructuralFeature cmpfeature = cObj.eClass().
                getEStructuralFeature("envs");
            EObject envObj = EcoreUtils.createEObject(((EReference) cmpfeature).
              getEReferenceType(), true);
            cObj.eSet(cmpfeature, envObj);
            EReference envfeature = (EReference) ((EReference) cmpfeature).
            getEReferenceType().getEStructuralFeature("nameValue");
            List envList = (List) envObj.eGet(envfeature);
            List compEnvList = (List) compObj.eGet(feature);
            copyComplexValues(compEnvList, envList, envfeature);
            List csiTypeList = (List) EcoreUtils.getValue((EObject) EcoreUtils.getValue(cObj, "csiTypes"), "csiType");
            List compCsiTypeList = (List) EcoreUtils.getValue((EObject) EcoreUtils.getValue(compObj, "csiTypes"), "csiType");
            Iterator<EObject> itr = compCsiTypeList.iterator();
            EObject csiTypeObj;
			while (itr.hasNext()) {
				csiTypeObj = EcoreUtils.createEObject((EClass) cObj.eClass()
						.getEPackage().getEClassifier("CompCSIType"), true);
				EcoreUtils.setValue(csiTypeObj, "name", EcoreUtils.getName(itr
						.next()));
				csiTypeList.add(csiTypeObj);
			}
            feature =
                compObj.eClass().getEStructuralFeature("commandLineArgument");
            cmpfeature = cObj.eClass().
                getEStructuralFeature("args");
            EObject argObj = EcoreUtils.createEObject(((EReference) cmpfeature).
              getEReferenceType(), true);
            cObj.eSet(cmpfeature, argObj);
            EReference argfeature = (EReference) ((EReference) cmpfeature).
            getEReferenceType().getEStructuralFeature("argument");
            List argList = (List) argObj.eGet(argfeature);
            List compArgList = (List) compObj.eGet(feature);
            copyComplexValues(compArgList, argList, argfeature);
            EReference supportedCSIref = (EReference) cObj.eClass().
                getEStructuralFeature("supportedCSITypes");
            List csiTypesList = (List) EcoreUtils.
                getValue(cObj, supportedCSIref.getName());
            List compCsiTypesList = (List) EcoreUtils.
                getValue(compObj, supportedCSIref.getName());
            copyComplexValues(compCsiTypesList, csiTypesList, supportedCSIref);
            EObject timeoutObj = (EObject) EcoreUtils.
                getValue(compObj, "timeouts");
            EObject toutObj = (EObject) EcoreUtils.
                getValue(cObj, "timeouts");
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + TIMEOUTMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list1 = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(timeoutObj, toutObj, list1);
            } catch (Exception exception) {
                LOG.error("Error while reading TimeoutCopyMap data file",
                        exception);
            }
            EObject healthCheckObj = (EObject) EcoreUtils.getValue(compObj,
					"healthCheck");
			EObject hCheckObj = (EObject) EcoreUtils.getValue(cObj, "healthCheck");
			try {
				URL url = DataPlugin.getDefault().getBundle().getEntry("/");
				url = Platform.resolve(url);
				String fileName = url.getFile() + "xml" + File.separator
						+ HEALTHCHECKMAP_FILE_NAME;
				URI uri = URI.createFileURI(fileName);
				List list1 = (List) EcoreModels.read(uri);
				EcoreUtils.copyValues(healthCheckObj, hCheckObj, list1);
			} catch (Exception exception) {
				LOG.error("Error while reading HealthCheckCopyMap data file",
						exception);
			}
        }
        EReference csiTypesRef = (EReference) amfClass
        .getEStructuralFeature("csiTypes");
        EReference csiTypeRef = (EReference) csiTypesRef
            .getEReferenceType().getEStructuralFeature("csiType");
        EObject csiTypesObj = (EObject) amfObj.eGet(csiTypesRef);
        List csiList = (List) csiTypesObj.eGet(csiTypeRef);
        csiList.clear();
        List csiCompList = getfilterList(model.getEList(),
                ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME);
        for (int i = 0; i < csiCompList.size(); i++) {
            EObject csiObj = (EObject) csiCompList.get(i);
            EObject cObj = EcoreUtils.createEObject(csiTypeRef.
                    getEReferenceType(), true);
            csiList.add(cObj);
            try {
                URL url = DataPlugin.getDefault().getBundle().getEntry("/");
                url = Platform.resolve(url);
                String fileName = url.getFile() + "xml" + File.separator
                    + CSIMAP_FILE_NAME;
                URI uri = URI.createFileURI(fileName);
                List list = (List) EcoreModels.read(uri);
                EcoreUtils.copyValues(csiObj, cObj, list);
            } catch (Exception exception) {
              LOG.error("Error while reading CsiCopyMap data file", exception);
            }
            EStructuralFeature cmpfeature = cObj.eClass().
            getEStructuralFeature("nameValueLists");
            EObject nameValueObj = EcoreUtils.createEObject(((EReference)
                cmpfeature).getEReferenceType(), true);
            cObj.eSet(cmpfeature, nameValueObj);
            EReference nvfeature = (EReference) ((EReference) cmpfeature).
            getEReferenceType().getEStructuralFeature("nameValue");
            List nvList = (List) EcoreUtils.getValue(nameValueObj,
                    nvfeature.getName());
            List compNvList = (List) EcoreUtils.getValue(csiObj,
                    cmpfeature.getName());
            copyComplexValues(compNvList, nvList, nvfeature);

        }

    }
    /**
    *
    * @param list List
    * @param className - Name on which filtering to be done
    * @return th filtered list
    */
   private List getfilterList(NotifyingList list, String className)
   {
       List compList = new Vector();
       if (className.equals(ComponentEditorConstants.COMPONENT_NAME)) {
           EObject rootObject = (EObject) list.get(0);
    	   compList.addAll((EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ComponentEditorConstants.SAFCOMPONENT_REF_NAME)));
    	   compList.addAll((EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME)));
       } else {
            EObject rootObject = (EObject) list.get(0);
			List refList = rootObject.eClass().getEAllReferences();
			for (int i = 0; i < refList.size(); i++) {
				EReference ref = (EReference) refList.get(i);
				if (ref.getEType().getName().equals(className)) {
					compList.addAll((EList) rootObject.eGet(ref));
				}
			}
       }
       return compList;
   }
   /**
    * @param srcList SourceList
    * @param destList DestList
    * @param ref EReference
    */
   private void copyComplexValues(List srcList, List destList, EReference ref)
   {
	   if(srcList != null){
	       for (int i = 0; i < srcList.size(); i++) {
	           EObject srcObj = (EObject) srcList.get(i);
	           EObject destObj = EcoreUtils.createEObject(
	                     ref.getEReferenceType(), true);
	           destList.add(destObj);
	           List features = srcObj.eClass().getEAllStructuralFeatures();
	           Properties map = new Properties();
	           for (int j = 0; j < features.size(); j++) {
	               EStructuralFeature feature = (EStructuralFeature)
	                   features.get(j);
	               map.put(feature.getName(), feature.getName());
	           }
               EcoreUtils.copyValues(srcObj, destObj, map);
	       }		   
	   }
   }
   
   
   /**
    * @param resource - Resource to be saved
    */
   private void saveResource(Resource resource)
   {
       try {
    	   EcoreModels.save(resource);
       } catch (IOException e) {
          LOG.error("Resource cannot be written",e);
       }

   }

    /**
     *
     * @return AMF config Resource
     */
    public Resource getAMFDefResource()
    {
        return _amfDefResource;
    }
    /**
     *
     * @return AMF Def eclass
     */
    public EClass getAmfDefClass()
    {
        return _amfDefClass;
    }
}
