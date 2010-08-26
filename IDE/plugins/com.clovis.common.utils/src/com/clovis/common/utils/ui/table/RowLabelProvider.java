/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/RowLabelProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table;

import java.util.List;

import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.resource.ImageRegistry;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * Label provider for Table.
 */
public class RowLabelProvider extends LabelProvider
    implements ITableLabelProvider
{
    private final TableUI _tableUI;
    //Names of images used to represent checkboxes
    private static final String  CHECKED_IMAGE      = "checked.gif";
    private static final String  UNCHECKED_IMAGE = "unchecked.gif";
    private static ImageRegistry imageRegistry   = new ImageRegistry();
    static {
        imageRegistry.put(CHECKED_IMAGE, ImageDescriptor.createFromFile(
                RowLabelProvider.class, CHECKED_IMAGE));
        imageRegistry.put(UNCHECKED_IMAGE, ImageDescriptor.createFromFile(
                RowLabelProvider.class, UNCHECKED_IMAGE));
    }
    /**
     * Create RowLabelProvider for this TableUI
     * @param tableUI TableUI Instance
     */
    public RowLabelProvider(TableUI tableUI)
    {
        _tableUI = tableUI;
    }
    /**
     * No Image as of now.
     *  @param element - Object
     * @param columnIndex - Column Index
     * @return null
     */
    public Image getColumnImage(Object element, int columnIndex)
    {
        EObject eObject  = (EObject) element;
        EStructuralFeature attr = (EStructuralFeature)
            _tableUI.getFeatures().get(columnIndex);
        if (attr.getEType().getClassifierID() == EcorePackage.EBOOLEAN) {
            boolean value = ((Boolean) eObject.eGet(attr)).booleanValue();
            return imageRegistry.get(value ? CHECKED_IMAGE : UNCHECKED_IMAGE);
        }
        return null;
    }
    /**
     * Get label for the column.
     * @param element - Object
     * @param columnIndex - Column Index
     * @return lable for the column.
     */
    public String getColumnText(Object element, int columnIndex)
    {
        EObject eObject  = (EObject) element;
        EStructuralFeature attr = (EStructuralFeature)
            _tableUI.getFeatures().get(columnIndex);
        if (attr.getEType().getClassifierID() == EcorePackage.EBOOLEAN
            || attr instanceof EReference) {
            return "";
        } else {
            Object value = eObject.eGet(attr);
            if (attr.getEType() instanceof EEnum) {
                EEnum uiEnum = EcoreUtils.getUIEnum(attr);
                if (uiEnum != null) {
                    EEnumLiteral literal = (EEnumLiteral) value;
                    return uiEnum.getEEnumLiteral(literal.getValue()).getName();
                }
            }
            if (value instanceof EObject) {
                String name = EcoreUtils.getName((EObject) value);
                if (name == null) {
                    name = EcoreUtils.getLabel(((EObject)value).eClass());
                }
                if (name != null) {
                    return name;
                } 
            } else if (value instanceof List) {
                String canDisplayValue = EcoreUtils.getAnnotationVal(attr, null,
                        AnnotationConstants.CAN_DISPLAY_VALUE);
                if (canDisplayValue == null || canDisplayValue.equals("false")) {
                    return "";
                }
                String name = "";
                List list = (List) value;
                for (int i = 0; i < list.size(); i++) {
                    if (list.get(i) instanceof EObject) {
                        EObject eobj = (EObject) list.get(i);
                        if (i < list.size() - 1) {
                            name = name + EcoreUtils.getName(eobj) + ", ";
                        } else {
                            name = name + EcoreUtils.getName(eobj);
                        }
                    } else {
                        if (i < list.size() - 1) {
                            name = name + list.get(i).toString() + ", ";
                        } else {
                            name = name + list.get(i).toString();
                        }
                    }
                }
                return name;
            }
            
            return (value != null) ? value.toString() : "";
        }
    }
    /**
     * Dispose. Does nothing.
     */
    public void dispose()
    { }
    /**
     * Return false.
     * @param eleme - Object
     * @param p - String
     * @return false.
     */
    public boolean isLabelProperty(Object eleme, String p)
    { return false; }
}
