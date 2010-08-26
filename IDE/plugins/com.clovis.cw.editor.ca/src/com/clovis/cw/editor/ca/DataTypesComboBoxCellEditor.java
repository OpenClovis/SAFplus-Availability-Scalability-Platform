/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/DataTypesComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.dialog.GenericFormPage;
import com.clovis.cw.editor.ca.dialog.RMDDialog;

/**
 * @author shubhada
 * Combo Box cell editor for DataTypes
 */
public class DataTypesComboBoxCellEditor extends CustomComboBoxCellEditor
{
    private Environment _env = null;
    /**
     * @param parent - parent Composite
     * @param feature - EStructuralFeature
     * @param env - Environment
     */
    public DataTypesComboBoxCellEditor(Composite parent,
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
        return new DataTypesComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        List comboValues = new Vector();
        EObject idlObj = RMDDialog.getInstance().getIDLSpecsObject();
        EPackage ePack = idlObj.eClass().getEPackage();
        EEnum dataTypes = (EEnum)ePack.getEClassifier("DataTypesEnum");
        EList dataTypeList = dataTypes.getELiterals();
        
        for (int i = 0; i < dataTypeList.size(); i++) {
            EEnumLiteral enumLiteral = (EEnumLiteral) dataTypeList.get(i);
            comboValues.add(enumLiteral.toString());            
        }
        addUserDefinedDataTypes(idlObj, comboValues);
        
        // Also add the local data types defined for the EO.
        StructuredSelection obj = (StructuredSelection) _env.getValue("selection");
        EObject selObj = (EObject) obj.getFirstElement();
        if (selObj.eClass().getName().equals("ARGUMENT")) {
            Environment parentEnv = _env.getParentEnv();
			PreferencePage page = (PreferencePage) ((FormView) parentEnv)
					.getValue("container");
			EObject eoObj = ((GenericFormPage) page).getEObject().eContainer()
					.eContainer().eContainer();
			addUserDefinedDataTypes(eoObj, comboValues);
        } else if (selObj.eClass().getName().equals("DataMember")) {
            Environment parentEnv = _env.getParentEnv();
			PreferencePage page = (PreferencePage) ((FormView) parentEnv)
					.getValue("container");
			EObject dataTypeObj = ((GenericFormPage) page).getEObject();
			EObject eoObj = dataTypeObj.eContainer();
			if (eoObj != null) {
				addUserDefinedDataTypes(eoObj, comboValues);
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
     * @param typeObj - Types Objects which is a holder for
     * user defined Data types
     * @param comboValues - Combobox values
     */
    private void addUserDefinedDataTypes(EObject typeObj, List comboValues)
    {
        EList enumList = (EList) EcoreUtils.getValue(typeObj, "Enum");
        EList unionList = (EList) EcoreUtils.getValue(typeObj, "Union");
        EList structList = (EList) EcoreUtils.getValue(typeObj, "Struct");
        for (int i = 0; i < enumList.size(); i++) {
            EObject eobj = (EObject) enumList.get(i);
            String name = (String) EcoreUtils.getValue(eobj, "name");
            comboValues.add(name);
        }
        for (int i = 0; i < unionList.size(); i++) {
            EObject eobj = (EObject) unionList.get(i);
            String name = (String) EcoreUtils.getValue(eobj, "name");
            comboValues.add(name);
        }
        for (int i = 0; i < structList.size(); i++) {
            EObject eobj = (EObject) structList.get(i);
            String name = (String) EcoreUtils.getValue(eobj, "name");
            comboValues.add(name);
        }
    }
}
