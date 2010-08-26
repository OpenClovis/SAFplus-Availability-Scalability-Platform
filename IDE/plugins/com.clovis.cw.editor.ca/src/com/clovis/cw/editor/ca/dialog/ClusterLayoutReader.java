/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ClusterLayoutReader.java $
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
public class ClusterLayoutReader {

	private EPackage _clusterLayoutPackage = null;

	private EClass _clusterLayoutClass = null;

	private Resource _clusterLayoutResource = null;

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	private static final String CLUSTERLAYOUT_XML_FILE_NAME = "clusterlayout.xml";

	private static final String CLUSTERLAYOUT_ECORE_FILE_NAME = "clusterLayout.ecore";

	private static final String EMS = "ems";	
	/**
	 * constructor
	 * 
	 * @param resource -
	 *            Project Resource
	 * 
	 */
	public ClusterLayoutReader(IResource resource) {
		readEcoreFiles();
		readClusterLayout(resource);

	}

	/**
	 * 
	 * Reads the Ecore File.
	 */
	private void readEcoreFiles() {
		try {
			URL clusterLayoutURL = DataPlugin.getDefault().find(
					new Path("model" + File.separator
							+ CLUSTERLAYOUT_ECORE_FILE_NAME));
			File ecoreFile = new Path(Platform.resolve(clusterLayoutURL)
					.getPath()).toFile();
			_clusterLayoutPackage = EcoreModels
					.get(ecoreFile.getAbsolutePath());
			_clusterLayoutClass = (EClass) _clusterLayoutPackage
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
	private void readClusterLayout(IResource project) {
		try {
			// Now get the CPM config xmi file from the node dir under project.
			String dataFilePath = project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + EMS + File.separator + CLUSTERLAYOUT_XML_FILE_NAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			_clusterLayoutResource = xmlFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : EcoreModels.create(uri);

			List clusterLayoutList = _clusterLayoutResource.getContents();
			if (clusterLayoutList.isEmpty()) {
				EObject clusterLayoutObj = EcoreUtils.createEObject(
						_clusterLayoutClass, true);
				clusterLayoutList.add(clusterLayoutObj);				
				
			}

		} catch (Exception e) {
			LOG.error("Error reading Cluster config XMI File", e);
		}
	}

	/**
	 * 
	 * @return AMF config Resource
	 */
	public Resource getClusterLayoutResource() {
		return _clusterLayoutResource;
	}

	/**
	 * 
	 * @return AMF eclass
	 */
	public EClass getClusterLayoutClass() {
		return _clusterLayoutClass;
	}
}
