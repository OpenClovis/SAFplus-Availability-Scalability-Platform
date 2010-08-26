/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/ClovisMibUtils.java $
 * $Author: pushparaj $
 * $Date: 2007/05/17 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.math.BigInteger;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.util.MibParseException;
import com.ireasoning.util.MibTreeNode;

/**
 * @author shubhada
 *
 * Utils class which has methods for parsing mibs, converting them to EObjects
 */
public class ClovisMibUtils
{
    private static EPackage mibEPackage = null;
    private static EClass eClass = null;
    private static EPackage uiEPackage = null;
    private static EClass uiClass = null;
    
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());

    private static HashMap filehash = new HashMap();
    private static int noofnodes = 0;
    private static Shell shell = null;
    private static boolean initialized;
    private static HashMap typesMap = new HashMap();
   
	private static final String binExpression = "'[01]+'[bB]";
	private static final String hexExpression = "'[0-9a-fA-F]+'[hH]";


    //Initialize
    static {
        if (!initialized) {
            init();
        }
    }
    /**
     * Reads Ecore and UI Ecore file and data type map properties file in
     *  first call. Subsequent calls does not
     * do anything.
     */
    private static void init()
    {

        try {
            URL mibURL = DataPlugin.getDefault().find(new
                    Path("model/mib.ecore"));
            File ecoreFile = new Path(Platform.resolve(mibURL).getPath())
                    .toFile();
            mibEPackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            eClass = (EClass) mibEPackage.getEClassifier("MibNode");
            mibURL = DataPlugin.getDefault().find(new Path
                    ("model/mibdetail.ecore"));

            ecoreFile = new Path(Platform.resolve(mibURL).getPath())
                    .toFile();
            uiEPackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            uiClass = (EClass) uiEPackage.getEClassifier("MibModule");
            //read the mapping from Mib types to Clovis data types
            readDatatypes();
            initialized = true;
        } catch (IOException ex) {
            LOG.error("Ecore File cannot be read", ex);
        }
    }

    /**
     *
     * @param filenames
     *            Vector of mibfiles to be loaded after parsing mibs it merges
     *            the mibs.
     * @return EObject
     */
    public static EObject populateNode(ClovisNotifyingListImpl filenames)
    {
        MibTreeNode node = null, topnode = null;
        
        if (!filenames.isEmpty()) {
            try {
				MibUtil.unloadAllMibs();
	        	loadDefaultSystemMibs();
	        	MibUtil.setResolveSyntax(true);

            	node = MibUtil.parseMibs((String[]) filenames
                        .toArray(new String[0]));
                if (node == null) {
                	MessageDialog.openError(
                            shell, "Mib File Loading Error", "Could not load MIB file. Error occured while parsing MIB file");
                    LOG.error("Error occured while parsing MIB file");
                    return null;
                } else {
                	topnode = node.findChildNode(".1");
                }
            } catch (MibParseException e) {
                MessageDialog.openError(
                        shell, "Mib File Loading Error", e.toString());
                LOG.error("Error occured while parsing MIB file", e);
                return null;
            } catch (IOException e) {
                MessageDialog.openError(shell, "Mib File Loading Error",
                        "Could not load the MibFile. IO Exception has occured" + e);
                LOG.error("IO Exception has occured", e);
                return null;
            } catch (Exception e) {
            	MessageDialog.openError(shell, "Mib File Loading Error",
                        "Could not load the MibFile. Exception has occured" + e);
                LOG.error("Exception has occured", e);
                return null;
            }
            return convertToEobject(topnode);
        } else {
            EObject topObj = EcoreUtils.createEObject(eClass, true);
            EcoreUtils.setValue(topObj, "Name", "MibTreeNode(No Mibs Loaded)");
            return topObj;
        }
    }

    /**
     *
     * @param filename
     *            Filename
     * @return the EObject corresponding to the MibTreeNode parsed from file.
     *
     */
    public static EObject populateUINode(String filename)
    {
        MibTreeNode node = null;
        EObject uiObj = null;
        try {
			MibUtil.unloadAllMibs();
        	loadDefaultSystemMibs();
        	MibUtil.setResolveSyntax(true);

        	node = MibUtil.parseMib(filename);
            if (node == null) {
            	MessageDialog.openError(
                        null, "Mib File Loading Error", "Could not load MIB file. Error occured while parsing MIB file");
                LOG.error("Error occured while parsing MIB file");
            } 
            uiObj = convertToUiObject(node);
            /*
             * Adds the filenames and corresponding EObjects to hashmap. We need the
             * corresponding filename when Unload action has to be performed on a
             * EObject in the Tree.
             *
             */
            if (!filehash.containsKey(uiObj)
                    && !filehash.containsValue(filename)) {
                filehash.put(uiObj, filename);
            } else if (filehash.containsValue(filename)) {
                return null;
            }
            EcoreUtils.setValue(uiObj, "MibPath", filename);
            EcoreUtils.setValue(uiObj, "MibSize", (new Long((new File(
                filename)).length())).toString());
        
        } catch (MibParseException e) {
            LOG.error("Error occured while parsing MIB file", e);
            return null;
        } catch (IOException e) {
             LOG.error("IO Exception has occured", e);
             return null;
        }
        return uiObj;
    }
   

    /**
     * Maps the MibTreeNode attributes to UI EObject Eattributes.
     *
     * @param mnode
     *            MibTreeNode
     * @return EObject
     */
    public static EObject convertToUiObject(MibTreeNode mnode)
    {
        EObject uiObj = EcoreUtils.createEObject(uiClass, false);
        EcoreUtils.setValue(uiObj, "MibName", mnode.getModuleName());
        EcoreUtils.setValue(uiObj, "Description", mnode.getDescription());
        noofnodes = 0;
        int nodecount = getChildnodeCount(mnode);
        EcoreUtils.setValue(uiObj, "NoOfNodes", (new Integer(nodecount))
                .toString());

        return uiObj;
    }

    /**
     *
     * @param mnode
     *            MibTreeNode
     * @return the total count of childnodes
     */
    private static int getChildnodeCount(MibTreeNode mnode)
    {
        if (mnode.getChildNodeCount() > 0) {
            for (int i = 0; i < mnode.getChildNodeCount(); i++) {
                noofnodes = noofnodes + mnode.getChildNodeCount();
                getChildnodeCount((MibTreeNode) mnode.getChildNode(i));
            }
        }
        return noofnodes;
    }

    /**
     * Maps the MibTreeNode attributes to EObject Eattributes.
     *
     * @param mnode
     *            MibTreeNode
     * @return EObject
     */
    public static EObject convertToEobject(MibTreeNode mnode)
    {
        EObject object = EcoreUtils.createEObject(eClass, true);

        EcoreUtils.setValue(object, "Name", (String) mnode.getName());
        EcoreUtils.setValue(object, "FullName", mnode.getFullName());
        EcoreUtils.setValue(object, "Description", mnode.getDescription());
        EcoreUtils.setValue(object, "Access", mnode.getAccess());
        EcoreUtils.setValue(object, "Auguments", mnode.getAugments());
        EcoreUtils.setValue(object, "DefaultVal", mnode.getDefVal());
        String[] indice = mnode.getIndice();
        if (indice != null) {
            for (int i = 0; i < indice.length; i++) {
                EList nodeIndice = (EList) object.eGet(eClass
                        .getEStructuralFeature("Indice"));
                nodeIndice.add(indice[i]);
            }
        }
        EcoreUtils
                .setValue(object, "ModuleIdentity", mnode.getModuleIdentity());
        EcoreUtils.setValue(object, "ModuleName", mnode.getModuleName());
        indice = mnode.getObjects();
        if (indice != null) {
            for (int i = 0; i < indice.length; i++) {
                EList nodes = (EList) object.eGet(eClass
                        .getEStructuralFeature("Nodes"));
                nodes.add(indice[i]);
            }
        }
        indice = mnode.getObjectsOIDs();
        if (indice != null) {
            for (int i = 0; i < indice.length; i++) {
                EList nodeOids = (EList) object.eGet(eClass
                        .getEStructuralFeature("NodeOIDs"));
                nodeOids.add(indice[i]);
            }
        }

        EcoreUtils.setValue(object, "OID", mnode.getOID().toString());
        if (mnode.getRowStatusOID() != null) {
            EcoreUtils.setValue(object, "RowStatusOID", mnode.getRowStatusOID()
                    .toString());
        }
        EcoreUtils.setValue(object, "Status", mnode.getStatus());
        if (mnode.getSyntax() != null) {
            EcoreUtils.setValue(object, "Syntax", mnode.getSyntax().toString());
            if(mnode.getSyntax().getSize() != null) {
            	EcoreUtils.setValue(object, "Size", mnode.getSyntax().getSize().toString());
            }
        }
        EcoreUtils.setValue(object, "SyntaxType", mnode.getSyntaxType());
        indice = mnode.getTableIndice();
        if (indice != null) {
            for (int i = 0; i < indice.length; i++) {
                EList tableIndice = (EList) object.eGet(eClass
                        .getEStructuralFeature("TableIndice"));
                tableIndice.add(indice[i]);
            }
        }

        Vector trapNodes = mnode.getTrapNodes();
        BasicEList elist = new BasicEList();
        if (trapNodes != null) {
            for (int i = 0; i < trapNodes.size(); i++) {
                elist.add(trapNodes.get(i));
            }
        }

        object.eSet(eClass.getEStructuralFeature("TrapNodes"), elist);
        EcoreUtils.setValue(object, "IsDynamicTable", new Boolean(mnode
                .isDynamicTable()).toString());
        EcoreUtils.setValue(object, "IsEntryStatus", new Boolean(mnode
                .isEntryStatus()).toString());
        EcoreUtils.setValue(object, "IsGroupNode", new Boolean(mnode
                .isGroupNode()).toString());
        EcoreUtils.setValue(object, "IsImplied", new Boolean(mnode.isImplied())
                .toString());
        EcoreUtils.setValue(object, "IsIndexNode", new Boolean(mnode
                .isIndexNode()).toString());
        EcoreUtils.setValue(object, "IsScalarNode", new Boolean(mnode
                .isScalarNode()).toString());
        EcoreUtils.setValue(object, "IsSnmpV2TrapNode", new Boolean(mnode
                .isSnmpV2TrapNode()).toString());
        EcoreUtils.setValue(object, "IsTableColumnNode", new Boolean(mnode
                .isTableColumnNode()).toString());
        EcoreUtils.setValue(object, "IsTableEntryNode", new Boolean(mnode
                .isTableEntryNode()).toString());
        EcoreUtils.setValue(object, "IsTableLeafNode", new Boolean(mnode
                .isTableLeafNode()).toString());
        EcoreUtils.setValue(object, "IsTableNode", new Boolean(mnode
                .isTableNode()).toString());
        EcoreUtils.setValue(object, "IsLeafMibNode",
                new Boolean(mnode.isLeaf()).toString());
        EReference childRef = (EReference) eClass
                .getEStructuralFeature("ChildNode");
        EList children = (EList) object.eGet(childRef);
        for (int i = 0; i < mnode.getChildNodeCount(); i++) {
            children.add(convertToEobject((MibTreeNode) mnode.getChildNode(i)));
        }
        //object.eSet(childRef, children);
        return object;
    }

    /**
     * returns the filename corresponding to EObject
     *
     * @param eobj
     *            EObject
     * @return filename String
     */
    public static String getFileName(EObject eobj)
    {
        return (String) filehash.get(eobj);
    }

    /**
     * returns the EObject corresponding to filename
     *
     * @param filename
     *            String
     * @return EObject
     */
    public static EObject getEObject(String filename)
    {
        Iterator iterator = filehash.keySet().iterator();
        while (iterator.hasNext()) {
            EObject eobj = (EObject) iterator.next();
            String fname = (String) filehash.get(eobj);
            if (fname.equals(filename)) {
                return eobj;
            }
        }
        return null;
    }

    /**
     *
     * @return Eclass corresponding to UI ecore file
     */
    public static EClass getUiECLass()
    {
        return uiClass;
    }
    /**
     *
     * @param parentshell Shell
     */
    public static void setShell(Shell parentshell)
    {
        shell = parentshell;
    }
    /**
     *
     * reads the properties file which holds the mapping of Mib datatypes
     * to clovis supported data types.
     */
    public static void readDatatypes()
    {
        NotifyingList dataTypesList = null;
        URL url = DataPlugin.getDefault().getBundle().getEntry("/");
        try {
            url = Platform.resolve(url);
            String fileName = url.getFile() + "xml" + File.separator
            + "datatypesmap.xmi";
            URI uri = URI.createFileURI(fileName);
            dataTypesList = (NotifyingList) EcoreModels.read(uri);
        } catch (IOException e) {
            LOG.error("Error reading Data Types Map File", e);
        }
        for (
          int i = 0; dataTypesList != null && i < dataTypesList.size(); i++) {
            EObject eobj = (EObject) dataTypesList.get(i);
            String snmpType = (String) EcoreUtils.getValue(eobj, "SnmpType");
            EObject cTypeObj = (EObject) EcoreUtils.getValue(eobj, "ClovisType");
            if (!typesMap.containsKey(snmpType)) {
                typesMap.put(snmpType, cTypeObj);
            }

        }

    }
    /**
     *
     * @param mibtype -Mib data type
     * @return Corresponding Clovis Data type.
     */
    public static EObject getClovisDatatype(String mibtype)
    {
    	Iterator iterator = typesMap.keySet().iterator();
    	while (iterator.hasNext()) {
    		String snmpType = (String) iterator.next();
    		if (mibtype.indexOf(snmpType)!= -1) {
    			return (EObject) typesMap.get(snmpType);
    		}
    	}
        return null;
    }
    /**
     * Remove the object from mao when file is unloaded.Fix for bug#3238
     * @param fileName
     */
    public static void updateMap(String fileName)
    {
        EObject eobj = getEObject(fileName);
        filehash.remove(eobj);
    }

    /**
     * Fetches the name of each valid table/group/notification from the mib
     * 
     * @param node - MibTreeNode
     * @param project - IProject
     * @param oidObjMap - Map of table/group/notification oid and names in the MIB
     */
    public static void getMibObjects(IProject project, MibTreeNode node,
    		List mibNodesList)
    {
    	if (node != null) {
    		if(node.getStatus() != null && (node.getStatus().equals("obsolete") || node.getStatus().equals("deprecated"))) {
    			// No need to parse depricated / obsolete;
    		} else if (!isDuplicateResource((String) node.getName(), project)
	    			&& node.isTableNode()) {
	    		mibNodesList.add(node);
	    	} else if (!isDuplicateResource((String) node.getName(), project)
	    			&& node.isGroupNode()) {
	    		// check whether this node has any scalar objects
	    		boolean isGroupWithScalar = false;
	    		List children = node.getChildNodes();
	    		for (int i = 0; i < children.size(); i++) {
	    			MibTreeNode child = (MibTreeNode) children.get(i);
	    			if (child.isScalarNode()) {
	    				isGroupWithScalar = true;
	    				break;
	    			}
	    		}
	    		if (isGroupWithScalar) {
	    			mibNodesList.add(node);
	    		}
	    	} else if (!isDuplicateAlarm((String) node.getName(), project) && node.isSnmpV2TrapNode()) {
	    		mibNodesList.add(node);
	    	}
    	}
    	List children = node.getChildNodes();
		for (int i = 0; i < children.size(); i++) {
			MibTreeNode child = (MibTreeNode) children.get(i);
			getMibObjects(project, child, mibNodesList);
		}
    	
    }
    /**
     * 
     * @param alarmName - Alarm ID to be checked for duplicate
     * @param project - IProject
     * @return true if alarmName is duplicate  else return false
     */
	public static boolean isDuplicateAlarm(String alarmName, IProject project)
	{
		ProjectDataModel pdm = ProjectDataModel.
			getProjectDataModel(project);
		Model alarmModel = pdm.getAlarmProfiles();
		EObject alarmInfoObj = (EObject) alarmModel.getEList().get(0);
		EReference alarmProfileRef = (EReference) alarmInfoObj.eClass().
			getEStructuralFeature("AlarmProfile");
		List alarmProfiles = (List) alarmInfoObj.eGet(alarmProfileRef);
		for (int i = 0; i < alarmProfiles.size(); i++) {
			EObject alarm = (EObject) alarmProfiles.get(i);
			String alarmId = EcoreUtils.getValue(alarm, "alarmID").toString();
			if (alarmId.equals(alarmName)) {
				return true;
			}
		}
		
		return false;
	}
	/**
	 * 
	 * @param mibObjName - MIB object name to be checked for duplicate
	 * @param project - IProject
     * @return true if mibObjName is duplicate  else return false
	 */
	public static boolean isDuplicateResource(String mibObjName, IProject project)
	{
		ProjectDataModel pdm = ProjectDataModel.
			getProjectDataModel(project);
		Model caModel = pdm.getCAModel();
		List resourceList = ResourceDataUtils.
			getMoList(caModel.getEList());
		for (int i = 0; i < resourceList.size(); i++) {
			EObject resource = (EObject) resourceList.get(i);
			if (mibObjName.equals(EcoreUtils.getName(resource))) {
				return true;
			}
		}
		
		return false;
	}
	/**
	 * Sets Default properties based on writable permissions
	 * @param isIndexed true/false
	 * @param access writable permissions
	 * @param eObject Mib Attribute Object 
	 */
	public static void setDefaultPropertiesForMibAttributes(boolean isIndexed, String access,
			EObject eObject) {
		if (isIndexed) {
			EcoreUtils.setValue(eObject, "type", "CONFIG");
			EcoreUtils.setValue(eObject, "initialized", "true");
			EcoreUtils.setValue(eObject, "writable", "true");
			EcoreUtils.setValue(eObject, "caching", "true");
			EcoreUtils.setValue(eObject, "persistency", "true");
		} else {
			if (access.equals("read-only")) {
				EcoreUtils.setValue(eObject, "type", "RUNTIME");
				EcoreUtils.setValue(eObject, "caching", "false");
				EcoreUtils.setValue(eObject, "persistency", "false");

				EcoreUtils.setValue(eObject, "writable", "false");
				EcoreUtils.setValue(eObject, "initialized", "false");
			} else {
				EcoreUtils.setValue(eObject, "type", "CONFIG");
				EcoreUtils.setValue(eObject, "writable", "true");
				EcoreUtils.setValue(eObject, "initialized", "false");

				EcoreUtils.setValue(eObject, "caching", "true");
				EcoreUtils.setValue(eObject, "persistency", "true");
			}
		}
	}
	
	/**
	 * Sets type and array size for mib attr
	 * @param mibType Type
	 * @param mibCoreType Type
	 * @param size array size
	 * @param valueEnums Possible values for attribute
	 * @param defaultVal Default value for attribute
	 * @param eObject Mib attribute Object
	 */
	public static void setTypeAndSize(String mibType, byte mibCoreType, String size, Hashtable valueEnums, String defaultVal, EObject eObject)
	{
		boolean isTypeSet = false;
		EEnum typeEnum = ((EEnumLiteral) EcoreUtils.getValue(eObject,
				ClassEditorConstants.ATTRIBUTE_TYPE)).getEEnum();
		String uTypeName = mibType.toUpperCase();

		if (uTypeName.equals("INTEGER32"))
			mibType = "Int32";
		else if (uTypeName.equals("UNSIGNED32"))
			mibType = "Uint32";
		else if (uTypeName.equals("COUNTER"))
			mibType = "COUNTER32";

		uTypeName = mibType.toUpperCase();
		String typeName = "";
		List literals = typeEnum.getELiterals();

		for (int j = 0; j < literals.size(); j++) {
			EEnumLiteral typeLiteral = (EEnumLiteral) literals.get(j);
			typeName = typeLiteral.getName();

			if (uTypeName.equals(typeName.toUpperCase())) {
				isTypeSet = true;
				break;
			}
		}

		if (!isTypeSet) {
			String basicMibType = getBasicMibType(mibCoreType);
			typeName = EcoreUtils.getValue(getClovisDatatype(basicMibType),
					"AttributeType").toString();
			isTypeSet = true;
		}

		if (isTypeSet) {
			EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_TYPE,
					typeName);

			BigInteger multiplicity = null;
			BigInteger minVal = null;
			BigInteger maxVal = null;

			List<BigInteger> minMaxVals = getMinMaxVals(size, valueEnums);
			minVal = minMaxVals.get(0);
			maxVal = minMaxVals.get(1);

			if (mibCoreType == MibTreeNode.SYN_STRING
					|| mibCoreType == MibTreeNode.SYN_OPAQUE) {
				multiplicity = maxVal;
				maxVal = BigInteger.ZERO;
				minVal = BigInteger.ZERO;
			}

			if (multiplicity == null) {

				switch (mibCoreType) {
				case MibTreeNode.SYN_STRING:
					multiplicity = new BigInteger("255");
					break;
				case MibTreeNode.SYN_OID:
					multiplicity = new BigInteger("512");
					break;
				case MibTreeNode.SYN_OPAQUE:
					multiplicity = new BigInteger("65535");
					break;
				default:
					multiplicity = new BigInteger("1");
					break;
				}
			}

			if (minVal == null)
				minVal = getDefaultMinValue(typeName);
			if (maxVal == null)
				maxVal = getDefaultMaxValue(typeName);

			BigInteger defVal = minVal;
			if (defaultVal != null) {
				try {
					defVal = new BigInteger(defaultVal);
				} catch (Exception e) {
					// ignore...default value will be equal to minimum value
				}
			}

			EcoreUtils.setValue(eObject,
					ClassEditorConstants.ATTRIBUTE_MIN_VALUE, String
							.valueOf(minVal));
			EcoreUtils.setValue(eObject,
					ClassEditorConstants.ATTRIBUTE_MAX_VALUE, String
							.valueOf(maxVal));
			EcoreUtils.setValue(eObject,
					ClassEditorConstants.ATTRIBUTE_MULTIPLICITY, String
							.valueOf(multiplicity));
			EcoreUtils.setValue(eObject,
					ClassEditorConstants.ATTRIBUTE_DEFAULT_VALUE, String
							.valueOf(defVal));

		} else {
			LOG.error("Invalid MIB type: [" + mibType + "]");
		}
	}

	/**
	 * Returns the List of locations for the system wide MIBs (Usually net-snmp
	 * and hpi Mibs)
	 * 
	 * @param project
	 * @return
	 */
	public static List<String> getSystemMibLocations(IProject project) {
		ArrayList<String> mibLocationList = new ArrayList<String>();

		try {
			String sdkLoc = project.getPersistentProperty(
			        new QualifiedName("", "SDK_LOCATION"));

			if(sdkLoc != null && !sdkLoc.equals("")) {
				String installLoc = new File(sdkLoc).getParentFile().getAbsolutePath();
				String mibPathSDK = installLoc + File.separator + "buildtools" + File.separator + "local" + File.separator + "share" + File.separator + "snmp" + File.separator + "mibs";

				if(new File(mibPathSDK).exists()) {
					mibLocationList.add(mibPathSDK);
				}
				
				// check for an alternative mib path and add it to our path if different
				String mibPathNonSDK = checkForAlternateMibPath(installLoc);
				if (mibPathNonSDK != null && mibPathNonSDK.length() > 0 && !mibPathNonSDK.equals(mibPathSDK))
				{
					mibLocationList.add(0, mibPathNonSDK);
				}
			}
		} catch (CoreException e) {
			LOG.error("Error resolving mib locations.", e);
		}
		return mibLocationList;
	}

	/**
	 * Returns the List of default locations for the system wide MIBs (Usually
	 * net-snmp and hpi Mibs)
	 * 
	 * @return
	 */
	public static List<String> getDefaultSystemMibLocations() {
		ArrayList<String> mibLocationList = new ArrayList<String>();

		String sdkLoc = DataPlugin.getDefaultSDKLocation();

		if (sdkLoc != null && !sdkLoc.equals("")) {
			String installLoc = new File(sdkLoc).getParentFile()
					.getAbsolutePath();
			String mibPathSDK = installLoc + File.separator + "buildtools"
					+ File.separator + "local" + File.separator + "share"
					+ File.separator + "snmp" + File.separator + "mibs";

			if (new File(mibPathSDK).exists()) {
				mibLocationList.add(mibPathSDK);
			}

			// check for an alternative mib path and add it to our path if
			// different
			String mibPathNonSDK = checkForAlternateMibPath(installLoc);
			if (mibPathNonSDK != null && mibPathNonSDK.length() > 0
					&& !mibPathNonSDK.equals(mibPathSDK)) {
				mibLocationList.add(0, mibPathNonSDK);
			}
		}
		return mibLocationList;
	}

	/**
	 * Checks for and returns the configured SNMP config directory. We need to do this
	 * because on systems where net-snmp is already installed the shared mib files may
	 * be in a location other than the ASP install directory tree. For example this is
	 * true on red hat installations.
	 * 
	 * The proper way to check this is to run the net-snmp-config command in the 
	 * <asp-install-dir>/buildtools/local/bin directory if it exists. Otherwise run it
	 * and let the path determine which version to run.
	 * 
	 * @param installLocation
	 * @return
	 */
	public static String checkForAlternateMibPath(String installLocation)
	{
		String altPath = new String();

		String binDir = installLocation + File.separator + "buildtools" + File.separator + "local" + File.separator + "bin";
		
		try {
			
			String command = "net-snmp-config";
			
			// The is a known bug that modifying the path environment variable ProcessBuilder
			//  environment map does not work properly. As a workaround we check for the existence
			//  of net-snmp-config in out install directory and run it explicitly if it exists.
			File clovisNSC = new File(binDir + File.separator + command);
			if (clovisNSC.exists())
			{
				command = binDir + File.separator + command;
			}
			
			List<String> commandList = new ArrayList<String>();
			
			commandList.add(command);
			commandList.add("--prefix");
			
			ProcessBuilder processBuilder = new ProcessBuilder(commandList);

			Process process = processBuilder.start();
			
			InputStream is = process.getInputStream();
			InputStreamReader isr = new InputStreamReader(is);
			BufferedReader br = new BufferedReader(isr);
			String line;
			List<String> commandOutput = new ArrayList<String>();

			while ((line = br.readLine()) != null) {
				commandOutput.add(line);
			}
			
			if (commandOutput.size() == 1)
			{
				String snmpPath = commandOutput.get(0).toString().trim();
				snmpPath = snmpPath + File.separator + "share" + File.separator + "snmp" + File.separator + "mibs";
				File test = new File(snmpPath);
				if (test.exists() && test.isDirectory())
				{
					altPath = snmpPath;
				}
			}

		} catch (IOException e) {
			// indicates no alternate directory found...eat the error
		}

		return altPath;
	}
	
	/**
	 * Returns the minumum and maximum values for the attribute. In the case of a
	 * string attribute the values returned will be the minimum and maximum length
	 * of the string.
	 * @param size Mib size specification
	 * @param valueEnums Mib define possible values for attribute
	 * @return
	 */
	private static List<BigInteger> getMinMaxVals(String size, Hashtable valueEnums)
	{
		List<BigInteger> minMaxVals = new ArrayList<BigInteger>(2);
		BigInteger minValue = null;
		BigInteger maxValue = null;
		
		if (size != null)
		{
			size = size.trim();
			int sizeIndex = size.indexOf("SIZE");
			if (sizeIndex != -1)
			{
				size = size.substring(sizeIndex + 4).trim();
			}
			while (size.startsWith("(")) {
				size=size.substring(1);
			}
			while (size.endsWith(")")) size=size.substring(0,size.length()-1);
	
			//check for 'or' values
			if (size.contains("|"))
			{
				StringTokenizer tokenizer = new StringTokenizer(size, "|");
				while (tokenizer.hasMoreTokens())
				{
					String value = tokenizer.nextToken();
					if (value.contains("."))
					{
						List<BigInteger> tempVals = getRangeMinMax(value);
						minValue = (minValue == null) ? tempVals.get(0) : minValue.min(tempVals.get(0));
						maxValue = (maxValue == null) ? tempVals.get(1) : maxValue.max(tempVals.get(1));
					} else {
						try
						{
							BigInteger biValue = new BigInteger(normalizeValue(value.trim()));
							minValue = (minValue == null) ? biValue : minValue.min(biValue);
							maxValue = (maxValue == null) ? biValue : maxValue.max(biValue);
						} catch (NumberFormatException e) {
	
						}
					}
				}
			} else if (size.contains(".")) {
				List<BigInteger> tempVals = getRangeMinMax(size);
				minValue = (minValue == null) ? tempVals.get(0) : minValue.min(tempVals.get(0));
				maxValue = (maxValue == null) ? tempVals.get(1) : maxValue.max(tempVals.get(1));
			} else {
				try {
					BigInteger biValue = new BigInteger(normalizeValue(size.trim()));
					maxValue = (maxValue == null) ? biValue : maxValue.max(biValue);
					minValue = (minValue == null) ? biValue : minValue.min(biValue);
				} catch (NumberFormatException e) {
	
				}
			}
		}
		
		if (valueEnums != null)
		{
			Iterator iter = valueEnums.keySet().iterator();
			while (iter.hasNext())
			{
				BigInteger biValue = new BigInteger(normalizeValue(iter.next().toString()));
				maxValue = (maxValue == null) ? biValue : maxValue.max(biValue);
				minValue = (minValue == null) ? biValue : minValue.min(biValue);
			}

		}

		minMaxVals.add(0, minValue);
		minMaxVals.add(1, maxValue);
		return minMaxVals;
	}
	
	/**
	 * Takes a string value that may represent a number in the Mib
	 * define binary or hex representation and returns it in an
	 * integer representation.
	 * @param value
	 * @return
	 */
	private static String normalizeValue(String value)
	{
		value = value.trim();
		String normalized = value;
		
		Pattern binPattern = Pattern.compile(binExpression);
		Matcher matcher = binPattern.matcher(value);
		if (matcher.matches())
		{
			String binString = value.substring(matcher.start()+1, matcher.end()-2);
			normalized = String.valueOf(Integer.parseInt(binString, 2));
		}

		Pattern hexPattern = Pattern.compile(hexExpression);
		matcher = hexPattern.matcher(value);
		if (matcher.matches())
		{
			String hexString = value.substring(matcher.start()+1, matcher.end()-2);
			normalized = String.valueOf(Integer.parseInt(hexString, 16));
		}

		return normalized;
	}

	/**
	 * Given a string representing a numeric range in Mib format (e.g. NNN.NNN)
	 * returns the minimum and maximum values of the range. 
	 * @param size
	 * @return
	 */
	private static List<BigInteger> getRangeMinMax(String size)
	{
		List<BigInteger> minMaxVals = new ArrayList<BigInteger>(2);
		BigInteger minValue = null;
		BigInteger maxValue = null;
		
		size = size.replace("..", ".");

		String[] vals = size.split("\\.");
		for (int i=0; i<vals.length; i++)
		{
			BigInteger biValue = new BigInteger(normalizeValue(vals[i].trim()));
			maxValue = (maxValue == null) ? biValue : maxValue.max(biValue);
			minValue = (minValue == null) ? biValue : minValue.min(biValue);
		}

		minMaxVals.add(0, minValue);
		minMaxVals.add(1, maxValue);
		return minMaxVals;
	}

	/**
	 * Returns the default minimum value for a Mib data type.
	 * @param type Type of Mib attribute
	 * @return
	 */
	private static BigInteger getDefaultMinValue(String type)
	{
		String uType = type.toUpperCase();
		BigInteger minVal = new BigInteger("0");
		if (uType.equals("INTEGER") || uType.equals("INT32") || uType.equals("INTEGER32")) {
			minVal = new BigInteger("-2147483648");
		} else if (uType.equals("INT8")) {
			minVal = new BigInteger("-128");
		} else if (uType.equals("UINT8")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("INT16")) {
			minVal = new BigInteger("-32768");
		} else if (uType.equals("UINT16")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("UINT32")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("INT64")) {
			minVal = new BigInteger("-9223372036854775808");
		} else if (uType.equals("UINT64")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("UNSIGNED32")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("GAUGE32")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("COUNTER32")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("COUNTER64")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("TIMETICKS")) {
			minVal = new BigInteger("0");
		} else if (uType.equals("BITS")) {
			minVal = new BigInteger("-2147483648");
		} else if (uType.equals("TRUTHVALUE")) {
			minVal = new BigInteger("0");
		}
		
		return minVal;
	}

	/**
	 * Returns the default maximum value for a Mib data type.
	 * @param type Type of Mib attribute
	 * @return
	 */
	private static BigInteger getDefaultMaxValue(String type)
	{
		String uType = type.toUpperCase();
		BigInteger maxVal = new BigInteger("0");
		if (uType.equals("INTEGER") || uType.equals("INT32") || uType.equals("INTEGER32")) {
			maxVal = new BigInteger("2147483647");
		} else if (uType.equals("INT8")) {
			maxVal = new BigInteger("127");
		} else if (uType.equals("UINT8")) {
			maxVal = new BigInteger("255");
		} else if (uType.equals("INT16")) {
			maxVal = new BigInteger("32767");
		} else if (uType.equals("UINT16")) {
			maxVal = new BigInteger("65535");
		} else if (uType.equals("UINT32")) {
			maxVal = new BigInteger("4294967295");
		} else if (uType.equals("INT64")) {
			maxVal = new BigInteger("9223372036854775807");
		} else if (uType.equals("UINT64")) {
			maxVal = new BigInteger("18446744073709551615");
		} else if (uType.equals("UNSIGNED32")) {
			maxVal = new BigInteger("4294967295");
		} else if (uType.equals("GAUGE32")) {
			maxVal = new BigInteger("4294967295");
		} else if (uType.equals("COUNTER32")) {
			maxVal = new BigInteger("4294967295");
		} else if (uType.equals("COUNTER64")) {
			maxVal = new BigInteger("18446744073709551615");
		} else if (uType.equals("TIMETICKS")) {
			maxVal = new BigInteger("4294967295");
		} else if (uType.equals("BITS")) {
			maxVal = new BigInteger("2147483647");
		} else if (uType.equals("TRUTHVALUE")) {
			maxVal = new BigInteger("255");
		}
		
		return maxVal;
	}

	/**
	 * Return the basic mib type
	 * 
	 * @param mibCoreType
	 * @return
	 */
	public static String getBasicMibType(byte mibCoreType) {
		String mibType = "";

		switch (mibCoreType) {
		case MibTreeNode.SYN_BITS:
			mibType = "BITS";
			break;
		case MibTreeNode.SYN_COUNTER:
			mibType = "Counter32";
			break;
		case MibTreeNode.SYN_COUNTER64:
			mibType = "Counter64";
			break;
		case MibTreeNode.SYN_GAUGE:
			mibType = "Gauge32";
			break;
		case MibTreeNode.SYN_INTEGER:
			mibType = "INTEGER";
			break;
		case MibTreeNode.SYN_IPADDRESS:
			mibType = "IpAddress";
			break;
		case MibTreeNode.SYN_OID:
			mibType = "OBJECT IDENTIFIER";
			break;
		case MibTreeNode.SYN_OPAQUE:
			mibType = "Opaque";
			break;
		case MibTreeNode.SYN_STRING:
			mibType = "OCTET STRING";
			break;
		case MibTreeNode.SYN_TIMETICKS:
			mibType = "TimeTicks";
			break;
		case MibTreeNode.SYN_UNSIGNED:
			mibType = "Unsigned32";
			break;
		}

		return mibType;
	}

	/**
	 * Loads System mibs for the given project.
	 */
	public static void loadSystemMibs(IProject project) {
		loadMibDirs(getSystemMibLocations(project));
	}

	/**
	 * Loads default set of system mibs.
	 */
	public static void loadDefaultSystemMibs() {
		loadMibDirs(getDefaultSystemMibLocations());
	}

	/**
	 * Loads mibs from the given dir list.
	 * 
	 * @param mibDirList
	 */
	public static void loadMibDirs(List<String> mibDirList) {
		Iterator<String> mibDirs = mibDirList.iterator();
		String mibDir, file, files[];

		while (mibDirs.hasNext()) {
			mibDir = mibDirs.next();

			if (new File(mibDir).exists()) {
				files = new File(mibDir).list();

				for (int i = 0; i < files.length; i++) {
					file = mibDir + File.separator + files[i];

					if (new File(file).isFile()) {

						try {
							MibUtil.parseMib(file);
							return;
						} catch (Exception e) {
						}
					}
				}
			}
		}
	}
}
