/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.commands;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.commands.Command;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.cw.genericeditor.Messages;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;


/**
 *
 * @author pushparaj
 *
 * This class handles all connection related requests
 */
public class ConnectionCommand extends Command
{
    private NodeModel _oldSource;
    private String _oldSourceTerminal;
    private NodeModel _oldTarget;
    private String _oldTargetTerminal;
    private NodeModel _sourceBase;
    private String _sourceTerminal;
    private NodeModel _targetSource;
    private String _targetTerminal;
    protected EdgeModel _edgeModel;
    private boolean _canUndo;
    
    /**
     * constructor
     */
    public ConnectionCommand()
    {
        super(Messages.CONNECTIONCOMMAND_LABEL);
    }

    /**
     * @see org.eclipse.gef.commands.Command#canExecute()
     */
    public boolean canExecute()
    {
        return true;
    }

    /**
     * @see org.eclipse.gef.commands.Command#execute()
     */
    public void execute()
    {
    	_canUndo = true;
    	// This code needs to be cleaned
        if (_sourceBase != null && (_targetSource == null || _sourceBase
                != _oldSource || _sourceTerminal != _oldSourceTerminal)) {
            // if source connection change and redo
        	EObject sourceObj = _sourceBase.getEObject();
        	EObject targetObj = null;
        	if(_edgeModel.getTarget() != null) {
        		targetObj = _edgeModel.getTarget().getEObject();
        	}
        	String message = null;
        	if (sourceObj != null && targetObj != null) {
					message = _sourceBase.getRootModel()
							.getEditorModelValidator().isConnectionValid(
									sourceObj, targetObj,
									_edgeModel.getEObject());
			}
        	if (message == null) {
				_edgeModel.detachSource();
				_edgeModel.setSource(_sourceBase);
				_edgeModel.setSourceTerminal(_sourceTerminal);
				_edgeModel.attachSource();
			} else {
				_canUndo = false;
				MessageDialog.openError(_edgeModel.getTarget().getRootModel()
						.getEditorModelValidator().getShell(),
						"Connection Error", message);
			}
        }
        if (_targetSource != null && (_sourceBase == null || _oldTarget
                != _targetSource || _targetTerminal != _oldTargetTerminal)) {
            // if target connection change and redo
        	EObject sourceObj = null;
        	EObject targetObj = _targetSource.getEObject();
        	if(_edgeModel.getSource() != null) {
        		sourceObj = _edgeModel.getSource().getEObject();
        	}
        	String message = null;
        	if (sourceObj != null && targetObj != null && _oldTarget != null ) {
				message = _targetSource.getRootModel()
						.getEditorModelValidator().isConnectionValid(
								sourceObj, targetObj,
								_edgeModel.getEObject());
        	}
        	if (message == null) {
				_edgeModel.detachTarget();
				_edgeModel.setTarget(_targetSource);
				_edgeModel.setTargetTerminal(_targetTerminal);
				_edgeModel.attachTarget();
			} else {
				_canUndo = false;
				MessageDialog.openError(_edgeModel.getSource().getRootModel()
						.getEditorModelValidator().getShell(),
						"Connection Error", message);
			}
        }
        if ((_oldSource == null && _oldTarget == null)
				&& (_sourceBase != null && _targetSource != null)) {
			// if connection add snd redo
			_edgeModel.detachSource();
			_edgeModel.setSource(_sourceBase);
			_edgeModel.setSourceTerminal(_sourceTerminal);
			// _edgeModel.attachSource();
			_edgeModel.detachTarget();
			_edgeModel.setTarget(_targetSource);
			_edgeModel.setTargetTerminal(_targetTerminal);
			// _edgeModel.attachTarget();

			String message = _sourceBase
					.getRootModel()
					.getEditorModelValidator()
					.isConnectionValid(_sourceBase.getEObject(),
							_targetSource.getEObject(), _edgeModel.getEObject());
			if (message == null) {
				_sourceBase.getRootModel().addEObject(
						_edgeModel.getEObject());
			} else {
				_canUndo = false;
				MessageDialog.openError(_sourceBase.getRootModel()
						.getEditorModelValidator().getShell(),
						"Connection Error", message);
			}
		}
        if (_sourceBase == null && _targetSource == null && _oldSource != null
                && _oldTarget != null) {
            // if connection delete and redo
             //_edgeModel.detachSource();
            //_edgeModel.detachTarget();
             _edgeModel.getSource().getRootModel().removeEObject(
                     _edgeModel.getEObject());
            _edgeModel.setTarget(null);
            _edgeModel.setSource(null);
        }
     }

