/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.factory;

import java.lang.reflect.Method;

import org.eclipse.emf.ecore.EClassifier;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.CheckboxCellEditor;
import org.eclipse.jface.viewers.ICellEditorValidator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.editor.CComboBoxCellEditor;
import com.clovis.common.utils.ui.editor.CTextCellEditor;
/**
 * CellEditorFactory creates cell editors for a given attribute.
 * @author Ashish
 */
public class CellEditorFactory
{
    private final int _viewType;

    public static final int FORM_VIEW     = 0;
    public static final int TABLE_VIEW    = 1;
    public static final int PROPERTY_VIEW = 2;

    private static final String[] BOOL_VALS = {
            Boolean.TRUE.toString(),
            Boolean.FALSE.toString()
    };

    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    public static final CellEditorFactory FORM_INSTANCE
        = new CellEditorFactory(FORM_VIEW);
    public static final CellEditorFactory TABLE_INSTANCE
        = new CellEditorFactory(TABLE_VIEW);
    public static final CellEditorFactory PROPERTY_INSTANCE
        = new CellEditorFactory(PROPERTY_VIEW);
    /**
     * Private constructor.
     * @param viewType Type of View.
     */
    private CellEditorFactory(int viewType)
    {
        _viewType = viewType;
    }
    /**
     * Creates a CellEditor with default style.
     * @param feature editor for this attribute
     * @param parent  parent component for the editor
     * @param env     Environment
     * @return CellEditor for the supplied attribute.
     */
    public CellEditor getEditor(EStructuralFeature feature,
            Composite parent, Environment env)
    {
        return getEditor(feature, parent, env, SWT.NONE);
    }
    /**
     * Returns the appropriate editor for the supplied attribute.
     * For an attribute of type EEnum, it returns a ComboBoxCellEditor,
     * and if type is EBoolean it returns a checkbox cell editor.If type
     * is EReference it returns PushButtonCellEditor.
     * while for rest others, it returns a TextCellEditor.
     *
     * @param feature editor for this attribute
     * @param parent  parent component for the editor
     * @param env     Environment
     * @param style   SWT Style
     * @return CellEditor for the supplied attribute.
     */
    public CellEditor getEditor(EStructuralFeature feature,
            Composite parent, Environment env, int style)
    {
        CellEditor cellEditor = null;
        ICellEditorValidator validator = new CellEditorValidator(feature, env);
        String custom =
            EcoreUtils.getAnnotationVal(feature, null, "editor");
        if (custom != null) {
            if (custom.equalsIgnoreCase("no")) {
                return null;
            }
            cellEditor = getCustomEditor(custom, parent, feature, env);
            cellEditor.setValidator(validator);
        }
        if (cellEditor == null) {
            EClassifier type = feature.getEType();
            if (type instanceof EEnum) {
                style = style | SWT.READ_ONLY;
                
                // provides enum to ui enum mapping
                EEnum uiEnum = EcoreUtils.getUIEnum(feature);
                if (uiEnum == null) {
                    uiEnum = (EEnum) feature.getEType();
                }
                cellEditor = new CComboBoxCellEditor(parent,
                        uiEnum, style, _viewType, feature, env);
                cellEditor.setValidator(validator);
            } else if (feature instanceof EReference) {
                EReference reference = (EReference) feature;
                cellEditor = new PushButtonCellEditor(parent, reference, env);
                cellEditor.setValidator(validator);
            } else if (type.getClassifierID() == EcorePackage.EBOOLEAN) {
                if (_viewType == TABLE_VIEW) {
                    cellEditor = new CheckboxCellEditor(parent, style);
                    cellEditor.setValidator(validator);
                } else {
                    style = style | SWT.READ_ONLY;
                    cellEditor = new CComboBoxCellEditor(
                            parent, BOOL_VALS, style, _viewType, feature, env);
                    cellEditor.setValidator(validator);
                }
            } else {
                cellEditor = new CTextCellEditor(parent, style, _viewType, feature, env);
                cellEditor.setValidator(validator);
            }
        }
        String tooltip =
            EcoreUtils.getAnnotationVal(feature, null, "tooltip");
        if (cellEditor.getControl() != null && tooltip != null) {
            cellEditor.getControl().setToolTipText(tooltip);
        }
        custom = EcoreUtils.getAnnotationVal(feature, null, "celleditorvalidator");
        if(custom != null) {
        	cellEditor.setValidator(getCustomCellEditorValidator(custom, parent, feature, env));
        }
        return cellEditor;
    }
    
    /**
     * Get Custom Editor for given feature.
     * @param customEditor Name of editor class.
     * @param parent       Parent Composite
     * @param feature      Feature
     * @param env          Environment
     * @return Custom Editor (null in case of errors)
     */
    private CellEditor getCustomEditor(String customEditor,
            Composite parent, EStructuralFeature feature, Environment env)
    {
        CellEditor editor = null;
        Class editorClass = ClovisUtils.loadClass(customEditor);
        try {
            Class[] argType = {
                Composite.class,
                EStructuralFeature.class,
                Environment.class
            };
            if (editorClass != null) {
                Method met  = editorClass.getMethod("createEditor", argType);
                Object[] args = {parent, feature, env};
                editor = (CellEditor) met.invoke(null, args);
            }
        } catch (NoSuchMethodException e) {
            LOG.error("Custom editor does not have createEditor() method", e);
        } catch (Throwable th) {
            LOG.error("Unhandled error while creating editor:" + customEditor, th);
        }
        return editor;
    }
    private ICellEditorValidator getCustomCellEditorValidator(String customValidator,
            Composite parent, EStructuralFeature feature, Environment env)
    {
    	ICellEditorValidator validator = null;
        Class editorClass = ClovisUtils.loadClass(customValidator);
        try {
            Class[] argType = {
                EStructuralFeature.class,
                Environment.class
            };
            if (editorClass != null) {
                Method met  = editorClass.getMethod("createCellEditorValidator", argType);
                Object[] args = {feature, env};
                validator = (ICellEditorValidator) met.invoke(null, args);
            }
        } catch (NoSuchMethodException e) {
            LOG.error("Custom editor does not have createCellEditorValidator() method", e);
        } catch (Throwable th) {
            LOG.error("Unhandled error while creating celleditorvalidator:" + customValidator, th);
        }
        return validator;
    }
}
