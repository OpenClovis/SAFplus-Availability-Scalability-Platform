package com.clovis.cw.editor.ca.manageability.common;

import java.io.IOException;
import java.util.List;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.snmp.ClovisMibUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.util.MibParseException;
import com.ireasoning.util.MibTreeNode;

/**
 * 
 * @author Pushparaj
 * Utils class to Load and UnLoad Mibs
 */
public class LoadedMibUtils {

     private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());

	/**
	 * Loaded all the mibs which are loaded in last session
	 * @param pdm
	 * @param loadedMibs
	 */
	public static void loadExistingMibs(ProjectDataModel pdm,
			List<String> loadedMibs, ResourceTypeBrowserUI treeViewer) {
		for (int i = 0; i < loadedMibs.size(); i++) {
			loadMib(pdm, loadedMibs.get(i), treeViewer);
		}
	}
	/**
	 * Set the loaded Mibs in project properties
	 * 
	 * @param pdm
	 * @param loadedMibs
	 */
	public static void setLoadedMibs(ProjectDataModel pdm, List<String> loadedMibs) {
		String mibFileNames = "";
		for(int i = 0; i < loadedMibs.size(); i++) {
			mibFileNames += loadedMibs.get(i) + ",";
		}
		try {
			if(pdm.getProject().exists())
				pdm.getProject().setPersistentProperty(new QualifiedName("", "MIB_FILE_NAMES"), mibFileNames);
		} catch (CoreException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Loads MIB and return the root Node
	 * @param pdm
	 * @param mibFileName
	 * @return
	 */
	public static MibTreeNode loadMib(ProjectDataModel pdm, String mibFileName, ResourceTypeBrowserUI treeViewer) {
		MibTreeNode node = null;
		try {
			MibUtil.unloadAllMibs();
			ClovisMibUtils.loadSystemMibs(pdm.getProject());
			MibUtil.setResolveSyntax(true);
  		        node = MibUtil.parseMib(mibFileName,false);
			if (node == null) {
				MessageDialog
						.openError(null, "Mib File Loading Error",
								"Could not load MIB file. Error occured while parsing MIB file");
			} else {
				treeViewer.buildAvailableResourceTreeForMibNode(node,
						mibFileName);
			}
		} catch (MibParseException e) {
			MessageDialog.openError(null, "One Mib File Parsing Error", e.toString());
                        LOG.error("Exception has occured" + (new Throwable().getStackTrace()).toString(), e);

		} catch (IOException e) {
			MessageDialog.openError(null,
					"Could not load the MibFile. IO Exception has occured", e
							.toString());
		} catch (Exception e) {
			MessageDialog.openError(null, "Could not load the MibFile", e
					.toString());
		}
		return node;
	}
}