    /**
     * returns label
     * @return label
     */
    public String getLabel()
    {
        return Messages.CONNECTIONCOMMAND_DESCRIPTION;
    }

    /**
     * returns source node for this connection.
     * @return _sourceBase
     */
    public NodeModel getSource()
    {
        return _sourceBase;
    }

    /**
     * returns source terminal.
     * @return _sourceTerminal
     */
    public java.lang.String getSourceTerminal()
    {
        return _sourceTerminal;
    }

    /**
     * returns target node for this connection.
     * @return _targetSource
     */
    public NodeModel getTarget()
    {
        return _targetSource;
    }

    /**
     * return target terminal.
     * @return _targetTerminal
     */
    public String getTargetTerminal()
    {
        return _targetTerminal;
    }

    /**
     * returns connection model.
     * @return _edgeModel
     */
    public EdgeModel getConnectionModel()
    {
        return _edgeModel;
    }

    /**
     * @see org.eclipse.gef.commands.Command#redo()
     */
    public void redo()
    {
        execute();
    }

    /**
     * set source node for this connection.
     * @param newSource source node
     */
    public void setSource(NodeModel newSource)
    {
        _sourceBase = newSource;
    }

    /**
     * set source terminal for this connection.
     * @param newSourceTerminal source anchor point
     */
    public void setSourceTerminal(String newSourceTerminal)
    {
        _sourceTerminal = newSourceTerminal;
    }

    /**
     * set target node for this connection.
     * @param newTarget target node
     */
    public void setTarget(NodeModel newTarget)
    {
        _targetSource = newTarget;
    }

    /**
     * set target terminal for this connection.
     * @param newTargetTerminal target anchor point
     */
    public void setTargetTerminal(String newTargetTerminal)
    {
        _targetTerminal = newTargetTerminal;
    }

    /**
     * set model for this connection.
     * @param w EdgeModel
     */
    public void setConnectionModel(EdgeModel w)
    {
        _edgeModel = w;
        _oldSource = w.getSource();
        _oldTarget = w.getTarget();
        _oldSourceTerminal = w.getSourceTerminal();
        _oldTargetTerminal = w.getTargetTerminal();
    }

    /**
     * @see org.eclipse.gef.commands.Command#undo()
     */
    public void undo()
    {
        _sourceBase = _edgeModel.getSource();
        _targetSource = _edgeModel.getTarget();
        _sourceTerminal = _edgeModel.getSourceTerminal();
        _targetTerminal = _edgeModel.getTargetTerminal();

        if (_oldSource != null && _sourceBase != null && (_oldSource
                != _sourceBase || _oldSourceTerminal != _sourceTerminal)) {
            // if source connection change and undo
            _edgeModel.detachSource();
            _edgeModel.setSource(_oldSource);
            _edgeModel.setSourceTerminal(_oldSourceTerminal);
            _edgeModel.attachSource();
        }
        if (_oldTarget != null && _targetSource != null && (_oldTarget
                != _targetSource || _oldTargetTerminal != _targetTerminal)) {
            // if target connection change and undo
            _edgeModel.detachTarget();
            _edgeModel.setTarget(_oldTarget);
            _edgeModel.setTargetTerminal(_oldTargetTerminal);
            _edgeModel.attachTarget();
        }
        if (_sourceBase == null && _targetSource == null && _oldSource != null
                && _oldTarget != null) {
            // if connection add and undo
            _edgeModel.setSource(_oldSource);
            _edgeModel.setTarget(_oldTarget);
            _edgeModel.setSourceTerminal(_oldSourceTerminal);
            _edgeModel.setTargetTerminal(_oldTargetTerminal);
            _oldSource.getRootModel().addEObject(_edgeModel.getEObject());
        }
        if (_oldSource == null && _oldTarget == null && _sourceBase != null
                && _targetSource != null) {
            // if connection delete and undo
            _sourceBase.getRootModel().removeEObject(
                    _edgeModel.getEObject());
            _edgeModel.setSource(null);
            _edgeModel.setTarget(null);
        }
    }
    
    public boolean canUndo()
    {
    	return _canUndo;
    }
}
