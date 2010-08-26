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

import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CompoundCommand;
import org.eclipse.gef.ui.actions.Clipboard;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.actions.ActionFactory;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.commands.DeleteCommand;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * Class for Handling Cut action
 */
public class GECutAction extends AbstractCopyAction
{

    /**
     * @param editor Editor part
     */
    public GECutAction(IEditorPart editor)
    {
        super(editor);
    }

    /**
     * @see org.eclipse.gef.ui.actions.EditorPartAction#init()
     */
    protected void init()
    {
        setId(ActionFactory.CUT.getId());
        setText(Messages.CUTACTION_LABEL);
        setImageDescriptor(ImageDescriptor.createFromFile(getClass(),
		"icons/ecut_edit.gif"));
    }

    /**
     * Sets the default {@link Clipboard Clipboard's}contents to be the
     * currently selected template.
     */
    public void run()
    {
    	setEditorModel(_editorPart.getEditorModel());
    	List nodes = new ArrayList();
        nodes.addAll(getSelectedObjects());
        List models = new ArrayList();
        CompoundCommand compound = new CompoundCommand();
        List parentsToBeRemoved = new ArrayList();
		for (int i = 0; i < nodes.size(); i++) {
			if((nodes.get(i) instanceof BaseEditPart)) {
				BaseEditPart editpart = (BaseEditPart) nodes.get(i);
				if(!(editpart.getParent() instanceof BaseDiagramEditPart)) {
					ContainerNodeModel nodeModel = (ContainerNodeModel) editpart.getParent().getModel();
					EObject nodeObj = nodeModel.getEObject();
					String value = EcoreUtils.getAnnotationVal(nodeObj.eClass(), null, "removeIfNoChild");
					if(value != null && value.equals("true")) {
						parentsToBeRemoved.add(editpart.getParent());
					}
				}
			}
		}
		for(int i = 0; i < parentsToBeRemoved.size(); i++) {
			EditPart parentPart = (EditPart) parentsToBeRemoved.get(i);
			if(nodes.containsAll(parentPart.getChildren())) {
				nodes.removeAll(parentPart.getChildren());
				nodes.add(parentPart);
			}
		}
        copyNodes(nodes, models, compound);
        copyEdges(nodes, models);
        Clipboard.getDefault().setContents(models);
        execute(compound);
    }

    /**
     * Create cloned node objects and added into List. This list will be
     * maintained in Cipboard. Also creates DeleteCommands for all selected
     * nodes.
     *
     * @param parts selected EditParts
     * @param models clonedList
     * @param compound Delete Command
     */
    private void copyNodes(List parts, List models, CompoundCommand compound)
    {
        for (int i = 0; i < parts.size(); i++) {
        	if (parts.get(i) instanceof BaseEditPart) {
				BaseEditPart childPart = (BaseEditPart) parts.get(i);
				BaseEditPart parentPart = (BaseEditPart) childPart.getParent();
				compound.add(createDeleteCommand(parentPart.getModel(),
						childPart.getModel()));
				NodeModel childNode = createNode(parentPart, childPart);
				ContainerNodeModel parentNode = (ContainerNodeModel) _copyMap
						.get(parentPart.getModel());
				if (parentNode == null) {
					parentNode = _editorModel;
				}
				childNode.setParent(parentNode);
				models.add(childNode);
				copyNodes(childPart.getChildren(), models, compound);
			}
        }
    }

    /**
	 * Create and return DeleteCommand for Node.
	 * 
	 * @param parent
	 *            ContainerModel
	 * @param child
	 *            NodeModel
	 * @return DeleteCommand for NodeModel
	 */
    private Command createDeleteCommand(Object parent, Object child)
    {
        DeleteCommand del = new DeleteCommand();
        del.setParent((ContainerNodeModel) parent);
        del.setChild((NodeModel) child);
        return del;
    }

}
