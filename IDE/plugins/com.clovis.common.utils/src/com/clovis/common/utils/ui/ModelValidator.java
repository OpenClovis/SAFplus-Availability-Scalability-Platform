/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/ModelValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui;

import java.util.HashMap;
import java.util.List;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.AbstractValidator;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.factory.ValidatorFactory;

/**
 * @author shubhada
 *
 * Does specific validations like Duplication checking and Blank Value checking
 */
public class ModelValidator extends AbstractValidator
{
    protected Model   _model          = null;
    protected List   _elist          = null;
    protected EClass  _eClass        = null;
    protected HashMap _cwkeyObjectMap = new HashMap();
    
    /**
     * Empty Constructor
     */
    public ModelValidator()
    {
    	
    }
    /**
     *
     * @param model -
     *            Model
     */
    public ModelValidator(Model model)
    {
       this(model, 2);
    }
    /**
    *
    * @param model - Model
    * @param depth -  Depth at which listener to be attached
    */
   public ModelValidator(Model model, int depth)
   {
       _model = model;
       _elist = _model.getEList();
       EcoreUtils.addListener((NotifyingList) _elist, this, depth);
       initMap();
   }
    /**
     * initialize the map
     */
    protected void initMap()
    {
        for (int i = 0; i < _elist.size(); i++) {
            EObject eobj = (EObject) _elist.get(i);
            String cwkey = (String) EcoreUtils.getValue(eobj,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eobj);
        }

    }
    /**
    *
    * @param eobj
    *            EObject which is modified or newly added to list
    * @return true if the EObject is valid else else false
    */
   public String isValid(EObject eobj)
   {
       return isValid(eobj, null);
   }
   /**
   *
   * @param eobj
   *            EObject which is modified or newly added to list
   * @return true if the EObject is valid else else false
   */
  public String isValid(EObject eobj, Notification n)
  {
      String message = null;
      ObjectValidator validator = ValidatorFactory.
       getValidatorFactory().getValidator(eobj.eClass());
      if (validator != null) {
		  message = validator.isValid(eobj, _elist, n);
      }
      return message;
  }
    /**
     * @param notification -
     *            Notification
     */
    public void notifyChanged(Notification notification)
    {
        switch (notification.getEventType()) {
        case Notification.REMOVING_ADAPTER:
            break;
        case Notification.SET:
            Object object = notification.getNotifier();
            if (!notification.isTouch()
                    && object instanceof EObject) {
            isValid((EObject) object, notification);
            }
            break;
        case Notification.ADD:
            Object newVal = notification.getNewValue();
            if (newVal instanceof EObject) {
                EObject obj = (EObject) newVal;
                String cwkey = (String) EcoreUtils.getValue(obj,
                		ModelConstants.RDN_FEATURE_NAME);
                _cwkeyObjectMap.put(cwkey, obj);
                EcoreUtils.addListener(obj, this, 1);
                isValid(obj, notification);
            }
            break;

        case Notification.ADD_MANY:
            List objs = (List) notification.getNewValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject eObj = (EObject) objs.get(i);
                    String key = (String) EcoreUtils.getValue(eObj,
                    		ModelConstants.RDN_FEATURE_NAME);
                    _cwkeyObjectMap.put(key, eObj);
                    EcoreUtils.addListener(eObj, this, 1);
                   isValid(eObj, notification);
                }
            }
            break;
        case Notification.REMOVE:
            Object obj = notification.getOldValue();
            if (obj instanceof EObject) {
                EObject ob = (EObject) obj;
                String rkey = (String) EcoreUtils.getValue(ob,
                		ModelConstants.RDN_FEATURE_NAME);
                _cwkeyObjectMap.remove(rkey);
                EcoreUtils.removeListener(ob, this, 1);
            }
            break;
        case Notification.REMOVE_MANY:
            objs = (List) notification.getOldValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject o = (EObject) objs.get(i);
                    String rk = (String) EcoreUtils.getValue(o,
                    		ModelConstants.RDN_FEATURE_NAME);
                    _cwkeyObjectMap.remove(rk);
                    EcoreUtils.removeListener(o, this, 1);
                }
            }
            break;
        }
    }
    /**
     * removes the listeners attached to it.
     *
     */
    public void removeListeners()
    {
        EcoreUtils.removeListener((NotifyingList) _elist, this, 2);
    }
    
}
