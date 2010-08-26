/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/EditorModelValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.ModelValidator;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.common.utils.ui.factory.ValidatorFactory;

/**
 * @author pushparaj
 *
 * Validator for Editor
 */
public abstract class EditorModelValidator extends ModelValidator
{
	protected Shell _shell;
	public EditorModelValidator(Model model, Shell shell)
	{
		super(model);
		_shell = shell;
	}
	/**
	 * Returns true if connection is valid
	 * @param obj Connection's EObject
	 * @return boolean
	 */
	public abstract String isConnectionValid(EObject sourceObj,
            EObject targetObj, EObject eobj);
	
	public Shell getShell() 
	{
		return _shell;
	}
    /**
     *@param eobj EObject
     * @return true if the model is valid else return false.
     */
    public boolean isModelValid(EObject eobj)
    {
        return isModelValid(eobj, null);
    }
    /**
     *@param eobj EObject
     * @return true if the model is valid else return false.
     */
    public boolean isModelValid(EObject eobj, Notification n)
    {
        String message = isValid(eobj, n);
        if (message != null) {
            setValid(false);
            setMessage(message);
            return false;
        } else {
            setValid(true);
            setMessage("");
        }
        return true;
    }
    /**
     * initialize the map
     */
    protected void initMap()
    {
    	EObject rootObject = (EObject) _elist.get(0);
		List refList = rootObject.eClass().getEAllReferences();
        for (int i = 0; i < refList.size(); i++) {
            EList list = (EList) rootObject.eGet((EReference) refList.get(i));
            for( int j = 0; j < list.size(); j++) {
            	EObject eobj = (EObject) list.get(j);
            	String cwkey = (String) EcoreUtils.getValue(eobj,
            			ModelConstants.RDN_FEATURE_NAME);
            	_cwkeyObjectMap.put(cwkey, eobj);
            }
        }

    }
    /**
    *
    * @param eobj
    *            EObject which is modified or newly added to list
    * @return true if the EObject is valid else else false
    */
   public String isValid(EObject eobj, Notification n) {
		String message = null;
		ObjectValidator validator = ValidatorFactory.getValidatorFactory()
				.getValidator(eobj.eClass());
		if (validator != null) {
			EObject rootObject = (EObject) _elist.get(0);
			List refList = rootObject.eClass().getEAllReferences();
			for (int i = 0; i < refList.size(); i++) {
				EList list = (EList) rootObject.eGet((EReference) refList
						.get(i));
				message = validator.isValid(eobj, list, n);
				if(message != null)
					return message;
			}
		}
		return message;
	}
}
