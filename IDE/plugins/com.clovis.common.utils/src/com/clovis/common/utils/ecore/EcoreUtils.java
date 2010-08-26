/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/EcoreUtils.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ecore;

import java.math.BigInteger;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Adapter;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.Notifier;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.ENamedElement;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.emf.ecore.impl.EcoreFactoryImpl;
import org.eclipse.emf.ecore.xml.type.impl.XMLTypePackageImpl;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.log.Log;
/**
 * This class provide many utilities to manipulate
 * Ecore Objects.
 * @author nadeem
 */
public final class EcoreUtils
{
    public static final String CW_ANNOTATION_NAME      = "CWAnnotation";
    public static final EcoreFactoryImpl ECORE_FACTORY = new EcoreFactoryImpl();
    private static final String VALIDATION_CLASS       = "ValidationClass";
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    private static final String NON_UNIQUE             = "nonUnique";
    /**
     * Set Initial value for a attribute.
     * @param eObj EObject
     * @param attr EAttribute
     */
    public static void setInitialValue(EObject eObj, EAttribute attr)
    {
        if (attr.getDefaultValue() == null) {
            if (attr.getUpperBound() == 1) {
                switch (attr.getEAttributeType().getClassifierID()) {
                    case EcorePackage.ESTRING:
                    case XMLTypePackageImpl.STRING:
                        eObj.eSet(attr, "");
                        break;
                }
            }
        } else {
            if (attr.getUpperBound() == 1) {
            	
            	if ((attr.getEType()) instanceof EEnum ){
            		
            		eObj.eSet(attr, attr.getDefaultValue());
            	}
            	else{           	
	                switch (attr.getEAttributeType().getClassifierID()) {
	                    case EcorePackage.ESTRING:
	                    case XMLTypePackageImpl.STRING:
	                    case EcorePackage.EINT:
	                    case EcorePackage.EBIG_INTEGER:
	                    case EcorePackage.EBOOLEAN:
	                        eObj.eSet(attr, attr.getDefaultValue());
	                        break;
	                }
            	}
            }
            if (attr.getUpperBound() == -1) {
            	createListFromDefaultValue(eObj, attr);
            }
        }
    }
    /**
     * Creates the List form the Default Value
     * @param eObj
     * @param attr
     */
    private static void createListFromDefaultValue(EObject eObj, EAttribute attr) {
    	StringTokenizer st = new StringTokenizer(
    			attr.getDefaultValue().toString(), ";");
		EList list = (EList)getValue(eObj, attr.getName());

    	while(st.hasMoreTokens()) {
    		switch(attr.getEAttributeType().getClassifierID()) {
    			case EcorePackage.ESTRING:
    	    		list.add(st.nextToken());
    	    		break;
    		}
    	}
	}
    /**
     * Converts Notification to String for clean logging. This
     * has less information than toString() of NotificationImpl.
     * @param msg Notification Message
     * @return String representation of Notification.
     */
    public static String getNotificationString(Notification msg)
    {
        String formatStr = "{0} [{1}, Notifier: {2}, "
            + "Feature: {3}, OldValue: {4}, NewValue: {5}]";
        Object oldVal = msg.getOldValue();
        Object newVal = msg.getNewValue();
        EStructuralFeature feature =
            (EStructuralFeature) msg.getFeature();
        Object notifier = msg.getNotifier();
        String notifierStr = (notifier instanceof EObject)
            ? ((EObject) notifier).eClass().getName()
            : notifier.getClass().getName();
        Object[] args = {
            msg.getClass().getSimpleName(),
            new Integer(msg.getEventType()),
            notifierStr,
            (feature != null ? feature.getName() : "null"),
            (oldVal  != null ? oldVal.toString() : "null"),
            (newVal  != null ? newVal.toString() : "null"),
        };
        return MessageFormat.format(formatStr, args).toString();
    }
    /**
     * Get instance of EObject with default values.
     * @param  toInit true if to initialize attributes.
     * @param  eClass EClass for which instance is required.
     * @return New Created EObject
     */
    public static EObject createEObject(EClass eClass, boolean toInit)
    {
        EObject eObject = eClass.getEPackage().
            getEFactoryInstance().create(eClass);
        // Initialize attributes with some initial values.
        if (toInit) {
            List listOfFeatures = eClass.getEAllStructuralFeatures();
            for (int i = 0; i < listOfFeatures.size(); i++) {
                EStructuralFeature feature =
                    (EStructuralFeature) listOfFeatures.get(i);
                if (feature instanceof EReference) {
                    int lbound = feature.getLowerBound();
                    int ubound = feature.getUpperBound();
                    if (lbound == 1 && ubound == 1) {
                        EReference ref = (EReference) feature;
                        eObject.eSet(ref,
                            createEObject(ref.getEReferenceType(), true));
                    }
                } else if (feature instanceof EAttribute) {
                    setInitialValue(eObject, (EAttribute) feature);
                }
            }
        }
        if (eClass.getEStructuralFeature(ModelConstants.RDN_FEATURE_NAME) != null) {
            int code = eObject.hashCode();
            EcoreUtils.setValue(eObject, ModelConstants.RDN_FEATURE_NAME,
            		String.valueOf(code));
        }
        ClovisUtils.initializeDependency(eObject);
        return eObject;
    }
    /**
     * Get Label for given Feature.
     * @param element for which label is required
     * @return Lable for it.
     */
    public static String getLabel(ENamedElement element)
    {
        EAnnotation ann = element.getEAnnotation(CW_ANNOTATION_NAME);
        String label = null;
        if (ann != null) {
            label = (String) ann.getDetails().get("label");
        }
        return label != null ? label : element.getName();
    }
    /**
     * Get NameField form EClass definition
     * @param eClass EClass
     * @return Name of the Name field from EClass.
     */
    public static String getNameField(EClass eClass)
    {
        EAnnotation ann = eClass.getEAnnotation(CW_ANNOTATION_NAME);
        String nameField = (ann != null)
            ? (String) ann.getDetails().get("NameField") : null;
        if (nameField == null) {
            if (eClass.getEStructuralFeature("name") != null) {
                nameField = "name";
            }
        }
        return nameField;
    }
    /**
     * Get Label for given Feature. Name field is read from EAnnotation
     * and value for this field is returned.
     * @param object for which Name is required
     * @return Name for it.
     */
    public static String getName(EObject object)
    {
        String name = EcoreUtils.getNameField(object.eClass());
        EStructuralFeature nameFeature = null;
        if (name == null) {
        	nameFeature = object.eClass().getEStructuralFeature("name");
        	if (nameFeature == null) {
        		nameFeature = object.eClass().getEStructuralFeature("Name");
        	}
        } else {
        	nameFeature = object.eClass().getEStructuralFeature(name);
        }
        return (nameFeature != null) ? object.eGet(nameFeature).toString() : null;
    }
   /**
    * Gets next value for the feature. The next value is always unique
    * @param prefix Prefix String before unique number
    * @param list   List of EObjects
    * @param featureName feature name
    * @return Unique value generated
    */
    public static String getNextValue(String prefix, List list,
           String featureName)
    {
       HashSet set = new HashSet();
       String newVal = null;
       for (int i = 0; i < list.size(); i++) {
           EObject eobj = (EObject) list.get(i);
           Object val = EcoreUtils.getValue(eobj, featureName);
           if (val instanceof Integer
               || val instanceof Short
               || val instanceof Long
               || val instanceof BigInteger) {
               set.add(String.valueOf(val));
           } else {
               set.add((String) val);
           }
       }
       for (int i = 0; i <= list.size(); i++) {
           newVal = prefix + String.valueOf(i);
           if (!set.contains(newVal)) {
               break;
           }
       }
       return newVal;
    }
    /**
     * Gets next value for the feature. The next value is always unique
     * @param prefix Prefix String before unique number
     * @param list   List of EObjects
     * @param featureName feature name
     * @return Unique value generated
     */
     public static String getNextValue(String prefix, List list,
            String featureName, int startIndex)
     {
        HashSet set = new HashSet();
        String newVal = null;
        for (int i = 0; i < list.size(); i++) {
            EObject eobj = (EObject) list.get(i);
            Object val = EcoreUtils.getValue(eobj, featureName);
            if (val instanceof Integer
                || val instanceof Short
                || val instanceof Long
                || val instanceof BigInteger) {
                set.add(String.valueOf(val));
            } else {
                set.add((String) val);
            }
        }
        for (int i = 0; i <= list.size(); i++) {
            newVal = prefix + String.valueOf(i + startIndex);
            if (!set.contains(newVal)) {
                break;
            }
        }
        return newVal;
     }
    /**
     * Get Value of an key in given Annotation.
     * @param element        Named Element
     * @param annotationName Name of the Annotation, null for default
     * @param key            Key
     * @return Value for this key, return null if does not exist
     */
    public static String getAnnotationVal(ENamedElement element,
            String annotationName, String key)
    {
        if (annotationName == null) {
            annotationName = CW_ANNOTATION_NAME;
        }
        EAnnotation ann = element.getEAnnotation(annotationName);
        
        return (ann != null)
            ? (String) (ann.getDetails().get(key)) : null;
    }
    /**
     * Get Value of an key in given Annotation.
     * @param element        Named Element
     * @param annotationName Name of the Annotation, null for default
     * @param key            Key
     * @return Value for this key, return null if does not exist
     */
    public static void setAnnotation(ENamedElement element,
            String annotationName)
    {
        if (annotationName == null) {
            annotationName = CW_ANNOTATION_NAME;
        }
        EAnnotation ann = element.getEAnnotation(annotationName);
        if (ann == null) {
            ann = ECORE_FACTORY.createEAnnotation();
            ann.setSource(annotationName);
            ann.setEModelElement(element);
        }
        
    }
    /**
     * sets value in given Annotation.
     * @param element        Named Element
     * @param annotationName Name of the Annotation, null for default
     * @param key            Key
     * @param val            Value to be set
     * @return Value for this key, return null if does not exist
     */
    public static void setAnnotationVal(ENamedElement element,
            String annotationName, String key, String val)
    {
        if (annotationName == null) {
            annotationName = CW_ANNOTATION_NAME;
        }
        EAnnotation ann = element.getEAnnotation(annotationName);
        if (ann != null) {
            ann.getDetails().put(key, val);
        }
    }
    /**
    *
    * @param src - Source Eobject
    * @param dest - Destination EObject
    * @param p - Properties
    */
   public static List copyValues(EObject src, EObject dest, Properties p)
   {
       List problemList = new Vector();
       if (src != null && dest != null) {
           Iterator itr = p.keySet().iterator();
           while (itr.hasNext()) {
               String key   = (String) itr.next();
               EStructuralFeature srcFeature = src.eClass().
                   getEStructuralFeature(key);
               Object value = EcoreUtils.getValue(src, key);
               if (value != null) {
                   String destKey = p.getProperty(key);
                   EStructuralFeature destFeature = dest.eClass().
                       getEStructuralFeature(destKey);
                   if (destFeature != null) {
                       if (srcFeature.getEType().getClassifierID() != destFeature.getEType().getClassifierID()) {
                           problemList.add("Data Type of feature '" + destKey
                                   + "' does not match with data type of feature in the source object. So cannot copy value '"
                                   + value.toString() + "' to destination object");
                           continue;
                       }
                       if (value instanceof List) {
                           List destList = (List) EcoreUtils.getValue(dest, destKey);
                           destList.addAll((List) value);
                       } else {
                           EcoreUtils.setValue(dest, destKey, value.toString());
                       }
                   }
               }
           }
       }
       return problemList;
   }
   /**
   *
   * @param src - Source Eobject
   * @param dest - Destination EObject
   * @param objList - List which has source and destination
   * feature names to be copied
   */
  public static void copyValues(EObject src, EObject dest, List objList)
  {
      if (src != null && dest != null) {
          for (int i = 0; i < objList.size(); i++) {
              EObject eobj = (EObject) objList.get(i);
              String srcFeatureName = EcoreUtils.getValue(eobj, "source").
                  toString();
              Object value = EcoreUtils.getValue(src, srcFeatureName);
              if (value != null) {
                  String destFeatureName = EcoreUtils.getValue(eobj, "dest").
                      toString();
                  EcoreUtils.setValue(dest, destFeatureName, value.toString());
              }
          }
      }
  }
    /**
     * Return hide/unhide nature of this named element. Uses isHidden
     * key in CwAnnotation to find this.
     * @param element Element
     * @return true if hidden.
     */
    public static boolean isHidden(ENamedElement element)
    {
        String hideStr = getAnnotationVal(element, null,
				AnnotationConstants.IS_HIDDEN);
		return hideStr != null ? Boolean.parseBoolean(hideStr) : false;
    }
    /**
     * Return modify/non-modifiable nature of this named element. Uses isHidden
     * key in CwAnnotation to find this.
     * @param element Element
     * @return true if modifiable.
     */
    public static boolean isModifiable(ENamedElement element)
    {
        String modifyStr = getAnnotationVal(element, null, "canModify");
        return modifyStr != null ? Boolean.parseBoolean(modifyStr) : false;
    }
    /**
     * Get Value of feature from EObject.
     * @param  eObj EObject
     * @param  id   Name of Feature.
     * @return features value.
     */
    public static Object getValue(EObject eObj, String id)
    {
        EStructuralFeature feature =
            eObj.eClass().getEStructuralFeature(id);
        return feature != null ? eObj.eGet(feature) : null;
    }
    /**
     * Gets Index for a literal from EEnum
     * @param eEnum    Enum
     * @param eliteral literal
     * @return index of literal from EEnum
     */
    public static int getIndex(EEnum eEnum, EEnumLiteral eliteral)
    {
        int index = -1;
        List eliterals = eEnum.getELiterals();
        for (int i = 0; i < eliterals.size(); i++) {
            EEnumLiteral literal = (EEnumLiteral) eliterals.get(i);
            if (literal.getValue() == eliteral.getValue()) {
                index = i;
                break;
            }
        }
        return index;
    }
    /**
     * Gets the UI Enum for display purpose if any
     * @param feature - EStructuralFeature
     * @return the UI Enum if specified else null
     */
    public static EEnum getUIEnum(EStructuralFeature feature)
    {
        EPackage pack = feature.getEContainingClass().getEPackage();
        String uiType = EcoreUtils.getAnnotationVal(feature, null, "UItype");
        return (uiType != null) ?
            (EEnum) pack.getEClassifier(uiType): null;
        
    }
    /**
     * General add listener for EObject and NotifyingList
     * @param obj  Object
     * @param a     Listener
     * @param depth Depth for adding listeners, -ve for full depth
     */
    public static void addListener(Object obj, Adapter a, int depth)
    {
        if (obj instanceof EObject) {
            addListener((EObject) obj, a, depth);
        } else if (obj instanceof NotifyingList) {
            addListener((NotifyingList) obj, a, depth);
        }
    }
    /**
     * General remove listener for EObject and NotifyingList
     * @param obj  Object
     * @param a     Listener
     * @param depth Depth for removing listeners, -ve for full depth
     */
    public static void removeListener(Object obj, Adapter a, int depth)
    {
        if (obj instanceof EObject) {
            removeListener((EObject) obj, a, depth);
        } else if (obj instanceof NotifyingList) {
            removeListener((NotifyingList) obj, a, depth);
        }
    }
    /**
     * Adds listener to the EObject
     * @param eObj  Object
     * @param a     Listener
     * @param depth Depth for adding listeners, -ve for full depth
     */
    private static void addListener(EObject eObj, Adapter a, int depth)
    {
        if (depth != 0) {
            if (!eObj.eAdapters().contains(a))
                eObj.eAdapters().add(a);
            if (!(eObj instanceof EEnumLiteral)) {
                EList features  = eObj.eClass().getEAllStructuralFeatures();
                for (int i = 0; i < features.size(); i++) {
                    EStructuralFeature feature =
                        (EStructuralFeature) features.get(i);
                    Object value = eObj.eGet(feature);
                    if (value instanceof NotifyingList) {
                        addListener((NotifyingList) value, a, depth - 1);
                    } else if (value instanceof EObject) {
                        addListener((EObject) value, a, depth - 1);
                    }

                }
            }
        }
    }
    /**
     * Adds a Adapter object to the list of eAdapters of EList and
     * EObjects inside the list.
     * @param l     EList on which adapter has to be attached
     * @param a     Adapter Object
     * @param depth Depth for adding listeners, -ve for full depth
     */
    private static void addListener(NotifyingList l, Adapter a, int depth)
    {
        if (depth != 0) {
            // add adapter as listner to add/remove events on ecoreList
            if (!((Notifier) (l.getNotifier())).eAdapters().contains(a))
                ((Notifier) (l.getNotifier())).eAdapters().add(a);
            // add adapter as listner to modify events (eSet) on EObjects
            for (int i = 0; i < l.size(); i++) {
                Object element = l.get(i);
                if (element instanceof EObject) {
                    addListener((EObject) element, a, depth - 1);
                } else if (element instanceof NotifyingList) {
                    //List inside List
                    addListener((NotifyingList) element, a, depth - 1);
                }
            }
        }
    }
    /**
     * Removes listener to the EObject
     * @param eObj  Object
     * @param a     Listener
     * @param depth Depth for removing listeners, -ve for full depth
     */
    private static void removeListener(EObject eObj, Adapter a, int depth)
    {
        if (depth != 0) {
            eObj.eAdapters().remove(a);
            if (!(eObj instanceof EEnumLiteral)) {
                EList features  = eObj.eClass().getEAllStructuralFeatures();
                for (int i = 0; i < features.size(); i++) {
                    EStructuralFeature feature =
                        (EStructuralFeature) features.get(i);
                    Object value = eObj.eGet(feature);
                    if (value instanceof NotifyingList) {
                        removeListener((NotifyingList) value, a, depth - 1);
                    } else if (value instanceof EObject) {
                        removeListener((EObject) value, a, depth - 1);
                    }

                }
            }
        }
    }
    /**
     * Removes a Adapter object to the list of eAdapters of EList and
     * EObjects inside the list.
     * @param l     EList on which adapter has to be attached
     * @param a     Adapter Object
     * @param depth Depth for removing listeners, -ve for full depth
     */
    private static void removeListener(NotifyingList l, Adapter a, int depth)
    {
        if (depth != 0) {
            // Remove adapter as listner to add/remove events on ecoreList
            ((Notifier) (l.getNotifier())).eAdapters().remove(a);
            // Remove adapter as listner to modify events (eSet) on EObjects
            for (int i = 0; i < l.size(); i++) {
                Object element = l.get(i);
                if (element instanceof EObject) {
                    removeListener((EObject) element, a, depth - 1);
                } else if (element instanceof NotifyingList) {
                    //List inside List
                    removeListener((NotifyingList) element, a, depth - 1);
                }
            }
        }
    }
    /**
     * Get List of features for this EClass.
     * @param eClass       EClass
     * @param viewOnly  true for view only features
     * @return List of features.
     */
    public static List getFeatureList(EClass eClass, boolean viewOnly)
    {
        List featureList = new Vector();
        List features    = eClass.getEAllStructuralFeatures();
        for (int i = 0; i < features.size(); i++) {
            EStructuralFeature feature =
                (EStructuralFeature) features.get(i);
            if (viewOnly && !(EcoreUtils.isHidden(feature))) {
                featureList.add(feature);
            }
        }
        return featureList;
    }
    /**
     * Sets value of EAttribute from a String. This method is
     * for EAttribute only.
     * @param  eObj EObject
     * @param  id   Name of Feature.
     * @param  val  String value of Attribute
     */
    public static void setValue(EObject eObj, String id, String val)
    {
        EAttribute attr = (EAttribute)
            eObj.eClass().getEStructuralFeature(id);
        if (attr != null && val != null) {
            if (attr.getEType() instanceof EEnum) {
                EEnum eEnum = (EEnum) attr.getEType();
                eObj.eSet(attr, eEnum.getEEnumLiteral(val));
            } else {
                eObj.eSet(attr, ECORE_FACTORY.
                    createFromString(attr.getEAttributeType(), val));
            }
        }
    }
    
