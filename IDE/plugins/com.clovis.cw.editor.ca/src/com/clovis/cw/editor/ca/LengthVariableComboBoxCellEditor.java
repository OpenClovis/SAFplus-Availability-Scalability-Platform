/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/LengthVariableComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.editor.ca.dialog.RMDDialog;

/**
 * @author shubhada
 *
 * Custom ComboBox Cell Editor for Length Variable
 */
public class LengthVariableComboBoxCellEditor extends CustomComboBoxCellEditor
{
	public static final String NONE = "None"; 
	
	private Environment _env = null;
    /**
     * @param parent - parent Composite
     * @param feature - EStructuralFeature
     * @param env - Environment
     */
    public LengthVariableComboBoxCellEditor(Composite parent,
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
        return new LengthVariableComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        Vector<String> comboValues = new Vector<String>();
        comboValues.add(NONE);
        List memberList = (List) _env.getValue("model");
        EObject selObj = (EObject) ((StructuredSelection) _env.getValue(
        		"selection")).getFirstElement();
        String selPointerType = ((EEnumLiteral) EcoreUtils.
        	getValue(selObj, "pointer")).getName();
        String selParamType = ((EEnumLiteral) EcoreUtils.
			getValue(selObj, "ParameterType")).getName();
        for (int i = 0; i < memberList.size(); i++) {
            EObject memberObj = (EObject) memberList.get(i);
            if (!selObj.equals(memberObj)) {
	            String datatype = (String) EcoreUtils.
	                getValue(memberObj, "type");
	            String pointerType = ((EEnumLiteral) EcoreUtils.
	                getValue(memberObj, "pointer")).getName();
	            String paramType = ((EEnumLiteral) EcoreUtils.
					getValue(memberObj, "ParameterType")).getName();
	            String lengthVar = (String) EcoreUtils.
					getValue(memberObj, "lengthVar");
	            if ((checkDataType(datatype) && pointerType.equals(NONE))
	            		|| (checkDataType(datatype) && pointerType.equals("Single"))) {
	            	// Length variable should be a simple argument. i.e it should not have lengthVar defined
	            	if (!lengthVar.equals(NONE)) {
	            		continue;
	            	}
	            	// Argument should not be of CL_INOUT type pointer with length variable defined,
	            	if (selParamType.equals("CL_INOUT") && 
	            			(selPointerType.equals("Single") || selPointerType.equals("Double"))) {
	            		comboValues.clear();
	            		comboValues.add(NONE);
	            		break;
	            	} 
	            	if (paramType.equals("CL_INOUT")) {
	            		if (!isValidInOutParameter(memberList, memberObj, selObj)) {
	            			continue;
	            		} else if (selParamType.equals("CL_OUT") && 
	            				(selPointerType.equals(NONE) || selPointerType.equals("Single"))) {
	            			continue;
	            		}
	            	} 
	            	if (paramType.equals("CL_OUT") && selParamType.equals("CL_IN")) {
	            		continue;
	            	} 
	            	if (selPointerType.equals("Double") && pointerType.equals(NONE)) {
	            		continue;
	            	}  
	            	if (selParamType.equals("CL_IN") && selPointerType.equals("Single")
	            			&& (paramType.equals("CL_OUT") || paramType.equals("CL_INOUT"))) {
	            		continue;
	            	}
	                String name = EcoreUtils.getName(memberObj);
	                comboValues.add(name);
	            }
        	}
        }
        String [] values = new String[comboValues.size()];
        for (int i = 0; i < comboValues.size(); i++) {
            values[i] = (String) comboValues.get(i);
        }
        return values;
    }
    /**
     * Checks the data type of the argument for a match with basic data types
     * @param dataType  - data type of argument to be 
     * @return true, if the argument data type is a basic data type else returns false
     */
    private boolean checkDataType(String dataType)
    {
    	EObject idlObj = RMDDialog.getInstance().getIDLSpecsObject();
        EPackage ePack = idlObj.eClass().getEPackage();
        EEnum dataTypes = (EEnum)ePack.getEClassifier("DataTypesEnum");
        List dataTypeList = dataTypes.getELiterals();
        
        for (int i = 0; i < dataTypeList.size(); i++) {
            EEnumLiteral enumLiteral = (EEnumLiteral) dataTypeList.get(i);
            if( dataType.equals(enumLiteral.toString()))
            	return true;
        }
        
        return false;
    	
    }
    /**
     * 
     * @param modelList - List of arguments in the model
     * 
     * @param inOutLengthVar - The Argument whose parameter type
     * is CL_INOUT and which need to checked whether it can be a
     * valid length variable of any other argument
     * 
     * @param selObj - Selected Argument Object
     * 
     * @return true, if inOutLengthVar is valid length variable
     * else return false
     */
    private boolean isValidInOutParameter(List modelList,
    		EObject inOutLengthVar, EObject selObj)
    {
    	String argName = EcoreUtils.getName(inOutLengthVar);
    	String paramType = EcoreUtils.getValue(selObj,
			"ParameterType").toString();
    	for (int i = 0; i < modelList.size(); i++) {
    		EObject modelObj = (EObject) modelList.get(i);
    		if (!modelObj.equals(inOutLengthVar)) {
    			String objParamType = EcoreUtils.getValue(modelObj,
    					"ParameterType").toString();
    			String lengthVar = EcoreUtils.getValue(modelObj,
					"lengthVar").toString();
    			if (paramType.equals("CL_IN")
    					&& objParamType.equals("CL_OUT")
	    					&& lengthVar.equals(argName)) {
	    				return false;
	    				
    			} else if (paramType.equals("CL_OUT")
    					&& objParamType.equals("CL_IN")
	    					&& lengthVar.equals(argName)) {
	    				return false;
	    				
	    		} 
    		}
    	}
    	return true;
    }

}
