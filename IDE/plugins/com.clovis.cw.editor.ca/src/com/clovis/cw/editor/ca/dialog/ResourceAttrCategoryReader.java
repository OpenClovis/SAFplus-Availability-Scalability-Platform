/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ResourceAttrCategoryReader.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;

/**
 * @author shubhada
 * 
 * Node Profile Reader Class which reads the clAmfConfig.xml file.
 */
public class ResourceAttrCategoryReader {

	private EPackage _resourceAttrCategoryPackage = null;

	private EClass _categoriesClass = null;

	private Resource _categoriesResource = null;

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	private String _xmlfileName = null;

	private static final String ATTRCATEGORY_XML_FILE_NAME = "AttrCategory.xml";
	private static final String EMS = "ems";	
	private static final String RESOURCEATTRCATEGORY_ECORE_FILE_NAME = "resourceAttrCategory.ecore";

	/**
	 * constructor
	 * 
	 * @param resource -
	 *            Project Resource
	 * 
	 */
	public ResourceAttrCategoryReader(IResource resource, String resourceName) {
		readEcoreFiles();
		readResourceAttrCategory(resource, resourceName);

	}

	/**
	 * 
	 * Reads the Ecore File.
	 */
	private void readEcoreFiles() {
		try {
			URL clusterConfigURL = DataPlugin.getDefault().find(
					new Path("model" + File.separator
							+ RESOURCEATTRCATEGORY_ECORE_FILE_NAME));
			File ecoreFile = new Path(Platform.resolve(clusterConfigURL)
					.getPath()).toFile();
			_resourceAttrCategoryPackage = EcoreModels
					.get(ecoreFile.getAbsolutePath());
			_categoriesClass = (EClass) _resourceAttrCategoryPackage
					.getEClassifier("CATEGORIES");

		} catch (IOException ex) {
			LOG.error("Resource category Ecore File cannot be read", ex);
		}
	}

	/**
	 * read the Node Profiles from the XMI files in the Workspace.
	 * 
	 * @param project -
	 *            Project Resource
	 */
	private void readResourceAttrCategory(IResource project, String resourceName) {
		try {
			
			_xmlfileName = resourceName + ATTRCATEGORY_XML_FILE_NAME;
			// Now get the CPM config xmi file from the node dir under project.
			String dataFilePath = project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + EMS + File.separator + _xmlfileName;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			_categoriesResource = xmlFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);

			List attrCategoryList = _categoriesResource.getContents();
			if (attrCategoryList.isEmpty()) {
				
				EObject attrCategoryObj = EcoreUtils.createEObject(
						_categoriesClass, true);
				attrCategoryList.add(attrCategoryObj);
				
			}

		} catch (Exception e) {
			LOG.error("Error reading Resource Attribute Category XMI File", e);
		}
	}

	/**
	 * 
	 * @return AMF config Resource
	 */
	public Resource getCategoriesResource() {
		return _categoriesResource;
	}

	/**
	 * 
	 * @return AMF eclass
	 */
	public EClass getCategoriesClass() {
		return _categoriesClass;
	}
}
