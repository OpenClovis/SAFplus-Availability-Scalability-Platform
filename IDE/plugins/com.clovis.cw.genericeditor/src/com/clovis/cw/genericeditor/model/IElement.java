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

import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;
import java.io.Serializable;


/**
 * 
 * @author pushparaj
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public abstract class IElement
	implements Cloneable, Serializable {
	
	private boolean      _isCollapsedElement;
	
	/**
	 * 
	 * @param name
	 */
	protected abstract void setName(String name);
	
	/**
	 * 
	 * @return
	 */
	protected abstract String getName();
	
	/**
	 * 
	 */
	protected PropertyChangeSupport _listeners =
		new PropertyChangeSupport(this);
	
	/**
	 * 
	 * @param l
	 */
	public void addPropertyChangeListener(PropertyChangeListener l) {
		_listeners.addPropertyChangeListener(l);
	}
	
	/**
	 * 
	 * @param l
	 */
	public void removePropertyChangeListener(PropertyChangeListener l) {
		_listeners.removePropertyChangeListener(l);
	}
	/**
     * Set flag for collapse and expand
     * @param collapse flag for expand and collapse
     */
    public void setCollapsedElement(boolean collapse)
    {
    	_isCollapsedElement = collapse;
    }
    /**
     * Returnns flag for exapnd and collapse
     * @return flag
     */
    public boolean isCollapsedElement()
    {
    	return _isCollapsedElement;
    }
}