    /**
     * Sets value of EAttribute to null if it is not required.
     * @param  eObj EObject
     * @param  id   Name of Feature.
     */
	public static void clearValue(EObject eObj, String id)
	{
		EAttribute attr = (EAttribute)eObj.eClass().getEStructuralFeature(id);
		if (attr != null && !attr.isRequired()) {
			eObj.eSet(attr, ECORE_FACTORY.createFromString(attr.getEAttributeType(), null));
		}
	}
    
    /**
     * Return EObject from the EList having the given rdn 
     * @param  list List of EObjects
     * @param  rdn  Key of the EObject that needs to be located.  
     * @return EObject corresponding to the given key
     */
    public static EObject getObjectFromKey(EList list, String rdn)
    {
    	EObject retValue = null;
    	for (int i = 0; i < list.size(); i++){
    		EObject eobj = (EObject)list.get(i);
    		String objKey = (String)getValue(eobj,
    				ModelConstants.RDN_FEATURE_NAME);
    		if (objKey != null && objKey.equals(rdn)){
    			return eobj;
    		}
    	}
    	return retValue;
    }
    /**
     * Puts 'isHidden' annotation to those features which are
     * not in the 'visibleFeatures' list
     * @param eClass - EClass
     * @param visibleFeatures - List of visible features of the EClass
     */
    public static HashMap processHiddenFields(EClass eClass, List visibleFeatures)
    {
        HashMap featureValueMap = new HashMap();
        List features = eClass.getEAllStructuralFeatures();
        for (int i = 0; i < features.size(); i++) {
            EStructuralFeature feature = (EStructuralFeature) features.get(i);
            String val = getAnnotationVal(feature, null,
                    AnnotationConstants.IS_HIDDEN);
            if (val == null) {
                featureValueMap.put(feature, "false");
                setAnnotation(feature, null);
            } else {
                featureValueMap.put(feature, val);
            }
            if (!visibleFeatures.contains(feature.getName())) {
                
                
                setAnnotationVal(feature, null,
                        AnnotationConstants.IS_HIDDEN, "true");
            } else {
                setAnnotationVal(feature, null,
                        AnnotationConstants.IS_HIDDEN, "false");
            }
        }
        return featureValueMap;
        
    }
    /*
     * Set the custom field with the new name
     * field format example : InstantiationCMD,asp_$Name$;EOProperties:eoname,$Name$EO
     */
    public static void initializeFields(EObject obj, List currentObjs, String initializationInfo)
    {
        StringTokenizer strTok = new StringTokenizer(initializationInfo, ";");
        while( strTok.hasMoreTokens())
        {
            // get the field and prefix string pair
            String[] fieldPrefixPair = strTok.nextToken().split(",");
            if( fieldPrefixPair == null || fieldPrefixPair.length > 3)
                // should never happen - it's our ecore!
                continue;
            
            String field = fieldPrefixPair[0];                      
            // zero down to the actual EObject and field
            EObjectAndField objectField = getEObject(obj, field);
            
            // get the actual prefix by substituting fields if any
            String prefixValue = "";
            if( !fieldPrefixPair[1].equals("None"))
            {
                String prefix = fieldPrefixPair[1];         
                prefixValue = getNewPrefix(obj, prefix);
            }

            // get the new unique name
            String newName;
            if (fieldPrefixPair.length == 3 && fieldPrefixPair[2].equals(NON_UNIQUE))
            {
				// skip the addition of a number suffix
            	newName = prefixValue;
            } else {
				if (EcoreUtils.getAnnotationVal(obj.eClass(), null,
						"uniqueChildren") != null) {
					ArrayList objList = new ArrayList();
					Object tmpObj;
					for (int i = 0; i < currentObjs.size(); i++) {
						tmpObj = getEObject((EObject) currentObjs.get(i), field)
								.getEObject();
						if (tmpObj != null) {
							objList.add(tmpObj);
						}
					}
					EStructuralFeature feature = objectField.getEObject().eClass()
							.getEStructuralFeature(objectField.getField());
					String ann = EcoreUtils.getAnnotationVal(feature, null,
							"initializationStartIndex");
					if (ann == null) {
						newName = EcoreUtils.getNextValue(prefixValue, objList,
								objectField.getField());
					} else {
						newName = EcoreUtils.getNextValue(prefixValue, objList,
								objectField.getField(), Integer.parseInt(ann));
					}
				} else {
					EStructuralFeature feature = obj.eClass()
							.getEStructuralFeature(field);
					String ann = EcoreUtils.getAnnotationVal(feature, null,
							"initializationStartIndex");
					if (ann == null) {
						newName = EcoreUtils.getNextValue(prefixValue, currentObjs,
								field);
					} else {
						newName = EcoreUtils.getNextValue(prefixValue, currentObjs,
								field, Integer.parseInt(ann));
					}
				}
            }
            EcoreUtils.setValue(objectField.getEObject(), objectField.getField(), newName);
        }
        
    }

