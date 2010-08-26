/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/ObjectValidator.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.regex.Pattern;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;

/**
 *
 * @author shubhada
 * Validator at Object Level
 */
public class ObjectValidator
{
    protected Vector  _featureNames   = new Vector();
    /**
     * constructor
     * @param featureNames - Feature Names to be validated
     */
    public ObjectValidator(Vector featureNames)
    {
        _featureNames = featureNames;
    }
    /**
    *
    * @param features -
    *            Name of the features on which u want to do validation
    */
   public void setValidationFeature(String[] features)
   {
       for (int i = 0; i < features.length; i++) {
           _featureNames.add(features[i]);
       }
   }
   /**
    *
    * @param eobj EObject
    * @param eList EList
    * @return message
    */
   public String isValid(EObject eobj, List eList)
   {
      return isValid(eobj, eList, null);
   }
   /**
   *
   * @param eobj EObject
   * @param eList EList
   * @return message
   */
  public String isValid(EObject eobj, List eList, Notification n)
  {
      String message = null;
      message = checkPatternAndBlankValue(eList);
      if (message == null) {
      message = isDuplicate(eobj, eList, n);
      }
      if (message == null) {
    	  message = isFieldCombinationDuplicate(eobj, eList, n);
      }
      if (message == null) {
          message = isNonBlankFeatureBlank(eobj);
      }
      return message;
  }
  /**
   * 
   * @param eobj - EObject to be validated
   * @param eList - List of objects
   * @return the List of errors encountered 
   */
  public List getAllErrors(EObject eobj, List eList)
  {
	  List errorList = new Vector();
	  String message = null;
	  message = checkPatternAndBlankValue(eList);
	  if (message != null) {
		  errorList.add(message);
	  }
	  message = null;
	  message = isDuplicate(eobj, eList, null);
	  if (message != null) {
		  errorList.add(message);
	  }
	  message = null;
	  message = isFieldCombinationDuplicate(eobj, eList, null);
	  if (message != null) {
		  errorList.add(message);
	  }
	  message = null;
	  message = isNonBlankFeatureBlank(eobj);
	  if (message != null) {
		  errorList.add(message);
	  }
	  return errorList;
  }
   /**
    * 
    * @param eobj
    * @return
    */
   protected String isNonBlankFeatureBlank(EObject eobj)
   {
	   List features = eobj.eClass().getEAllStructuralFeatures();
       for (int i = 0; i < features.size(); i++) {
          EStructuralFeature feature = (EStructuralFeature) features.get(i);
          Object val = eobj.eGet(feature);
          String isBlankAllowed =  EcoreUtils.getAnnotationVal(
                      feature, null, "isBlankAllowed");
          if (isBlankAllowed != null) {
              boolean isBlankValAllowed = Boolean.
              	parseBoolean(isBlankAllowed);
              if (!isBlankValAllowed
                  && (val.equals("") || val.equals(" "))) {
            	  String label = EcoreUtils.getAnnotationVal(
                          feature, null, "label");
            	  if (label != null) {
            		  return label + " cannot be blank";
            	  } else {
            		  return feature.getName() + " cannot be blank";
            	  }
              }
          }
       }
	return null;
}
/**
   *
   * @param eobj
   *            EObject which is modified or newly added to list
   * @param eList Elist
   * @param n - Notification Object
   * @return errorMessage if the feature value is duplicated in the list.
   * else return null
   */
  protected String isDuplicate(EObject eobj, List eList, Notification n)
  {
      
      for (int j = 0; j < _featureNames.size(); j++) {
          String featureName = (String) _featureNames.get(j);
          String name = null;
          if (eobj.eClass().getEStructuralFeature(featureName) != null) {
              name = EcoreUtils.getValue(eobj, featureName).toString();
          }
          EStructuralFeature feature = eobj.eClass().
              getEStructuralFeature(featureName);
          String featureLabel = EcoreUtils.getAnnotationVal(feature, null, "label");
          String label = featureLabel != null ? featureLabel: featureName;
          for (int i = 0; i < eList.size(); i++) {
              EObject obj = (EObject) eList.get(i);
              if (featureName != null && !eobj.equals(obj)) {
                  String objName = null;
                  if (obj.eClass().getEStructuralFeature(
                          featureName) != null) {
                  objName = EcoreUtils.getValue(obj, featureName).toString();
                  }
                  if (name != null && objName != null
                          && name.equals(objName)) {
                      if (n != null && n.getEventType() == Notification.SET) {
                          String oldVal = n.getOldStringValue();
                          if( oldVal != null && !oldVal.equals(""))
                          {
		                      EcoreUtils.setValue(eobj, featureName, oldVal);
                              MessageDialog.openError(null, "Validations", "Duplicate entry for " + label + " : '" + name + "'. " +
                                  label + " reverted");
		                      return null;
                          }
                      }
                      return "Duplicate entry for - " + label + " : " + name +
                      ".\n This has to be corrected in order to be able to save changes.";
                  }
              }
           }
      }
      return null;
  }

