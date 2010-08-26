/**
 * 
 */
package com.clovis.common.utils.ui.handler;

import java.io.File;
import java.util.Arrays;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EObject;
import org.w3c.dom.Document;
import org.w3c.dom.Node;

import com.clovis.common.utils.ClovisDomUtils;
import com.clovis.common.utils.ClovisProjectUtils;

/**
 * Conditional hidden handler for various parts of IDE.
 * 
 * @author Suraj Rajyaguru
 */
public class ConditionalHiddenHandler {

	public static final String CODEGEN_MODE_OPENCLOVIS = "openclovis";
	public static final String CODEGEN_MODE_OPENSAF = "openSAF";

	public static final String HIDDEN_FILEDS_FILE = "hiddenFields.xml";

	/**
	 * Returns conditional hidden fields for the particular entity based on
	 * code-gen mode.
	 * 
	 * @param eObj
	 * @return conditional hidden fields
	 */
	public static List<String> getHiddenFieldsForCodegenMode(EObject eObj,
			String details) {
		IProject project = ClovisProjectUtils.getSelectedProject();
		String codeGenMode = ClovisProjectUtils.getCodeGenMode(project);

		if (!codeGenMode.equals(CODEGEN_MODE_OPENCLOVIS)) {
			String hiddenFieldsFileLocation = Platform.getInstanceLocation()
					.getURL().getPath()
					+ project.getName()
					+ File.separator
					+ "codegen"
					+ File.separator
					+ codeGenMode
					+ File.separator
					+ HIDDEN_FILEDS_FILE;

			Document document = ClovisDomUtils
					.buildDocument(hiddenFieldsFileLocation);
			List<Node> nodeList = ClovisDomUtils.getNodesForPath(document
					.getDocumentElement(), details);

			if (nodeList.size() > 0) {
				String[] hiddenFields = nodeList.get(0).getTextContent().split(
						",");
				return Arrays.asList(hiddenFields);
			}
		}

		return null;
	}
}
