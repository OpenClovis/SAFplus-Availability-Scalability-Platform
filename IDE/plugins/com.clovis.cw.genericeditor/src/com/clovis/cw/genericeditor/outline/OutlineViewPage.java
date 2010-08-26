/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/outline/OutlineViewPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.outline;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.views.contentoutline.ContentOutlinePage;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ListObjectListener;
import com.clovis.common.utils.ui.tree.TreeContentProvider;
import com.clovis.common.utils.ui.tree.TreeLabelProvider;
import com.clovis.common.utils.ui.tree.TreeNode;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.NodeModel;
/**
 * @author shubhada
 *
 * Outline View for the Resources in Resource Editor
 */
public class OutlineViewPage extends ContentOutlinePage implements IDoubleClickListener
{
    private final EList _elist;
    private GenericEditor _editor = null;
    private ListObjectListener      _listener;
    private ITreeContentProvider   _treeContentProvider = null;
    private LabelProvider          _labelProvider = null;
    /**
     * Constructor.
     * @param list List of EObjects.
     */
    public OutlineViewPage(EList list, GenericEditor editor)
    {
        this(list, editor, null, null);
    }
    /**
     * Constructor.
     * @param list List of EObjects.
     */
    public OutlineViewPage(EList list, GenericEditor editor,
    		ITreeContentProvider cProvider,
            LabelProvider lProvider) 
    {
        super();
        _editor = editor;
        _elist = list;
        _treeContentProvider = cProvider;
        _labelProvider = lProvider;
    }
    
    /**
     * Creates control for outline view.
     * gets tree viewer from superclass and sets new content and
     * label provider. These providers are written for Ecore Models.
     * @param parent Parent Composite.
     */
    public void createControl(Composite parent)
    {
        super.createControl(parent);
        if (_treeContentProvider != null) {
            getTreeViewer().setContentProvider(_treeContentProvider);
        } else {
            getTreeViewer().setContentProvider(new TreeContentProvider());
        }
        if (_labelProvider != null) {
            getTreeViewer().setLabelProvider(_labelProvider);
        } else {
            getTreeViewer().setLabelProvider(new TreeLabelProvider());
        }
        _listener    = new ListObjectListener(getTreeViewer());
        getTreeViewer().getTree().addDisposeListener(_listener);
        getTreeViewer().addDoubleClickListener(this);
        NotifyingList list = (NotifyingList) _elist;
//      Add listener to new Input
        if (list != null) {
            EcoreUtils.addListener(list, _listener, -1);
        }
        getTreeViewer().setInput(_elist);
    }
    /**
     * Removes the attached listeners
     *
     */
    public void removeListeners()
    {
    	EcoreUtils.removeListener(_elist, _listener, -1);
    }
    /**
     * @param event - DoubleClickEvent
     * Set selection on the object if double clicked
     */
	public void doubleClick(DoubleClickEvent event)
	{
		IStructuredSelection sel = (IStructuredSelection) event.getSelection();
		if (sel.getFirstElement() instanceof TreeNode) {
			TreeNode selNode = (TreeNode) sel.getFirstElement();
			Object val = selNode.getValue();
			if (val instanceof EObject && selNode.getFeature() == null) {
				GenericEditorInput geInput = (GenericEditorInput)
					_editor.getValue("input");
				if (geInput != null && geInput.getEditor().
						getEditorModel() != null) {
                    NodeModel nodeModel = geInput.getEditor().
                        getEditorModel().getNodeModel(EcoreUtils.
                                getName((EObject) val));
                    if (nodeModel != null) {
                        nodeModel.setSelection();
                        nodeModel.setFocus();
                        
                    }
                }
			}
		}
		
	}
}
