/*******************************************************************************
 * ModuleName  : plugins
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/TrapAttributesTab.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.snmp;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.custom.CTabFolder;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.tree.TreeNode;
import com.clovis.common.utils.ui.tree.TreeUI;
/**
 * @author shubhada
 *
 * Tab which has tree which shows loaded mibs to the user and
 * allows hime to select attributes for import
 */
public class TrapAttributesTab extends TreeAttributesTab
{
    
    /**
     * Constructor
     * @param parent CtabFolder
     * @param style int
     */
    public TrapAttributesTab(CTabFolder parent, int style)
    {
        super(parent, style);
    }
    /**
     * Add the selected tree items in to the list
     *
     */
    protected void addItems()
    {
    	TreeUI treeViewer = _treeComp.getTree();
        StructuredSelection sel = (StructuredSelection) treeViewer
                .getSelection();
        for (int i = 0; i < sel.toList().size(); i++) {
            TreeNode node = (TreeNode) sel.toList().get(i);
            if (isTreeNodeLeaf(node) 
            		&&	isTreeNodeTrapObject(node)) {
                if (!(((NotifyingList) _listViewer.getInput())
                        .contains((EObject) node.getValue()))) {
                    ((NotifyingList) _listViewer.getInput())
                            .add((EObject) node.getValue());
                    _listViewer.refresh();
                }
            } else if (isTreeNodeTableObject(node)) {
                Object val = node.getValue();
                if (val instanceof EObject) {
                    EList features = ((EObject) val).eClass().
                                  getEAllStructuralFeatures();
                    for (int j = 0; j < features.size(); j++) {
                        if (features.get(j) instanceof EReference) {
                            EList refList = (EList) ((EObject) val).
                            eGet((EStructuralFeature) features.get(j));
                            if (!(refList.isEmpty()
                                || refList == null)) {
                                    for (int k = 0; k < refList.size();
                                                               k++) {
                                        EObject refObj = (EObject)
                                        refList.get(k);
                                        if (!(((NotifyingList)
                                                _listViewer.
                                                 getInput())
                                                .contains(refObj))) {
                                            ((NotifyingList)
                                                    _listViewer.
                                                    getInput())
                                                    .add(refObj);
                                            _listViewer.refresh();
                                        }
                                    }

                            }
                        }
                    }
                }
            }
        }
    }
    /**
    *
    *@param node TreeNode
    * @return true if the tree node is the table entry EObject else return false.
    */
   public boolean isTreeNodeTableObject(TreeNode node)
   {
       Object val = node.getValue();
       if (val instanceof EObject) {
           EList features = ((EObject) val).eClass().
                         getEAllStructuralFeatures();
           for (int i = 0; i < features.size(); i++) {
               if (features.get(i) instanceof EReference) {
                   EList refList = (EList) ((EObject) val).
                       eGet((EStructuralFeature) features.get(i));
                   if (!(refList.isEmpty() || refList == null)) {
                           for (int j = 0; j < refList.size(); j++) {
                               EObject refObj = (EObject) refList.get(j);
                               Boolean isTrap = (Boolean) EcoreUtils.getValue(
                             		  refObj, "IsSnmpV2TrapNode");
                               boolean isTrapNode = false;
                               if (isTrap != null) {
                             	  isTrapNode = isTrap.booleanValue();
                               }
                               EList refObjFeatures = refObj.eClass().
                              getEAllStructuralFeatures();
                               for (int k = 0; k < refObjFeatures.size(); k++) {
                                   if (refObjFeatures.get(k)
                                       instanceof EReference) {
                                       EList childrefList = (EList) refObj.
                                        eGet((EStructuralFeature) refObjFeatures.
                                                get(k));
                                       if ((childrefList.isEmpty()
                                           || childrefList == null) && isTrapNode) {
                                        return true;
                                    }
                                   }
                               }
                           }

                   }
               }
           }
       }
           return false;
   }
  /**
   * 
   * @param node - TreeNode
   * 
   * Changes the enable/disable state of the
   * select button based on whether Selected
   * Node is a leaf ot table
   */
  protected void changeStatus(TreeNode node)
  {
	  if ((isTreeNodeLeaf(node)
              && isTreeNodeTrapObject(node))
              || isTreeNodeTableObject(node)) {
		  _selectButton.setEnabled(true);
       } else {
    	   _selectButton.setEnabled(false);
       }
  }

}
