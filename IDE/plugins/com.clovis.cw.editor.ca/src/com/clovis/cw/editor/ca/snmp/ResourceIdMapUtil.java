/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/ResourceIdMapUtil.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import java.io.File;
import java.net.URL;

import java.util.Map;
import java.util.List;
import java.util.HashMap;

import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;

import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.SnmpConstants;
/**
 * @author shubhada
 *
 * MO File Reader Class
 */
public class ResourceIdMapUtil implements ClassEditorConstants
{
    private Resource            _resource     = null;
    private EObject             _countWrapper = null;
    private EClass              _attrInfoClass     = null;
    private EClass              _resourceInfoClass = null;
    private HashMap             _moMap        = new HashMap();
    private HashMap             _keyObjectMap = new HashMap();
    private long                 _resMaxCount  = 0;
    private static final Log    LOG = Log.getLog(CaPlugin.getDefault());
    /**
     * Creates instance of this class for a project.
     * @param project IResource for project
     */
    public ResourceIdMapUtil(IResource project)
    {
        init(project);
    }
    /**
     * Reads Ecore and UI Ecore file and data type map properties file in first
     * call. Subsequent calls does not do anything.
     * @param project IResource for project
     */
    private void init(IResource project)
    {
        try {
            URL moURL = DataPlugin.getDefault().find(
                    new Path("model" + File.separator
                             + SnmpConstants.MO_ECORE_FILE_NAME));
            File ecoreFile = new Path(Platform.resolve(moURL).getPath())
                    .toFile();
            EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

            EClass counterClass =
                (EClass) pack.getEClassifier("CounterWrapper");
            _attrInfoClass     = (EClass) pack.getEClassifier("Attribute");
            _resourceInfoClass = (EClass) pack.getEClassifier("ResourceInfo");

            // Now get the xmi file from the project which may not exist
            String dataFilePath = project.getLocation().toOSString()
                + File.separator
                + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                + File.separator + SnmpConstants.MO_XMI_FILE_NAME;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            _resource = xmiFile.exists() ? EcoreModels.getResource(uri)
                    : EcoreModels.create(uri);
            List contents = (NotifyingList) _resource.getContents();
            if (contents.isEmpty()) {
                contents.add(EcoreUtils.createEObject(counterClass, true));
            }
            _countWrapper = (EObject) contents.get(0);
            createResourceKeyIdMap();
        } catch (Exception exception) {
            LOG.error("Error while reading the MO's", exception);
        }
    }
    /**
     * Create map of Key and OID.
     */
    private void createResourceKeyIdMap()
    {        
        _resMaxCount = ((Long) EcoreUtils.getValue(_countWrapper,
                "resourceCount")).longValue();
        List resourceInfoList = (List)
            EcoreUtils.getValue(_countWrapper, "ResourceInfo");
        for (int i = 0; i < resourceInfoList.size(); i++) {
            EObject moObj = (EObject) resourceInfoList.get(i);
            String key = (String) EcoreUtils.getValue(moObj,
            		ModelConstants.RDN_FEATURE_NAME);
            _moMap.put(key, EcoreUtils.getValue(moObj, "ID"));
            _keyObjectMap.put(key, moObj);
        }
    }
    /**
     * Process List of Attributes for one Resource.
     * @param attrList  List of Attributes.
     * @param attrInfoList List of counters for Attribute.
     * @param attrIdMap Key-ID Mapping
     * @param count Count of Attributes
     * @return Update count of attributes for future references.
     */
    private long processAttrList(List attrList, List attrInfoList,
                                 Map attrIdMap, long count)
    {
        for (int i = 0; i < attrList.size(); i++) {
            EObject obj = (EObject) attrList.get(i);
            String key = (String) EcoreUtils.getValue(obj,
            		ModelConstants.RDN_FEATURE_NAME);
            if (!attrIdMap.containsKey(key)) {
                count++;
                attrIdMap.put(key, new Long(count));
                EObject attrInfo =
                    EcoreUtils.createEObject(_attrInfoClass, true);
                EcoreUtils.setValue(attrInfo,
                		ModelConstants.RDN_FEATURE_NAME, key);
                EcoreUtils.setValue(attrInfo, "ID", String.valueOf(count));
                attrInfoList.add(attrInfo);
            }
        }
        return count;
    }
    /**
     * Update Mapping file for this input.
     * @param resourceList Resource List
     * @param override If existing oid is to be overridden.
     */
    public void updateMOList(List resourceList, boolean override)
    {
        List resourceInfoList = (List)
            EcoreUtils.getValue(_countWrapper, "ResourceInfo");
        for (int i = 0; i < resourceList.size(); i++) {
            EObject resource = (EObject) resourceList.get(i);
            String key = (String) EcoreUtils.getValue(resource,
            		ModelConstants.RDN_FEATURE_NAME);
            EObject resourceInfo = (EObject) _keyObjectMap.get(key);
            if (resourceInfo == null) {
                // Add new entry in Map/CounterWrapper
                _resMaxCount++;
                _moMap.put(key, new Long(_resMaxCount));
                resourceInfo =
                    EcoreUtils.createEObject(_resourceInfoClass, true);
                resourceInfoList.add(resourceInfo);
                EcoreUtils.setValue(resourceInfo,
                		ModelConstants.RDN_FEATURE_NAME, key);
                String countStr = String.valueOf(_resMaxCount);
                EcoreUtils.setValue(resourceInfo, "ID", countStr);
            }

            long atCount = ((Long) EcoreUtils.getValue(
                        resourceInfo, "attributeCount")).longValue();
            HashMap atMap = new HashMap();
            List attrInfoList = (List)
                EcoreUtils.getValue(resourceInfo, "Attributes");
            for (int j = 0; j < attrInfoList.size(); j++) {
                EObject obj = (EObject) attrInfoList.get(j);
                String atKey = (String) EcoreUtils.getValue(obj,
                		ModelConstants.RDN_FEATURE_NAME);
                atMap.put(atKey, EcoreUtils.getValue(obj, "ID"));
            }

            List attrList = null;
            // Update for Attributes.
            attrList = (List) EcoreUtils.getValue(resource, CLASS_ATTRIBUTES);
            atCount = processAttrList(attrList, attrInfoList, atMap, atCount);
            // Update for Provisioning->Attributes
            EObject prov = (EObject)
                EcoreUtils.getValue(resource, RESOURCE_PROVISIONING);
            if (prov != null) {
            attrList = (List) EcoreUtils.getValue(prov, CLASS_ATTRIBUTES);
            atCount = processAttrList(attrList, attrInfoList, atMap, atCount);
            }
            //Update the count in Resource.
            String countStr = String.valueOf(atCount);
            EcoreUtils.setValue(resourceInfo, "attributeCount", countStr);
        }
        //Update Resource Count
        String resCountStr = String.valueOf(_resMaxCount);
        EcoreUtils.setValue(_countWrapper, "resourceCount", resCountStr);
        //Save the Mapping file
        try {
            EcoreModels.save(_resource);
        } catch (Exception exception) {
            LOG.error("Exception in Saving CwKey-ID mapping file.", exception);
        }
    }
}
