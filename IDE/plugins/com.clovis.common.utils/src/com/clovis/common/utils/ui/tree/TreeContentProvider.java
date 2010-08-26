/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/tree/TreeContentProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.tree;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;

import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * @author shubhada
 *
 * Content Provider for the tree.
 */
public class TreeContentProvider implements IStructuredContentProvider,
                                ITreeContentProvider
{
    /**
     * @param v - Viewer
     * @param oldInput Object
     * @param newInput Object
     */

    public void inputChanged(Viewer v, Object oldInput, Object newInput)
    {
    }
    /**
     * Does nothing
     */
    public void dispose()
    { }
    /**
     * @param parent - Object
     * @return the top level elements in the tree.
     */
    public Object[] getElements(Object parent)
    {
        NotifyingList elems = (NotifyingList) parent;
        Vector elemList = new Vector();
        for (int i = 0; i < elems.size(); i++) {
            elemList.add(new TreeNode(null, null, (EObject) elems.get(i)));
        }
        return elemList.toArray();
    }
    /**
    * @param child Object
    * @return the parent of TreeNode
    */
    public Object getParent(Object child)
    {
        TreeNode node = (TreeNode) child;
        return node.getParent();
    }
    /**
     * @param parent Object
     * Checks whether parent has any children
     * @return true if parent has chilren else false.
     */
    public boolean hasChildren(Object parent)
    {
        Object val    = ((TreeNode) parent).getValue();
        EStructuralFeature feature = ((TreeNode) parent).getFeature();
        if (feature != null) {
            return ((val instanceof EObject || val instanceof EList)
                && !(feature.getEType() instanceof EEnum));
        } else {
            return (val instanceof EObject || val instanceof EList);
        }
    }
    /**
    * @param parent Object
    * @return the child nodes of parent TreeNode
    */
    public Object [] getChildren(Object parent)
    {
        TreeNode node     = (TreeNode) parent;
        Object   obj      =  node.getValue();
        Vector   elemList = new Vector();
        if (obj instanceof EObject) {
            EObject eObj = (EObject) obj;
            List features = EcoreUtils.getFeatureList(eObj.eClass(), true);
            for (int i = 0; i < features.size(); i++) {
                EStructuralFeature feature =
                    (EStructuralFeature) features.get(i);
                if (!(feature instanceof EReference)) {
                    elemList.add(new TreeNode(feature, (TreeNode) parent,
                            eObj.eGet(feature)));
                } else {
                    Object value = eObj.eGet(feature);
                    if (value instanceof EObject) {
                        EObject refobj = (EObject) value;
                        elemList.add(new TreeNode(feature, node, refobj));
                    } else if (value instanceof EList) {
                        EList refList = (EList) value;
                        for (int j = 0; j < refList.size(); j++) {
                            EObject eobj = (EObject) refList.get(j);
                            elemList.add(new TreeNode(feature, node, eobj));
                        }
                    }
                }
            }
        } else if (obj instanceof List) {
            for (int i = 0; i < ((List) obj).size(); i++) {
                if (((List) obj).get(i) instanceof EObject) {
                EObject eObj = (EObject) ((List) obj).get(i);
                if (eObj != null) {
                    elemList.add(new TreeNode(null, node.getParent(), eObj));
                }
                } else {
                    Object object = (Object) ((List) obj).get(i);
                    if (obj != null) {
                    elemList.add(new TreeNode(null, node.getParent(), object));
                    }
                }
            }
        }
        return elemList.toArray();
    }
}
