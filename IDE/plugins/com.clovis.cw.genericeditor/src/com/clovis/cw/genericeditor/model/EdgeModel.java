/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor.model;


import java.util.ArrayList;
import java.util.List;

import org.eclipse.draw2d.Bendpoint;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

/**
 * @author pushparaj
 * 
 * TODO To change the template for this generated type comment go to Window -
 * Preferences - Java - Code Style - Code Templates
 */
public class EdgeModel extends IElement {

	private EObject _eObject;
	private NodeModel m_source;
	private NodeModel m_target;
	private String _className;

	/**
	 * constructor
	 * 
	 * @param value
	 */
	public EdgeModel(String value) {
		_className = value;
	}

	/**
	 * set the name.
	 * 
	 * @param value
	 */
	public void setName(String value) {
		_className = value;
	}

	/**
	 * returns name.
	 * 
	 * @return
	 */
	public String getName() {
		return _className;
	}

	/**
	 * constructor
	 *  
	 */
	public EdgeModel() {

	}

	/**
	 * set the source node.
	 * 
	 * @param iSource
	 */
	public void setSource(NodeModel iSource) {
		m_source = iSource;
	}

	/**
	 * set the target node.
	 * 
	 * @param iTarget
	 */
	public void setTarget(NodeModel iTarget) {
		m_target = iTarget;
	}

	/** Gets the connection source. */
	public NodeModel getSource() {
		return m_source;
	}

	/** Gets the connection target. */
	public NodeModel getTarget() {
		return m_target;
	}

	/**
	 * List of bendpoint model object associated with current connection model
	 * object.
	 */
	protected List bendpoints = new ArrayList();

	/** Returns the list of bendpoints model objects. */
	public List getBendpoints() {
		return bendpoints;
	}
	
	/**
	 * Inserts a bendpoint.
	 */
	public void insertBendpoint(int index, Bendpoint point) {
		getBendpoints().add(index, point);
		_listeners.firePropertyChange("bendpoint", null, null);
	}

	/**
	 * Removes a bendpoint.
	 */
	public void removeBendpoint(int index) {
		getBendpoints().remove(index);
		_listeners.firePropertyChange("bendpoint", null, null);
	}

	/**
	 * Sets another location for an existing bendpoint.
	 */
	public void setBendpoint(int index, Bendpoint point) {
		getBendpoints().set(index, point);
		_listeners.firePropertyChange("bendpoint", null, null);
	}

	/**
	 * 
	 *  
	 */
	public void attachSource() {
		if (getSource() == null)
			return;
		getSource().connectInput(this);
		_listeners.firePropertyChange("attachsource", null, null);
	}

	/**
	 * 
	 *  
	 */
	public void attachTarget() {
		if (getTarget() == null)
			return;
		getTarget().connectOutput(this);
		_listeners.firePropertyChange("attachtarget", null, null);
	}

	/**
	 * 
	 *  
	 */
	public void detachSource() {
		if (getSource() == null)
			return;
		getSource().disconnectInput(this);
	}

	/**
	 * 
	 *  
	 */
	public void detachTarget() {
		if (getTarget() == null)
			return;
		getTarget().disconnectOutput(this);
	}

	/** Source terminal name. */
	protected String m_sourceTerminal = "31";

	/** Target terminal name. */
	protected String m_targetTerminal = "E";
	
	/** Source terminal name. */
	protected String _oldSourceTerminal = null;

	/** Target terminal name. */
	protected String _oldTargetTerminal = null;

	/**
	 * Sets the name of the source terminal.
	 */
	public void setSourceTerminal(String s) {
		if(_oldSourceTerminal == null)
			setOldSourceTerminal(s);
		else
			setOldSourceTerminal(m_sourceTerminal);
		m_sourceTerminal = s;
	}
	/**
	 * Sets the name of the old source terminal.
	 */
	public void setOldSourceTerminal(String s) {
		_oldSourceTerminal = s;
	}
	/**
	 * Sets the name of the target terminal.
	 */
	public void setTargetTerminal(String s) {
		if(_oldTargetTerminal == null)
			setOldTargetTerminal(s);
		else
			setOldTargetTerminal(m_targetTerminal);
		m_targetTerminal = s;
	}
	/**
	 * Sets the name of the old target terminal.
	 */
	public void setOldTargetTerminal(String s) {
		_oldTargetTerminal = s;
	}
	/** Gets source terminal name. */
	public String getSourceTerminal() {
		return m_sourceTerminal;
	}
	/** Gets old source terminal name. */
	public String getOldSourceTerminal() {
		return _oldSourceTerminal;
	}
	/** Gets target terminal name. */
	public String getTargetTerminal() {
		return m_targetTerminal;
	}
	/** Gets old target terminal name. */
	public String getOldTargetTerminal() {
		return _oldTargetTerminal;
	}
	/** Sets EObject instance. */
	public void setEObject(EObject obj) {
		_eObject = obj;
	}

	/** Gets intstance of EObject */
	public EObject getEObject() {
		return _eObject;
	}

	/** Sets EObject propery value * */
	public void setEPropertyValue(Object property, Object value) {
        if (_eObject == null)
			return;

		EStructuralFeature attr = _eObject.eClass().getEStructuralFeature(String.valueOf(property));
		if (attr != null) {
        	_eObject.eSet(attr, value);
            //firePropertyChange(String.valueOf(property), null, value);
		}
	}

	/** Gets EObject propery value * */
	public Object getEPropertyValue(Object property) {
		if (_eObject == null)
			return null;

		EStructuralFeature attr = _eObject.eClass().getEStructuralFeature(
				String.valueOf(property));
		if (attr != null)
			return _eObject.eGet(attr);
		return null;
	}

}
