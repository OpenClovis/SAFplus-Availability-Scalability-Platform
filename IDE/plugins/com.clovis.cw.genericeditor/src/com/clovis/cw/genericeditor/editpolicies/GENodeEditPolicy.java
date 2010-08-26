/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.editpolicies;


import org.eclipse.draw2d.ConnectionAnchor;
import org.eclipse.gef.GraphicalEditPart;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.requests.CreateConnectionRequest;
import org.eclipse.gef.requests.ReconnectRequest;

import com.clovis.cw.genericeditor.commands.ConnectionCommand;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.figures.AbstractNodeFigure;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * This class will Creates all connection related commands
 * based on the request
 */
public class GENodeEditPolicy
    extends org.eclipse.gef.editpolicies.GraphicalNodeEditPolicy
{

    /**
     * @see org.eclipse.gef.editpolicies.GraphicalNodeEditPolicy
     * #getConnectionCompleteCommand(
     * org.eclipse.gef.requests.CreateConnectionRequest)
     */
    protected Command getConnectionCompleteCommand(
            CreateConnectionRequest request)
    {
        //System.out.println("Connection Complete Command"+request.toString());
        ConnectionCommand command =
            (ConnectionCommand) request.getStartCommand();
        command.setTarget(getBase());
        ConnectionAnchor ctor =
            getBaseEditPart().getTargetConnectionAnchor(request);
        if (ctor == null) {
            return null;
        }
        command.setTargetTerminal(
            getBaseEditPart().mapConnectionAnchorToTerminal(ctor));
        return command;
    }

    /**
     * @see org.eclipse.gef.editpolicies.GraphicalNodeEditPolicy
     * #getConnectionCreateCommand(
     * org.eclipse.gef.requests.CreateConnectionRequest)
     */
    protected Command getConnectionCreateCommand(
            CreateConnectionRequest request)
    {
        //System.out.println("Connection Create Command"+request.toString());
        ConnectionCommand command = new ConnectionCommand();
        command.setConnectionModel((EdgeModel) request.getNewObject());
        command.setSource(getBase());
        ConnectionAnchor ctor =
            getBaseEditPart().getSourceConnectionAnchor(request);
        command.setSourceTerminal(
            getBaseEditPart().mapConnectionAnchorToTerminal(ctor));
        request.setStartCommand(command);
        return command;
    }

    /**
     * returns editpart
     * @return editpart EditPart
     */
    protected BaseEditPart getBaseEditPart()
    {
        return (BaseEditPart) getHost();
    }

    /**
     * return node model.
     * @return node NodeModel
     */
    protected NodeModel getBase()
    {
        return (NodeModel) getHost().getModel();
    }

    /**
     * @see org.eclipse.gef.editpolicies.GraphicalNodeEditPolicy
     * #getReconnectTargetCommand(
     * org.eclipse.gef.requests.ReconnectRequest)
     */
    protected Command getReconnectTargetCommand(ReconnectRequest request)
    {
        //System.out.println("Connection Reconnect Command"+request.toString());
        ConnectionCommand cmd = new ConnectionCommand();
        cmd.setConnectionModel(
            (EdgeModel) request.getConnectionEditPart().getModel());

        ConnectionAnchor ctor =
            getBaseEditPart().getTargetConnectionAnchor(request);
        cmd.setTarget(getBase());
        cmd.setTargetTerminal(
            getBaseEditPart().mapConnectionAnchorToTerminal(ctor));
        return cmd;
    }

    /**
     * @see org.eclipse.gef.editpolicies.GraphicalNodeEditPolicy
     * #getReconnectSourceCommand(org.eclipse.gef.requests.ReconnectRequest)
     */
    protected Command getReconnectSourceCommand(ReconnectRequest request)
    {
        //System.out.println("Connection Reconnect Command"+request.toString());
        ConnectionCommand cmd = new ConnectionCommand();
        cmd.setConnectionModel(
            (EdgeModel) request.getConnectionEditPart().getModel());

        ConnectionAnchor ctor =
            getBaseEditPart().getSourceConnectionAnchor(request);
        cmd.setSource(getBase());
        cmd.setSourceTerminal(
            getBaseEditPart().mapConnectionAnchorToTerminal(ctor));
        return cmd;
    }

    /**
     * return node fgure.
     * @return figure
     */
    protected AbstractNodeFigure getNodeFigure()
    {
        return (AbstractNodeFigure) ((GraphicalEditPart) getHost()).getFigure();
    }
}
