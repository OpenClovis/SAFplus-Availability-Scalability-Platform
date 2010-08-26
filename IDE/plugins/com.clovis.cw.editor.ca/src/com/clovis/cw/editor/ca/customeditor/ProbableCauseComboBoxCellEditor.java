/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/customeditor/ProbableCauseComboBoxCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.customeditor;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;

/**
 * 
 * @author shubhada
 *
 * ComboBoxCellEditor to show the Probable Causes according to selected
 * Catagory value
 */
public class ProbableCauseComboBoxCellEditor extends ComboBoxCellEditor
{
    private Environment        _env     = null;
    private EStructuralFeature _feature = null;
    private static final int CATEGORY_COMMUNICATIONS = 0;
    private static final int CATEGORY_QUALITY_OF_SERVICE = 1;
    private static final int CATEGORY_PROCESSING_ERROR = 2;
    private static final int CATEGORY_EQUIPMENT = 3;
    private static final int CATEGORY_ENVIRONMENTAL = 4;
    /**
     * Constructor.
     *
     * @param parent
     *            Composite
     * @param feature
     *            EStructuralFeature
     * @param env
     *            Environment
     */
    public ProbableCauseComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
        _env = env;
        _feature = feature;
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
        return new ProbableCauseComboBoxCellEditor(parent, feature, env);
    }
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        List comboValues = new Vector();
        EObject selAlarmObj = (EObject)((StructuredSelection) _env.
                getValue("selection")).getFirstElement();
        int category = ((EEnumLiteral) EcoreUtils.getValue(
                selAlarmObj, "Category")).getValue();
        EPackage ePack = selAlarmObj.eClass().getEPackage();
        switch (category) {
        case CATEGORY_COMMUNICATIONS:
            EEnum categoryEnum = (EEnum) ePack.getEClassifier(
                    "UICommunicationsProbableCauseType");
            List pcList = categoryEnum.getELiterals();
            for (int i = 0 ; i < pcList.size(); i++) {
                comboValues.add(pcList.get(i).toString());
            }
            
            break;
        case CATEGORY_QUALITY_OF_SERVICE:
            EEnum qosEnum = (EEnum) ePack.getEClassifier(
                "UIQoSProbableCauseType");
            pcList = qosEnum.getELiterals();
            for (int i = 0 ; i < pcList.size(); i++) {
                comboValues.add(pcList.get(i).toString());
            }
            break;
        case CATEGORY_PROCESSING_ERROR:
            EEnum peEnum = (EEnum) ePack.getEClassifier(
                "UIPEProbableCauseType");
            pcList = peEnum.getELiterals();
            for (int i = 0 ; i < pcList.size(); i++) {
                comboValues.add(pcList.get(i).toString());
            }
            break;
        case CATEGORY_EQUIPMENT:
            EEnum equipEnum = (EEnum) ePack.getEClassifier(
                "UIEquipmentProbableCauseType");
            pcList = equipEnum.getELiterals();
            for (int i = 0 ; i < pcList.size(); i++) {
                comboValues.add(pcList.get(i).toString());
            }
            break;
        case CATEGORY_ENVIRONMENTAL:
            EEnum envEnum = (EEnum) ePack.getEClassifier(
                "UIEnvironmentalProbableCauseType");
            pcList = envEnum.getELiterals();
            for (int i = 0 ; i < pcList.size(); i++) {
                comboValues.add(pcList.get(i).toString());
            }
            break;
        }
        String [] values = new String[comboValues.size()];
        for (int i = 0; i < comboValues.size(); i++) {
            values[i] = (String) comboValues.get(i);
        }
        return values;
    }

    /**
     *@param value - Value to be set
     */
    protected void doSetValue(Object value)
    {
        setItems(getComboValues());
        int index = 0;
        String[] items = this.getItems();
        for (int i = 0; i < items.length; i++) {
            if (items[i].equals(value)) {
                index = i;
            }
        }
        super.doSetValue(new Integer(index));
    }
   /**
    *@return the Value
    */
   protected Object doGetValue()
   {
       if (((Integer) super.doGetValue()).intValue() == -1) {
           return new String("");
       } else {
           return getItems()[((Integer) super.doGetValue()).intValue()];
       }
   }
   /**
    * When Cell Editor is activated, the Tooltip message
    * is displayed on the Message Area of the Containing 
    * dialog 
    */
    public void activate()
    {
        /*Object container = _env.getValue("container");
        if (container!= null && container instanceof PreferenceDialog) {
            String tooltip =
                EcoreUtils.getAnnotationVal(_feature, null, "tooltip");
            ((PreferenceDialog) container).setMessage(tooltip,
                    IMessageProvider.INFORMATION);
        } else if (container!= null && container instanceof TitleAreaDialog) {
            String tooltip =
                EcoreUtils.getAnnotationVal(_feature, null, "tooltip");
            ((TitleAreaDialog) container).setMessage(tooltip,
                    IMessageProvider.INFORMATION);
        }*/
        super.activate();
    }
}