    /*
     * Returns the prefix with fields substituted for the actual values
     */
    private static String getNewPrefix(EObject obj, String prefix)
    {
        int dollarIndex = -1;
        String prefixValue = "";
        // substitute the field values from parent object in prefix string
        if( (dollarIndex = prefix.indexOf("$")) != -1 )
        {               
            StringTokenizer strTokDollar = new StringTokenizer(prefix, "$");
            boolean addNextToken = false;
            while ( strTokDollar.hasMoreTokens() )
            {                   
                if( dollarIndex != 0 || addNextToken )
                    prefixValue += strTokDollar.nextToken();
                    
                if( strTokDollar.hasMoreTokens())
                {
                    String subsValue, substituteField = strTokDollar.nextToken();                  
                    
                    if (substituteField.contains("%")) {
						StringTokenizer subFieldTok = new StringTokenizer(
								substituteField, "%");
						int level = Integer.parseInt(subFieldTok.nextToken());
						substituteField = subFieldTok.nextToken();

						EObject conObj = obj;
						for (int i = 0; i < level; i++) {
							conObj = conObj.eContainer();
						}
						subsValue = (String) EcoreUtils.getValue(conObj,
								substituteField).toString();

					} else {
						subsValue = (String) EcoreUtils.getValue(obj,
								substituteField).toString();
					}
                    prefixValue += subsValue;
                    addNextToken = true;
                }
            }
        }
        else
            prefixValue = prefix;       
        
        return prefixValue;
    }
    
