/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/AbstractValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ecore;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
/**
 * @author shubhada
 *
 * Abstract validator, should be integrated with UI to show erros
 * to users.
 */
public abstract class AbstractValidator extends AdapterImpl
{
    protected String  _message;
    protected boolean _isValid = true;
    /**
     * Is Valid
     * @return true for valid state.
     */
    public boolean isValid()
    {
        return _isValid;
    }
    /**
     * Set validdity of model.
     * @param valid valid status
     */
    public void setValid(boolean valid)
    {
        _isValid = valid;
    }
    /**
     * Get Message
     * @return Message
     */
    public String getMessage()
    {
        return _message;
    }
    /**
     * Set Message
     * @param message -  Message
     */
    public void setMessage(String message)
    {
        _message = message;
    }
    /**
     * Handles notification. Subclasses should implement
     * this to check if the model is valid.
     * @param msg Change Notification Message
     */
    public abstract void notifyChanged(Notification msg);
}
