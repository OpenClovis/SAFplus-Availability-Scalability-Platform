/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/view/MOTreeLabelProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.view;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.tree.TreeLabelProvider;
import com.clovis.common.utils.ui.tree.TreeNode;

/**
 * @author shubhada
 *
 *MO Tree Label Provider
 */
public class MOTreeLabelProvider extends TreeLabelProvider
{
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
             	/*if (eobj.eClass().getName().equals("Attribute")
             			|| eobj.eClass().getName().equals("Method")) {
             		EStructuralFeature typeFeature = eobj.eClass().
		             getEStructuralFeature(ClassEditorConstants.ATTRIBUTE_TYPE);
             		 return name + " : " + eobj.eGet(typeFeature).toString();
             	}*/
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
         } else if (feature != null && val != null
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