    /*
     * Takes care of sub-objects and fields - zeroes down to 
     * the actual EObject and field to be set
     */
    private static EObjectAndField getEObject(EObject obj, String field)
    {
        int colonIndex = -1;
        if( (colonIndex = field.indexOf(":")) == -1 )
            return new EObjectAndField(obj, field);
        
        String memberField = field.substring(0, colonIndex);
        EObject fieldEObj = (EObject)EcoreUtils.getValue(obj, memberField);
        String newField = field.substring(colonIndex+1);
        return getEObject(fieldEObj, newField);
        
    }
    
    // dummy Container class to contain target EObject and field pair
    public static class EObjectAndField{
        
        EObject _obj;
        String _field;
        
        public EObjectAndField(EObject obj, String field)
        {
            _obj = obj;
            _field = field;
    
        }
        
        public EObject getEObject()
        {
            return _obj;
        }
        
        public String getField()
        {
            return _field;
        }
    }
    /**
     * To the supplied list of eObjects, adds the validation listener, if any.
     * It's worthwile to note that the Adapter to be added may be residing
     * (and most likely would be) residing in another plugin. For this the
     * Ecore file's EAnnotation entry has reference to the plugin from where
     * the validator/adapter is coming.
     *
     * The EAnnotation's detail HashMap has a key called "ValidationClass"
     * and the value for it is "pludin id:class to be loaded". If the value
     * doesnot have the plugin id, then we search in the local plugin.
     *
     * @param eList EList
     */
    public static void addValidationListener(EList eList)
    {
        if (eList == null || eList.size() == 0) {
            return;
        }
        HashMap classMap = new HashMap();
        for (int i = 0; i < eList.size(); i++) {
            EObject eObject = (EObject) eList.get(i);
            EAnnotation eAn =
                eObject.eClass().getEAnnotation(CW_ANNOTATION_NAME);
            String valClass = (eAn != null)
                ? (String) eAn.getDetails().get(VALIDATION_CLASS) : null;
            if (valClass != null) {
                Class adapterClass = (Class) classMap.get(valClass);
                try {
                    if (adapterClass == null) {
                        int index = valClass.indexOf(':');
                        String plugin    = null;
                        String validator = valClass;
                        if (index != -1) {
                            plugin    = valClass.substring(0, index);
                            validator = valClass.substring(index);
                        }
                        adapterClass = (plugin != null)
                            ?  Platform.getBundle(plugin).loadClass(validator)
                             : Class.forName(validator);
                        classMap.put(valClass, adapterClass);
                    }
                    eObject.eAdapters().add((Adapter)adapterClass.newInstance());
                } catch (Exception e) {
                    LOG.error("Could not load Validator Class:" + valClass, e);
                }
            }
        }
    }

