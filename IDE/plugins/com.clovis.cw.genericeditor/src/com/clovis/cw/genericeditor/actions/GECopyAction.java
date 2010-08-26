/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.actions;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.gef.ui.actions.Clipboard;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.actions.ActionFactory;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Class fo handling Copy Action
 */
public class GECopyAction extends AbstractCopyAction
{

    /**
     * @param editor Editor part
     */
    public GECopyAction(IEditorPart editor)
    {
        super(editor);
    }

    /**
     * @see org.eclipse.gef.ui.actions.EditorPartAction#init()
     */
    protected void init()
    {
        setId(ActionFactory.COPY.getId());
        setText(Messages.COPYACTION_LABEL);
        setImageDescriptor(ImageDescriptor.createFromFile(getClass(),
				"icons/ecopy_edit.gif"));
    }

    /**
     * Sets the default {@link Clipboard Clipboard's}contents to be the
     * currently selected parts.
     */
    public void run()
    {
        setEditorModel(_editorPart.getEditorModel());
        List nodes = this.getSelectedObjects();
        List cmd = new ArrayList();
        copyNodes(nodes, cmd);
        copyEdges(nodes, cmd);
        Clipboard.getDefault().setContents(cmd);
    }

    /**
     * Create cloned node objects and added into List. This list will be
     * maintained in Cipboard.
     *
     * @param parts selected objects
     * @param copyList clonedList
     */
    private void copyNodes(List parts, List copyList)
    {
        for (int i = 0; i < parts.size(); i++) {
        	if (parts.get(i) instanceof BaseEditPart) {
	            BaseEditPart childPart = (BaseEditPart) parts.get(i);
	            BaseEditPart parentPart = (BaseEditPart) childPart.getParent();
	            NodeModel childNode = createNode(parentPart, childPart);
	            ContainerNodeModel parentNode = (ContainerNodeModel) _copyMap
	                    .get(parentPart.getModel());
	            if (parentNode == null) {
	                parentNode = _editorModel;
	            }
	            childNode.setParent(parentNode);
	            copyList.add(childNode);
	            copyNodes(childPart.getChildren(), copyList);
        	}
        }
    }

}
