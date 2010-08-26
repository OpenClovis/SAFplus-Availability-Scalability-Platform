/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/factory/ValidatorFactory.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.factory;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Vector;

import org.eclipse.emf.ecore.EClass;
import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.ObjectValidator;

/**
 *
 * @author shubhada
 *Validator Factory which creates the Validator per EClass
 */
public class ValidatorFactory
{
    private HashMap _eclassInstanceMap = new HashMap();
    private HashMap _eclassFeaturesMap = new HashMap();
    private static ValidatorFactory validatorFactory = null;
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());

    /**
     *
     * @return static instance of the ValidatorFactory
     */
    public static ValidatorFactory getValidatorFactory()
    {
        if (validatorFactory == null) {
            validatorFactory =  new ValidatorFactory();
            return validatorFactory;
        } else {
            return validatorFactory;
        }
    }
    /**
     * Maintains the validator instances per eClass and returns them.
     * If not found in map, creates new instance
     * @param eClass Eclass
     * @return Validator istance per EClass
     */
    public ObjectValidator getValidator(EClass eClass)
    {
        ObjectValidator validator = (ObjectValidator) _eclassInstanceMap.
            get(eClass);
        if (validator == null) {
            initFeatureNames(eClass);
            String custom =
                EcoreUtils.getAnnotationVal(eClass, null, "validator");
            if (custom != null) {
                return getCustomValidator(custom, (Vector) _eclassFeaturesMap.
                        get(eClass));
            } else {
                validator = new ObjectValidator((Vector) _eclassFeaturesMap.
                        get(eClass));
                _eclassInstanceMap.put(eClass, validator);
                return validator;
            }
        } else {
            return validator;
        }

    }
    /**
     * Get Custom Validator for given class.
     * @param customValidator Name of Validator class.
     * @param featureNames - creates custom validator
     * @return Custom Validator (null in case of errors)
     */
    private ObjectValidator getCustomValidator(
            String customValidator, Vector featureNames)
    {
        ObjectValidator validator = null;
        Class validatorClass = ClovisUtils.loadClass(customValidator);
        try {
            Class[] argType = {
                    Vector.class
            };
            if (validatorClass != null) {
                Method met  = validatorClass.
                    getMethod("createValidator", argType);
                Object[] args = {featureNames};
                validator = (ObjectValidator) met.invoke(null, args);
            }
        } catch (NoSuchMethodException e) {
            LOG.error("Custom editor does not have createValidator() method", e);
        } catch (Throwable th) {
            LOG.
            error("Unhandled error while creating validator:" + customValidator, th);
        }
        return validator;
    }
    /**
     * get the feature names to be validated from the Annotation
     * @param eClass EClass
     *
     */
    private void initFeatureNames(EClass eClass)
    {
        Vector featureNames = new Vector();
        if (eClass != null) {
            String featureName = EcoreUtils.getAnnotationVal(
                    eClass, null, "validationfeatures");
            if (featureName != null) {
                int index = featureName.indexOf(',');
                while (index != -1) {
                    String feature = featureName.substring(0, index);
                    featureNames.add(feature);
                    featureName = featureName.substring(index + 1);
                    index = featureName.indexOf(',');
                }

                featureNames.add(featureName);
            }
            _eclassFeaturesMap.put(eClass, featureNames);
        }

    }
}