	/**
	 * Initializes the Name Field for the EObject.
	 * 
	 * @param eObj
	 *            EObject
	 * @param containerList
	 *            List which contains this object
	 */
	public static void initializeNameField(EObject eObj, List containerList) {
		String nameField = EcoreUtils.getNameField(eObj.eClass());
		if (nameField != null) {
			String name = EcoreUtils.getNextValue(eObj.eClass().getName(),
						containerList, nameField);
			EcoreUtils.setValue(eObj, nameField, name);
		}
	}

    /**
	 * Copies the values from srcObj to destObj.
	 * 
	 * @param srcObj
	 *            the Source EObject
	 * @param destObj
	 *            the Destination EObject
	 */
    public static void copyEObject(EObject srcObj, EObject destObj) {
    	Object attrVal = null;
		for (EAttribute attr : srcObj.eClass().getEAllAttributes()) {

			if (attr.getUpperBound() != 1)
				continue;

			if ((attrVal = EcoreUtils.getValue(srcObj, attr.getName())) != null) {
				EcoreUtils
						.setValue(destObj, attr.getName(), attrVal.toString());
			}
		}

    	List<EReference> srcRefList = srcObj.eClass().getEAllReferences();
    	List<EReference> destRefList = destObj.eClass().getEAllReferences();

    	for(int i=0 ; i<srcRefList.size() ; i++) {
    		EReference srcRef = srcRefList.get(i);
    		EReference destRef = destRefList.get(i);

    		Object srcRefObj = srcObj.eGet(srcRef);
    		Object destRefObj = destObj.eGet(destRef);

    		if(srcRefObj == null || destRefObj == null)		continue;

    		if(srcRefObj instanceof List) {
    			List srcList = (List) srcRefObj;
    			List destList = (List) destRefObj;

    			for(int j=0 ; j<srcList.size() ; j++) {
    				copyEObject((EObject) srcList.get(j), (EObject) destList.get(j));
    			}
    			
    		} else {
    			copyEObject((EObject) srcRefObj, (EObject) destRefObj);
    		}
    	}
    }