  	/**
	 * Checks whether the combination of multiple field is duplicate.
	 * 
	 * @param eobj
	 * @param eList
	 * @param n
	 * @return
	 */
	protected String isFieldCombinationDuplicate(EObject eobj, List eList,
			Notification n) {

		Object uniqueFieldCombinationAnnVal = EcoreUtils.getAnnotationVal(eobj
				.eClass(), null, "uniqueFieldCombination");
		if (uniqueFieldCombinationAnnVal == null) {
			return null;
		}

		List<String[]> uniqueFieldCombinationList = new ArrayList<String[]>();
		StringTokenizer uniqueFieldCombinationTokenizer = new StringTokenizer(
				uniqueFieldCombinationAnnVal.toString(), ";");

		while (uniqueFieldCombinationTokenizer.hasMoreTokens()) {
			uniqueFieldCombinationList.add(uniqueFieldCombinationTokenizer
					.nextToken().split(","));
		}

		Iterator<String[]> uniqueFieldCombinationIterator;
		for (int i = 0; i < eList.size(); i++) {

			EObject obj = (EObject) eList.get(i);
			if (obj.equals(eobj)) {
				continue;
			}

			uniqueFieldCombinationIterator = uniqueFieldCombinationList
					.iterator();
			while (uniqueFieldCombinationIterator.hasNext()) {

				String features[] = uniqueFieldCombinationIterator.next();
				String orgVal = null, cmpVal;
				boolean isDuplicate = true;
				int j = 0;

				for (; j < features.length; j++) {
					orgVal = EcoreUtils.getValue(eobj, features[j]).toString();
					cmpVal = EcoreUtils.getValue(obj, features[j]).toString();
					if (orgVal != null && cmpVal != null
							&& !orgVal.equals(cmpVal)) {
						isDuplicate = false;
						break;
					}
				}

				if (isDuplicate) {

					String fieldCombination = "";
					for (int k = 0; k < features.length; k++) {

						EStructuralFeature feature = eobj.eClass()
								.getEStructuralFeature(features[k]);
						String featureLabel = EcoreUtils.getAnnotationVal(
								feature, null, "label");
						String label = featureLabel != null ? featureLabel
								: features[k];

						fieldCombination += label + " - ";
					}
					fieldCombination = fieldCombination.substring(0,
							fieldCombination.length() - 3);

					if (n != null && n.getEventType() == Notification.SET) {
						String oldVal = n.getOldStringValue();

						if (oldVal != null && !oldVal.equals("")) {
							EStructuralFeature changedFeature = ((EStructuralFeature) n
									.getFeature());
							EcoreUtils.setValue(eobj, changedFeature.getName(),
									oldVal);

							String featureLabel = EcoreUtils.getAnnotationVal(
									changedFeature, null, "label");
							String label = featureLabel != null ? featureLabel
									: changedFeature.getName();

							if (changedFeature.getEType() instanceof EEnum) {
								EEnum uiEnum = EcoreUtils
										.getUIEnum(changedFeature);

								if (uiEnum != null) {
									EEnumLiteral literal = (EEnumLiteral) eobj
											.eGet(changedFeature);
									oldVal = uiEnum.getEEnumLiteral(
											literal.getValue()).getName();
								}
							}

							MessageDialog.openError(null, "Validations",
									"Duplicate entry for field combination : "
											+ fieldCombination + "."
											+ "\nField '" + label + "' : "
											+ oldVal + " has been reverted.");
							return null;
						}
					}

					return "Duplicate entry for field combination : "
							+ fieldCombination
							+ "."
							+ ".\n This has to be corrected in order to be able to save changes.";
				}
			}
		}
		return null;
	}

  /**
	 * 
	 * @param eList -
	 *            Model objects
	 * @return errorMsg if feature value is blank else null
	 */
  private String checkPatternAndBlankValue(List eList)
  {
	  for (int i = 0; i < eList.size(); i++) {
		  EObject eobj = (EObject) eList.get(i);
		  checkPatternAndBlankValue(eobj);
	  }

      return null;
  }
  
  /**
  *
  * @param eobj - EObject to be checked
  * @return errorMsg if feature value is blank else null
  */
 public static String checkPatternAndBlankValue(EObject eobj)
 {
      
      List features = eobj.eClass().getEAllStructuralFeatures();
      for (int j = 0; j < features.size(); j++) {
          EStructuralFeature feature = (EStructuralFeature) features.get(j);
          Object val = eobj.eGet(feature);
          if (val instanceof String || val instanceof Integer) {
              String pattern =
                  EcoreUtils.getAnnotationVal(feature, null, "pattern");
              String msg =
                  EcoreUtils.getAnnotationVal(feature, null, "message");
              if (pattern == null) {
                  if ((feature.getEType().getClassifierID() == EcorePackage.EINT
                 || feature.getEType().getClassifierID() == EcorePackage.ELONG
                || feature.getEType().getClassifierID() == EcorePackage.ESHORT)
                        && !Pattern.compile("^-?[0-9][0-9]*$").
                          matcher(val.toString()).matches()) {
                      msg = "The value is not a number";
                      return msg;
                  }
              } else {
                  if (!Pattern.compile(pattern).
                      matcher(val.toString()).matches()) {
                      return msg;
                  }
              }
          }
          String isBlankAllowed =  EcoreUtils.getAnnotationVal(
                  feature, null, "isBlankAllowed");
          if (isBlankAllowed != null) {
              boolean isBlankValAllowed = Boolean.
                parseBoolean(isBlankAllowed);
              if (!isBlankValAllowed
                  && (val.equals("") || val.equals(" "))) {
                  String label = EcoreUtils.getAnnotationVal(
                          feature, null, "label");
                  if (label != null) {
                      return label + " cannot be blank";
                  } else {
                      return feature.getName() + " cannot be blank";
                  }
              }
          }

      }
      

     return null;
 }
}
