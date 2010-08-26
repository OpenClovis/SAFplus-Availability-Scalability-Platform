/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/view/MOTreeContentProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.view;

import java.util.HashMap;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITreeContentProvider;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.tree.TreeContentProvider;
import com.clovis.common.utils.ui.tree.TreeNode;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
/**
*
* @author shubhada
*Content Provider Class for the MOTree.
*/
public class MOTreeContentProvider extends TreeContentProvider
implements IStructuredContentProvider, ITreeContentProvider
{
   private EList _contentList = null;

   /**
   * @param parent - Object
   * @return the top level elements in the tree.
   */
   public Object[] getElements(Object parent)
   {
       _contentList = (EList) parent;
       ClovisNotifyingListImpl elemList = new ClovisNotifyingListImpl();
       
       EObject rootObject = (EObject) _contentList.get(0);
       List chassisList = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
       for (int i = 0; i < chassisList.size(); i++) {
           EObject eobj = (EObject) chassisList.get(i);
           elemList.add(new TreeNode(null, null, eobj));
       }
       return elemList.toArray();
   }
   /**
    * @param parent - Object
    * @return true if has children else false.
    *//*
    public boolean hasChildren(Object parent)
    {
        boolean hasChildren = super.hasChildren(parent);
        if (hasChildren) {
            Object val = ((TreeNode) parent).getValue();
            if (val instanceof EObject) {
                EClass eClass = ((EObject) val).eClass();
                hasChildren  = !(eClass.getName().equals("Attribute")
                    || eClass.getName().equals("Method"));
            }
        }
        return hasChildren;
    }*/
   /**
    * @param parent Object
    * @return the child nodes of parent TreeNode
    */
   public Object [] getChildren(Object parent)
   {
       TreeNode node = (TreeNode) parent;
       Object   obj  =  node.getValue();
       if (!(obj instanceof EObject) && !(obj instanceof List)) {
           return new Object[0];
       }
       Vector elemList = new Vector();
       Object[] children = super.getChildren(parent);
       for (int i = 0; i < children.length; i++) {
           elemList.add(children[i]);
       }
       elemList.addAll(getChildNodes(node));
       return elemList.toArray();
   }
   /**
    * Get Child Nodes (Complex Children only)
    * @param parent Parent EObject
    * @return list of complex children.
    */
   private List getChildNodes(TreeNode parent)
   {   
	   List childrens = new Vector();
       if (parent.getValue() instanceof EObject) {
           EObject parentObj = (EObject) parent.getValue();
           HashMap keyObjMap = new HashMap();
           EObject rootObject = (EObject) _contentList.get(0);
           List refList = rootObject.eClass().getEAllReferences();
           for (int i = 0; i < refList.size(); i++) {
        	   EReference ref = (EReference) refList.get(i);
        	   List list = (EList) rootObject.eGet(ref);
        	   for (int j = 0; j < list.size(); j++) {
                   EObject eObj = (EObject) list.get(j);
                   String  key  = (String) EcoreUtils.getValue(eObj,
                		   ModelConstants.RDN_FEATURE_NAME);
                   keyObjMap.put(key, eObj);
               }
           }
           
           List connsList = (EList) rootObject.eGet(rootObject.eClass()
   				.getEStructuralFeature(ClassEditorConstants.COMPOSITION_REF_NAME));
           for (int i = 0; i < connsList.size(); i++) {
               EObject eObj = (EObject) connsList.get(i);
               String src = (String) EcoreUtils.getValue(eObj, ClassEditorConstants.CONNECTION_START);
               if (src.equals(EcoreUtils.getValue(parentObj,
            		   ModelConstants.RDN_FEATURE_NAME))) {
                   String end = (String) EcoreUtils.
                       getValue(eObj, ClassEditorConstants.CONNECTION_END);
                   Object targetObj = keyObjMap.get(end);
                   if (targetObj != null) {
                       childrens.add(new TreeNode(null, parent, targetObj));
                   }
               }
           }
       }
       return childrens;
   }
}
