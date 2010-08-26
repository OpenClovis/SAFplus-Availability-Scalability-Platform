/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ClusterConfigReader.java $
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
import org.eclipse.emf.ecore.EReference;
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
public class ClusterConfigReader {

	private EPackage _clusterConfigPackage = null;

	private EClass _clusterConfigClass = null;

	private Resource _clusterConfigResource = null;

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	private static final String CLUSTERCONFIG_XML_FILE_NAME = "clusterconfig.xml";

	private static final String CLUSTERCONFIG_ECORE_FILE_NAME = "clusterConfig.ecore";
	
	private static final String EMS = "ems";

	/**
	 * constructor
	 * 
	 * @param resource -
	 *            Project Resource
	 * 
	 */
	public ClusterConfigReader(IResource resource) {
		readEcoreFiles();
		readClusterConfig(resource);

	}

	/**
	 * 
	 * Reads the Ecore File.
	 */
	private void readEcoreFiles() {
		try {
			URL clusterConfigURL = DataPlugin.getDefault().find(
					new Path("model" + File.separator
							+ CLUSTERCONFIG_ECORE_FILE_NAME));
			File ecoreFile = new Path(Platform.resolve(clusterConfigURL)
					.getPath()).toFile();
			_clusterConfigPackage = EcoreModels
					.get(ecoreFile.getAbsolutePath());
			_clusterConfigClass = (EClass) _clusterConfigPackage
					.getEClassifier("CONFIG");

		} catch (IOException ex) {
			LOG.error("Cluster config Ecore File cannot be read", ex);
		}
	}

	/**
	 * read the Node Profiles from the XMI files in the Workspace.
	 * 
	 * @param project -
	 *            Project Resource
	 */
	private void readClusterConfig(IResource project) {
		try {
			// Now get the CPM config xmi file from the node dir under project.
			String dataFilePath = project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + EMS + File.separator + CLUSTERCONFIG_XML_FILE_NAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			_clusterConfigResource = xmlFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);

			List clusterConfigList = _clusterConfigResource.getContents();
			if (clusterConfigList.isEmpty()) {
				EObject clusterConfigObj = EcoreUtils.createEObject(
						_clusterConfigClass, true);
				clusterConfigList.add(clusterConfigObj);
				EReference timerRef = (EReference) _clusterConfigClass
						.getEStructuralFeature("REFRESHTIMERS");
				EObject timerObj = (EObject) clusterConfigObj.eGet(timerRef);
				if (timerObj == null) {
					timerObj = EcoreUtils.createEObject(timerRef
							.getEReferenceType(), true);
					clusterConfigObj.eSet(timerRef, timerObj);
				}
			}

		} catch (Exception e) {
			LOG.error("Error reading Cluster config XMI File", e);
		}
	}

	/**
	 * 
	 * @return AMF config Resource
	 */
	public Resource getClusterConfigResource() {
		return _clusterConfigResource;
	}

	/**
	 * 
	 * @return AMF eclass
	 */
	public EClass getClusterConfigClass() {
		return _clusterConfigClass;
	}
}
