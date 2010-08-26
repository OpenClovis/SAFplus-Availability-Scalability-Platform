/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/TreeAttributesTab.java $
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
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.List;

import com.clovis.common.utils.ui.list.ListView;
import com.clovis.common.utils.ui.tree.TreeNode;
import com.clovis.common.utils.ui.tree.TreeUI;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
/**
 * @author shubhada
 *
 * Tab which has tree which shows loaded mibs to the user and
 * allows hime to select attributes for import
 */
public class TreeAttributesTab extends CTabItem
{
    protected Button        _selectButton, _unselectButton;
    protected AttributeList      _listViewer;
    protected TreeComposite _treeComp;
    private List          _list;
    private final String  _moveRight = "Select";
    private final String  _moveLeft  = "Delete";
    /**
     * Constructor
     * @param parent CtabFolder
     * @param style int
     */
    public TreeAttributesTab(CTabFolder parent, int style)
    {
        super(parent, style);
        setControl(createContents(parent));
        setText("MIB Tree");
    }
    /**
     *
     * @param arg0 parent CTabFolder
     * @return Composite
     */
    protected Composite createContents(CTabFolder arg0)
    {
        Composite baseComposite = new Composite(arg0, SWT.NONE);
        baseComposite.setLayoutData(new GridData(GridData.FILL_BOTH));

        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 3;
        baseComposite.setLayout(gridLayout);

        _treeComp = new TreeComposite(baseComposite, SWT.NONE);

        GridData gridData1 = new GridData(GridData.FILL_BOTH);

        //gridData1.horizontalAlignment = GridData.BEGINNING;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.verticalAlignment = GridData.FILL;

        _treeComp.setLayoutData(gridData1);
        _treeComp.getTree().addSelectionChangedListener(new
                SelectionChangedListener());
        Composite buttonComposite = new Composite(baseComposite, SWT.NONE);
        buttonComposite.setLayoutData(new GridData());
        buttonComposite.setLayout(new GridLayout());

        GridData gridData2 = new GridData();
        gridData2.horizontalAlignment = GridData.CENTER;
        gridData2.horizontalAlignment = GridData.FILL;
        gridData2.grabExcessHorizontalSpace = true;
        gridData2.verticalAlignment = GridData.FILL;
        _selectButton = new Button(buttonComposite, SWT.PUSH);
        _selectButton.setText(_moveRight);
        _selectButton.setLayoutData(gridData2);
        _selectButton.setEnabled(false);
        //        Adding Selection listener on button
        _selectButton.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e)
            {
            	addItems();
            }
        });

        GridData gridData3 = new GridData();
        gridData3.horizontalAlignment = GridData.CENTER;
        gridData3.horizontalAlignment = GridData.FILL;
        gridData3.grabExcessHorizontalSpace = true;
        gridData3.verticalAlignment = GridData.FILL;
        _unselectButton = new Button(buttonComposite, SWT.PUSH);
        _unselectButton.setText(_moveLeft);
        _unselectButton.setLayoutData(gridData3);
        _unselectButton.setEnabled(false);
        //Adding Selection listener on button
        _unselectButton.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e)
            {
                StructuredSelection sel = (StructuredSelection) _listViewer
                        .getSelection();
                for (int i = 0; i < sel.toList().size(); i++) {
                    ((NotifyingList) _listViewer.getInput()).remove(sel.toList()
                            .get(i));
                    _listViewer.refresh();
                }
            }
        });
        Group attrGroup = new Group(baseComposite, SWT.SHADOW_OUT);
        attrGroup.setText("Selected MIB Attributes");
        GridData gd = new GridData();
        gd.horizontalAlignment = GridData.FILL;
        gd.verticalAlignment = GridData.FILL;
        gd.grabExcessVerticalSpace = true;
        attrGroup.setLayoutData(gd);

        GridLayout groupLayout = new GridLayout();
        attrGroup.setLayout(groupLayout);
        int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL
                    | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
        _list = new List(attrGroup, style);

        _listViewer = new AttributeList(_list);
        _listViewer.setInput(new ClovisNotifyingListImpl());
        _listViewer.addSelectionChangedListener(new SelectionChangedListener());

        GridData gridData4 = new GridData(GridData.FILL_BOTH);
        gridData4.horizontalAlignment = GridData.FILL;
        gridData4.grabExcessHorizontalSpace = true;
        gridData4.verticalAlignment = GridData.FILL;
        gridData4.widthHint = 150;
        _list.setLayoutData(gridData4);
        return baseComposite;
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
            if (isTreeNodeLeaf(node) && !isTreeNodeTrapObject(node)) {
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
     * @return ListViewer
     */
    public ListView getListViewer()
    {
        return _listViewer;
    }
    /**
     *
     * @return TreeViewer
     */
    public TreeUI getTreeViewer()
    {
        return _treeComp.getTree();
    }
    /**
     *@param node TreeNode
     * @return true if the tree node is the leaf EObject else return false.
     */
    public boolean isTreeNodeTrapObject(TreeNode node)
    {
        Object val = node.getValue();
        if (val instanceof EObject) {
     	   return ((Boolean) EcoreUtils.getValue((EObject) val,
     			   "IsSnmpV2TrapNode")).booleanValue();
        }
        return false;
    }
    
    /**
    *@param node TreeNode
    * @return true if the tree node is the leaf EObject else return false.
    */
   public boolean isTreeNodeLeaf(TreeNode node)
   {
       Object val = node.getValue();
       if (val instanceof EObject) {
    	   String name = EcoreUtils.getName((EObject) val);
    	   if (name.equals("MibTreeNode(No Mibs Loaded)")) {
    		   return false;
    	   }
           EList features = ((EObject) val).eClass().
                         getEAllStructuralFeatures();
           for (int i = 0; i < features.size(); i++) {
               if (features.get(i) instanceof EReference) {
                   EList refList = (EList) ((EObject) val).
                       eGet((EStructuralFeature) features.get(i));
                   if (refList.isEmpty() || refList == null) {
                       return true;
                   }
               }
           }
       }
       return false;
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
                                          || childrefList == null) && !isTrapNode) {
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
              || isTreeNodeTableObject(node))
              && !isTreeNodeTrapObject(node)) {
		  _selectButton.setEnabled(true);
       } else {
    	   _selectButton.setEnabled(false);
       }
  }
  /**
   * @author shubhada
   *
   * Selection Changed Listener on Tree
   */
  class SelectionChangedListener implements
          ISelectionChangedListener
   {

      /**
       * Selection changed implementation
       *
       * @param event
       *            SelectionChangedEvent
       */

      public void selectionChanged(SelectionChangedEvent event)
      {

          if (event.getSource() instanceof TreeUI) {
              StructuredSelection sel = (StructuredSelection) _treeComp.
              getTree().getSelection();
              TreeNode node = (TreeNode) sel.getFirstElement();
              if (node != null && node.getValue() instanceof EObject) {
                  EObject eobj = (EObject) node.getValue();
                  _treeComp.getDescriptionText().setEditable(true);
                  _treeComp.getDescriptionText().setText("Description :  "
                          + (String) EcoreUtils.getValue(eobj, "Description")
                          + "\n\nOID :  "
                          + (String) EcoreUtils.getValue(eobj, "OID"));
                  _treeComp.getDescriptionText().setEditable(false);
              }
                  for (int i = 0; i < sel.toList().size(); i++) {
                      node = (TreeNode) sel.toList().get(i);
                      changeStatus(node);
                  }
	      } else if (event.getSource() instanceof ListView) {
	          StructuredSelection sel = (StructuredSelection) _listViewer
	        .getSelection();
	          if (sel != null && !sel.isEmpty()) {
	              _unselectButton.setEnabled(true);
	          } else {
	              _unselectButton.setEnabled(false);
	          }
	      }

      }
   }

}
