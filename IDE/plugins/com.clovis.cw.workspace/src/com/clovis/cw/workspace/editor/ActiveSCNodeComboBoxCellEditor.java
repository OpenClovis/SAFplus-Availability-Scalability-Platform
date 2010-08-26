/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/DataTypesComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.editor;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.CustomComboBoxCellEditor;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.action.CommonMenuAction;

/**
 * @author Pushparaj
 * Combo Box cell editor for Active SC Node name
 */
public class ActiveSCNodeComboBoxCellEditor extends CustomComboBoxCellEditor
{
    private Environment _env = null;
    /**
     * @param parent - parent Composite
     * @param feature - EStructuralFeature
     * @param env - Environment
     */
    public ActiveSCNodeComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, feature, env);
        _env = env;
    }
    /**
     * Create Editor Instance.
     * @param parent  Composite
     * @param feature EStructuralFeature
     * @param env     Environment
     * @return cell Editor
     */
    public static CellEditor createEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        return new ActiveSCNodeComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
    	List comboValues = new Vector();
    	EObject gmsObj = (EObject)_env.getValue("model");
    	EPackage pack = gmsObj.eClass().getEPackage();
    	EEnum nodeNames = (EEnum)pack.getEClassifier("SCNodeNameEnum");
        EList nodeNameList = nodeNames.getELiterals();
        
        for (int i = 0; i < nodeNameList.size(); i++) {
            EEnumLiteral enumLiteral = (EEnumLiteral) nodeNameList.get(i);
            comboValues.add(enumLiteral.toString());            
        }
        IProject project = CommonMenuAction.getProject();
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
		List nodeList = ComponentDataUtils.getNodesList(pdm.getComponentModel()
				.getEList());
		List scNodeList = getSystemControllerNodes(nodeList);
		List scNodeInstanceList = pdm.getNodeInstListFrmNodeProfile(pdm.getNodeProfiles().getEList());
		comboValues.addAll(getSCNodeInstanceNames(scNodeInstanceList, scNodeList));
		String[] values = new String[comboValues.size()];
		for (int i = 0; i < comboValues.size(); i++) {
			values[i] = (String) comboValues.get(i);
		}
		
		return values;
    }
    private List getSystemControllerNodes(List nodeList) {
    	List scNodes = new ArrayList();
    	for (int i = 0; i < nodeList.size(); i++) {
    		EObject eobj = (EObject)nodeList.get(i);
    		String classType = String.valueOf(EcoreUtils.getValue(eobj, "classType"));
    		if(classType.equals("CL_AMS_NODE_CLASS_B")){
    			scNodes.add(EcoreUtils.getName(eobj));
    		}
    	}
    	return scNodes;
    }
    private List getSCNodeInstanceNames(List nodeInstanceList, List nodeList) {
    	List scNodeInstances = new ArrayList();
    	for (int i = 0; i < nodeList.size(); i++) {
    		String nodeName = (String) nodeList.get(i);
    		for (int j = 0; j < nodeInstanceList.size(); j++) {
    			EObject nodeInstanceObj = (EObject) nodeInstanceList.get(j);
    			String nodeType = (String)EcoreUtils.getValue(nodeInstanceObj, "type");
    			if(nodeName.equals(nodeType)) {
    				scNodeInstances.add(EcoreUtils.getName(nodeInstanceObj));
    			}
    		}
    	}
    	return scNodeInstances;
    }
}
