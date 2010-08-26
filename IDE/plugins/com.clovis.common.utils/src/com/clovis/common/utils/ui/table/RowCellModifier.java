/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/RowCellModifier.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table;

import java.util.Iterator;
import java.util.StringTokenizer;

import org.eclipse.swt.widgets.TableItem;

import org.eclipse.jface.viewers.ICellModifier;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import com.clovis.common.utils.ecore.EcoreUtils;
/**
 * Provides CellModifier for the Row.
 * @author ASHISH
 */
public class RowCellModifier implements ICellModifier
{
    /**
     * Returns true
     * @param  element  Row (EObject)
     * @param  property Column Name (Name of the feature)
     * @return true
     */
    public boolean canModify(Object element, String property)
    {
        EObject eObject = null;
        if (element instanceof TableItem) {
            eObject = (EObject) (((TableItem) element).getData());
        } else if (element instanceof EObject) {
            eObject = (EObject) element;
        }

        EList list = (EList)EcoreUtils.getValue(eObject, "Disable");
        if(list != null) {
        	Iterator itr = list.iterator();
        	while(itr.hasNext()) {
        		if(itr.next().equals(property)) {
        			return false;
        		}
        	}
        }
        
        if (eObject != null) {
            EStructuralFeature feature =
                eObject.eClass().getEStructuralFeature(property);
            String modifyStr = EcoreUtils.getAnnotationVal(
                    feature, null, "canModify");
            if (modifyStr != null) {
                boolean canModify = Boolean.parseBoolean(modifyStr);
                if (!canModify) {
                    return false;
                }
            } else {
                return true;
            }
        }
        return true;
    }
    /**
     * Gets value for a given row and column.
     * @param  element  Row (EObject)
     * @param  property Column Name (Name of the feature)
     * @return Value for this EObject and feature.
     */
    public Object getValue(Object element, String property)
    {
        EObject eObject  = (EObject) element;
        EStructuralFeature attr =
            eObject.eClass().getEStructuralFeature(property);
        Object value = eObject.eGet(attr);
        if (attr instanceof EReference) {
            value = eObject.eGet(attr);
            if (value == null) {
                value = EcoreUtils.createEObject(((EReference) attr)
                        .getEReferenceType(), true);
            }
        } else if (attr.getEType() instanceof EEnum) {
            EEnum uiEnum = EcoreUtils.getUIEnum(attr);
            if (uiEnum != null) {
                EEnumLiteral literal = (EEnumLiteral) value;
                return uiEnum.getEEnumLiteral(literal.getValue()).getName();
            }
        }
        return value;
    }
    /**
     * Modify the value in object.
     * @param element  Row (EObject)
     * @param property Column name
     * @param val      Modified value
     */
    public void modify(Object element, String property, Object val)
    {
        EObject eObject = null;
        if (element instanceof TableItem) {
            eObject = (EObject) (((TableItem) element).getData());
        } else if (element instanceof EObject) {
            eObject = (EObject) element;
        }
        if (val != null && (eObject != null)) {
            EStructuralFeature feature =
                eObject.eClass().getEStructuralFeature(property);
            if (feature instanceof EReference) {
                if (!eObject.eIsSet(feature)) {
                    eObject.eSet(feature, val);
                }
            } else if (feature.getUpperBound() > 1
                    || feature.getUpperBound() == -1) {
                eObject.eSet(feature, val);
            } else if (feature.getEType() instanceof EEnum) {
                EEnum uiEnum = EcoreUtils.getUIEnum(feature);
                if (uiEnum != null) {
                    EEnumLiteral literal = uiEnum.getEEnumLiteral((String) val);
                    EEnum backendEnum = (EEnum) feature.getEType();
                    EEnumLiteral backendLiteral = (EEnumLiteral) backendEnum.
                            getEEnumLiteral(literal.getValue());
                    EcoreUtils.setValue(eObject, feature.getName(),
                            backendLiteral.getName());
                } else {
                    EcoreUtils.setValue(eObject, feature.getName(), val.toString());
                }
            } else {
                EcoreUtils.setValue(eObject, feature.getName(), val.toString());
            }

            if (element instanceof TableItem) {
            	applyDependency(eObject, feature, val);
            }
        }
    }
    
    /**
     * Updates the contents of dependent fields.
     * 
     * @param eObject EObject for the current updatation 
     * @param feature Structural Feature for which we need to do
     * dependancy updatation.
     * @param val Value of Object for the current updatation 
     */
	private void applyDependency(EObject eObject, EStructuralFeature feature, Object val) {
        String str = EcoreUtils.getAnnotationVal(feature, null, "Dependency");
        
        if(str != null) {
        	StringTokenizer entryTokenizer = new StringTokenizer(str, "|");

        	while(entryTokenizer.hasMoreTokens()) {
        		String entry = entryTokenizer.nextToken();
        		StringTokenizer detailTokenizer = new StringTokenizer(entry, ";=");
        		String entryValue = detailTokenizer.nextToken();

        		if((val.toString()).equalsIgnoreCase(entryValue)) {
        			while(detailTokenizer.hasMoreTokens()) {
        				String featureName = detailTokenizer.nextToken();
        				String value = detailTokenizer.nextToken();

        				if(featureName.contains(":")) {
        					int index = featureName.indexOf(":");
        					String disableType = featureName.substring(index+1, featureName.length());
        					featureName = featureName.substring(0, index);
        					EList list = (EList)EcoreUtils.getValue(eObject, featureName);

        					if(disableType.equals("add")) {
        						list.add(value);
        					} else if(disableType.equals("remove")) {
        						list.remove(value);
        					}
        				} else {
        					EcoreUtils.setValue(eObject, featureName, value);
        				}
        			}
        		}
        	}
        }
	}
}