    /**
	 * Copies the values from srcObj to destObj except the values for the
	 * features in featuresToSkip list.
	 * 
	 * @param srcObj
	 *            the Source EObject
	 * @param destObj
	 *            the Destination EObject
	 * @param featuresToSkip
	 *            features whose values are not supposed to be copied.
	 */
	public static void copyEObject(EObject srcObj, EObject destObj,
			List<String> featuresToSkip) {
		Object attrVal = null;
		for (EAttribute attr : srcObj.eClass().getEAllAttributes()) {

			if (featuresToSkip.contains(attr.getName())) {
				continue;
			}

			if (attr.getUpperBound() != 1)
				continue;

			if ((attrVal = EcoreUtils.getValue(srcObj, attr.getName())) != null) {
				EcoreUtils
						.setValue(destObj, attr.getName(), attrVal.toString());
			}
		}

		List<EReference> srcRefList = srcObj.eClass().getEAllReferences();
		List<EReference> destRefList = destObj.eClass().getEAllReferences();

		for (int i = 0; i < srcRefList.size(); i++) {
			EReference srcRef = srcRefList.get(i);
			EReference destRef = destRefList.get(i);

			if (featuresToSkip.contains(srcRef.getName())) {
				continue;
			}

			Object srcRefObj = srcObj.eGet(srcRef);
			Object destRefObj = destObj.eGet(destRef);

			if (srcRefObj == null || destRefObj == null)
				continue;

			if (srcRefObj instanceof List) {
				List srcList = (List) srcRefObj;
				List destList = (List) destRefObj;

				for (int j = 0; j < srcList.size(); j++) {
					copyEObject((EObject) srcList.get(j), (EObject) destList
							.get(j));
				}

			} else {
				copyEObject((EObject) srcRefObj, (EObject) destRefObj);
			}
		}
	}
}
