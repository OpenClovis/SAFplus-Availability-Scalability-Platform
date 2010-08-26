/*******************************************************************************
 * ModuleName  : plugins
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/DataMemberLengthVarComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.Vector;

import org.eclipse.emf.common.util.EList;
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
 * Custom ComboBox Cell Editor for Length Variable of Data Members
 */
public class DataMemberLengthVarComboBoxCellEditor extends CustomComboBoxCellEditor
{
    private Environment _env = null;
    /**
     * @param parent - parent Composite
     * @param feature - EStructuralFeature
     * @param env - Environment
     */
    public DataMemberLengthVarComboBoxCellEditor(Composite parent,
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
        return new DataMemberLengthVarComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        Vector comboValues = new Vector();
        comboValues.add("None");
        EObject selObj = (EObject) ((StructuredSelection) _env.getValue(
			"selection")).getFirstElement();
        EList memberList = (EList) _env.getValue("model");
        for (int i = 0; i < memberList.size(); i++) {
            EObject memberObj = (EObject) memberList.get(i);
            if (!memberObj.equals(selObj)) {
	            String datatype = (String) EcoreUtils.
	                getValue(memberObj, "type");
	            if ( checkDataType(datatype)) {
	                String name = (String) EcoreUtils.
	                getValue(memberObj, "name");
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
     * 
     * @param dataType - Basic Data Type supported by RMD
     * @return the true if data type is a basic data type else if 
     * data type is user defined return false
     */
    private boolean checkDataType(String dataType)
    {
    	EObject idlObj = RMDDialog.getInstance().getIDLSpecsObject();
        EPackage ePack = idlObj.eClass().getEPackage();
        EEnum dataTypes = (EEnum)ePack.getEClassifier("DataTypesEnum");
        EList dataTypeList = dataTypes.getELiterals();
        
        for (int i = 0; i < dataTypeList.size(); i++) {
            EEnumLiteral enumLiteral = (EEnumLiteral) dataTypeList.get(i);
            if( dataType.equals(enumLiteral.toString()))
            	return true;
        }
        
        return false;
    	
    }
}
