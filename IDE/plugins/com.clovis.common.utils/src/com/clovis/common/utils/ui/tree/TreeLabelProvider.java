/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/tree/TreeLabelProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.tree;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;

import com.clovis.common.utils.ecore.EcoreUtils;
/**
 *
 * @author shubhada Tree Label Provider class
 */
public class TreeLabelProvider extends LabelProvider
{
    /**
     * @param obj Object
     * @return image to be shown for the object
     */
    public Image getImage(Object obj)
    {
        return null;
    }
    /**
     * @param obj
     *            Object
     * @return label for the tree node.
     */
     public String getText(Object obj)
     {
         TreeNode node = (TreeNode) obj;
         EStructuralFeature feature = node.getFeature();
         Object val  = node.getValue();
         if (val instanceof EObject && feature != null) {
        	 String label = feature.getName();
             String featureLabel = EcoreUtils.
             getAnnotationVal(feature, null, "label");
             if (featureLabel != null) {
                 label = featureLabel;
             }
             if (feature.getEType() instanceof EEnum) {
                 return label.concat(":").
                 concat(val.toString());
             }
             EObject eobj = (EObject) val;
             String name = EcoreUtils.getName(eobj);
             if (name != null) {
                 return name;
             } else {
                 return label;
             }
         } else if (val instanceof EObject && feature == null) {
             EObject eobj = (EObject) val;
             String name = EcoreUtils.getName(eobj);
             if (name != null) {
                 return name;
             } else {
                 return eobj.eClass().getName();
             }
         } else if (val instanceof EList) {
             String label = feature.getName();
             String featureLabel = EcoreUtils.
                 getAnnotationVal(feature, null, "label");
             if (featureLabel != null) {
                 label = featureLabel;
             }
             return label;
         } if (feature != null && val != null
        && !(val instanceof EObject) && !(val instanceof EList)) {
             String label = feature.getName();
             String featureLabel = EcoreUtils.
                 getAnnotationVal(feature, null, "label");
             if (featureLabel != null) {
                 label = featureLabel;
             }
             return label.
             concat(":").concat(val.toString());
         }
             return val.toString();
         }
}
