/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/ComponentValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;


import java.util.HashMap;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.EditorModelValidator;


/**
 * @author pushparaj
 *
 * Validator for Component Editor
 */
public class ComponentValidator extends EditorModelValidator
{
    private List _connectionList = new ClovisNotifyingListImpl();
    /**
     * Constructor
     *
     * @param model -
     *           model of  List of EObjects
     * @param shell - Shell
     */
    public ComponentValidator(Model model, Shell shell)
    {
        super(model, shell);
        initConnectionList();
    }
    /**
    *
    * initializes the existing connections
    */
   public void initConnectionList() {
		EObject rootObject = (EObject) _elist.get(0);
		String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				_connectionList.add(eobj);
			}
		}
	}
   
    /**
	 * 
	 * @return if true if Editor contents are valid
	 */
   	public boolean isEditorValid() {
   		EObject rootObject = (EObject) _elist.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			if (ref.getName().equals(ComponentEditorConstants.CLUSTER_REF_NAME)) {
				EList list = (EList) rootObject.eGet(ref);
				if (list.size() == 0) {
					setValid(false);
					setMessage("Model should contain at least one Cluster");
					return false;
				} else if (list.size() > 1) {
					setValid(false);
					setMessage("Model should not contain more"
							+ " than one Cluster");
					return false;
				}
			} else if (ref.getName().equals(ComponentEditorConstants.NODE_REF_NAME)) {
				int sysControllerCount = 0;
				String classType;
				EList list = (EList) rootObject.eGet(ref);
				for(int j=0 ; j<list.size() ; j++) {
					classType = EcoreUtils.getValue((EObject) list.get(j), ComponentEditorConstants.NODE_CLASS_TYPE).toString();
					if(classType.equals(ComponentEditorConstants.NODE_CLASS_A) || classType.equals(ComponentEditorConstants.NODE_CLASS_B)) {
						sysControllerCount++;
						if(sysControllerCount > 1) {
							setValid(false);
							setMessage("Model should not contain more"
									+ " than one System Controller node type.");
							return false;
						}
					}
				}
			} else {
				EList list = (EList) rootObject.eGet(ref);
				for (int j = 0; j < list.size(); j++) {
					EObject eobj = (EObject) list.get(j);
					if (!isModelValid(eobj)) {
						return false;
					}
				}
			}
		}
		refList = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				if (!isConnectionValid(eobj)) {
					return false;
				}
			}
		}
		setValid(true);
		setMessage("");
		return true;
	}
    /**
	 * 
	 * @param eobj -
	 *            Connection Object
	 * @return true if the connection is valid else false.
	 */
    public boolean isConnectionValid(EObject eobj)
    {
//      check for same type of connection existing multiple times between
        //two resources in the editor
        HashMap connectionDetailsMap = new HashMap();
        for (int i = 0; i < _connectionList.size(); i++) {
            EObject connObj = (EObject) _connectionList.get(i);
            String startKey = (String) EcoreUtils.getValue(
                    connObj, ComponentEditorConstants.CONNECTION_START);
            String endKey = (String) EcoreUtils.getValue(
                    connObj, ComponentEditorConstants.CONNECTION_END);
            String connString = startKey.concat(":").concat(endKey).
            concat(":").concat(connObj.eClass().getName());
            if (!connectionDetailsMap.containsValue(connString)) {
                connectionDetailsMap.put(connObj, connString);
            } else {
                String source = (String) EcoreUtils.getValue(connObj,
                		ComponentEditorConstants.CONNECTION_START);
                String target = (String) EcoreUtils.getValue(connObj,
                		ComponentEditorConstants.CONNECTION_END);
                EObject sourceObj = (EObject) _cwkeyObjectMap.get(source);
                EObject targetObj = (EObject) _cwkeyObjectMap.get(target);
                setValid(false);
                setMessage("Connection from "
                        + EcoreUtils.getName(sourceObj) + " to " + EcoreUtils.
                        getName(targetObj) + " already exists!");
                return false;
            }
        }
        String source = (String) EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_START);
        String target = (String) EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_END);
        if (source.equals(target)) {
            setValid(false);
            setMessage("Source and Target cannot be same");
            return false;
        } else {
            EObject sourceObj = (EObject) _cwkeyObjectMap.get(source);
            EObject targetObj = (EObject) _cwkeyObjectMap.get(target);
            if (sourceObj != null && targetObj != null) {
				String message = checkConnectionValidity(sourceObj, targetObj,
						eobj);
				if (message != null) {
					setValid(false);
					setMessage(message);
					return false;
				}
			}
        }
        setValid(true);
        setMessage("");
        return true;
    }
    /**
     * Checks the connections before creating connections in UI
     * @param sourceObj Source EObject
     * @param targetObj Target EObject
     * @param eobj Connection EObject
     * @return error message or null
     */
    public String isConnectionValid(EObject sourceObj,
            EObject targetObj, EObject eobj)
    {	
    	String source = (String) EcoreUtils.getValue(
                sourceObj, ModelConstants.RDN_FEATURE_NAME);
        String target = (String) EcoreUtils.getValue(
                targetObj, ModelConstants.RDN_FEATURE_NAME);
        String key = (String) EcoreUtils.getValue(
                eobj, ModelConstants.RDN_FEATURE_NAME);
        
    	for (int i = 0; i < _connectionList.size(); i++) {
            EObject connObj = (EObject) _connectionList.get(i);
            if (!key.equals((String) EcoreUtils.getValue(connObj,
            		ModelConstants.RDN_FEATURE_NAME))
					&& eobj.eClass().getName().equals(
							connObj.eClass().getName())) {
				String source1 = (String) EcoreUtils.getValue(connObj, ComponentEditorConstants.CONNECTION_START);
				String target1 = (String) EcoreUtils.getValue(connObj, ComponentEditorConstants.CONNECTION_END);
				if (source.equals(source1) && target.equals(target1)) {
					return "Connection from "
							+ EcoreUtils.getName(sourceObj) + " to "
							+ EcoreUtils.getName(targetObj)
							+ " already exists!";
				}
			}
        }
    	if (sourceObj == targetObj || sourceObj.equals(targetObj)) {
            return "Source and Target cannot be same";
        } 
    	
        String message = checkConnectionValidity(
                sourceObj, targetObj, eobj);
        return message;
        
    }
    /**
     *
     * @param sourceObj Source Object
     * @param targetObj Target Object
     * @param connObj Connection Object
     * @return the Message on error else null
     */
    private String checkConnectionValidity(EObject sourceObj,
            EObject targetObj, EObject connObj)
    {
        String msg = null;
        List inConnList = new Vector(), outConnList = new Vector();
        List clonedConnectionList = EcoreCloneUtils.cloneList((NotifyingList) _connectionList);
        if( !_connectionList.contains(connObj) )
        {        	        
        	clonedConnectionList.add(connObj);
        }
        
        boolean inConnValid = false, outConnValid = false;
        List sourceSuperClasses = sourceObj.eClass().getEAllSuperTypes();
        List targetSuperClasses = targetObj.eClass().getEAllSuperTypes();
        String outgoingConn = EcoreUtils.getAnnotationVal(
                sourceObj.eClass(), null, "outgoingconnection");
        if (outgoingConn == null) {
            for (int i = 0; i < sourceSuperClasses.size(); i++) {
                EClass superClass = (EClass) sourceSuperClasses.get(i);
                outgoingConn = EcoreUtils.getAnnotationVal(superClass,
                        null, "outgoingconnection");
                if (outgoingConn != null) {
                    break;
                }
        } }
        if (outgoingConn != null) {
            int index = outgoingConn.indexOf(',');
            while (index != -1) {
                String feature = outgoingConn.substring(0, index);
                outConnList.add(feature);
                outgoingConn = outgoingConn.substring(index + 1);
                index = outgoingConn.indexOf(',');
            } outConnList.add(outgoingConn);
        }
        
        for (int i = 0; i < outConnList.size(); i++) {
            String conn = (String) outConnList.get(i);
            int index = conn.indexOf(':'), index1 = conn.indexOf('=');
            String targetObjType = conn.substring(0, index);
            String connType = conn.substring(index + 1, index1);
            String multiplicity = conn.substring(index1 + 1);
            if (targetObjType.equals(targetObj.eClass().getName()) && connType.
                    equals(connObj.eClass().getName())) {
                outConnValid = true;
                if (multiplicity.equals("-1")) {
                    break;
                } 
               int mul = Integer.parseInt(multiplicity);                    
               if (getAllOutConnections(connType, sourceObj, targetObj, clonedConnectionList) > mul) {
                   msg = "Connection from "
                   + sourceObj.eClass().getName() + " to " + targetObj.
                   eClass().getName() + " already exists!";
                        return msg;
                    }
                
            } else { // check if the eclass name matches super Eclasses
                for (int j = 0; j < targetSuperClasses.size(); j++) {
                    EClass superClass = (EClass) targetSuperClasses.get(j);
                    if (targetObjType.equals(superClass.getName()) && connType.
                            equals(connObj.eClass().getName())) {
                        outConnValid = true;
                        if (multiplicity.equals("-1")) {
                            break;
                        } 
                       int mul = Integer.parseInt(multiplicity);
               
              if (getAllOutConnections(connType, sourceObj, targetObj, clonedConnectionList) > mul) {
                  msg = "Connection from "
                  + sourceObj.eClass().getName() + " to " + targetObj.
                  eClass().getName() + " already exists!";
                                return msg;
                            }
                        
                    }
                }
            }
        }
        if (!outConnValid) {
            msg = "Connection is not valid from " + sourceObj.eClass().getName()
            + " to " + targetObj.eClass().getName();
            return msg;
        }
        String incomingConn = EcoreUtils.getAnnotationVal(
                targetObj.eClass(), null, "incomingconnection");
        if (incomingConn == null) {
            for (int i = 0; i < targetSuperClasses.size(); i++) {
                EClass superClass = (EClass) targetSuperClasses.get(i);
                incomingConn = EcoreUtils.getAnnotationVal(superClass,
                        null, "incomingconnection");
                if (incomingConn != null) {
                    break;
                }
            }
        }
        if (incomingConn != null) {
            int index = incomingConn.indexOf(',');
            while (index != -1) {
                String feature = incomingConn.substring(0, index);
                inConnList.add(feature);
                incomingConn = incomingConn.substring(index + 1);
                index = incomingConn.indexOf(',');
            }
            inConnList.add(incomingConn);
        }
        for (int i = 0; i < inConnList.size(); i++) {
            String conn = (String) inConnList.get(i);
            int index = conn.indexOf(':'), index1 = conn.indexOf('=');
            String sourceObjType = conn.substring(0, index);
            String connType = conn.substring(index + 1, index1);
            String multiplicity = conn.substring(index1 + 1);
            if (sourceObjType.equals(sourceObj.eClass().getName()) && connType.
                    equals(connObj.eClass().getName())) {
                inConnValid = true;
                if (multiplicity.equals("-1")) {
                    break;
                } 
                    int mul = Integer.parseInt(multiplicity);
               
               if (getAllInConnections(connType, sourceObj, targetObj, clonedConnectionList ) > mul) {
                   msg = "Connection from "
                   + sourceObj.eClass().getName() + " to " + targetObj.
                   eClass().getName() + " already exists!";
                        return msg;
                    }
                
            } else { // check if the eclass name matches any Super Eclass
                for (int j = 0; j < sourceSuperClasses.size(); j++) {
                    EClass superClass = (EClass) sourceSuperClasses.get(j);
                    if (sourceObjType.equals(superClass.getName()) && connType.
                            equals(connObj.eClass().getName())) {
                        outConnValid = true;
                        if (multiplicity.equals("-1")) {
                            break;
                        } 
                            int mul = Integer.parseInt(multiplicity);
               
                            if (getAllOutConnections(
                                connType, sourceObj, targetObj, clonedConnectionList) > mul) {
                            msg = "Connection from " + sourceObj.eClass().
                            getName() + " to " + targetObj.eClass().
                            getName() + " already exists!";
                                return msg;
                            }
                        
                    }
                }
            }
        }
        if (!inConnValid) {
            msg = "Connection is not valid from "
            + sourceObj.eClass().getName() + " to "
            + targetObj.eClass().getName();
            return msg;
        }
        return msg;
    }
    /**
     *
     * @param connType Connection Type
     * @param sourceObj Source Object
     * @param targetObj Target Object
     * @return number of outgoing connections of type connType
     */
    private int getAllOutConnections(String connType, EObject sourceObj,
            EObject targetObj, List connectionList)
    {
        int outConnCount = 0;
        String cwkey = (String) EcoreUtils.getValue(sourceObj,
        		ModelConstants.RDN_FEATURE_NAME);
           for (int i = 0; i < connectionList.size(); i++) {
               EObject connObj = (EObject) connectionList.get(i);
               String startKey = (String) EcoreUtils.getValue(
                       connObj, ComponentEditorConstants.CONNECTION_START);
               String endKey = (String) EcoreUtils.getValue(
                       connObj, ComponentEditorConstants.CONNECTION_END);
               EObject target = (EObject) _cwkeyObjectMap.get(endKey);
               if (startKey.equals(cwkey) && connObj.eClass().getName().
                equals(connType)) {
                   if (target.eClass().getName().equals(targetObj.
                           eClass().getName())) {
                   outConnCount++;
                   } else {
                      List superClasses = target.eClass().getEAllSuperTypes();
                      List targetSuperClasses = targetObj.eClass().
                          getEAllSuperTypes();
                      if (!superClasses.isEmpty()
                              && !targetSuperClasses.isEmpty()) {
                          EClass targetClass = (EClass) superClasses.get(0);
                          EClass targetObjClass = (EClass) targetSuperClasses.
                              get(0);
                          if (targetClass.getName().equals(
                                  targetObjClass.getName())) {
                              outConnCount++;
                          }
                      }
                   }
               }
           }
        return outConnCount;

    }
    /**
     *
     * @param connType Connection Type
     * @param sourceObj Source Object
     * @param targetObj Target Object
     * @return number of incoming connections of type connType
     */
    private int getAllInConnections(String connType, EObject sourceObj,
            EObject targetObj, List connectionList)
    {
        int inConnCount = 0;
        String cwkey = (String) EcoreUtils.getValue(targetObj,
        		ModelConstants.RDN_FEATURE_NAME);
           for (int i = 0; i < connectionList.size(); i++) {
               EObject connObj = (EObject) connectionList.get(i);
               String endKey = (String) EcoreUtils.getValue(
                       connObj, ComponentEditorConstants.CONNECTION_END);
               String startKey = (String) EcoreUtils.getValue(
                       connObj, ComponentEditorConstants.CONNECTION_START);
               EObject source = (EObject) _cwkeyObjectMap.get(startKey);
               if (endKey.equals(cwkey) && connObj.eClass().getName().
                       equals(connType)) {
                   if (source.eClass().getName().equals(sourceObj.
                           eClass().getName())) {
                   inConnCount++;
                   } else {
                       List superClasses = source.eClass().getEAllSuperTypes();
                          List sourceSuperClasses = sourceObj.eClass().
                              getEAllSuperTypes();
                          if (!superClasses.isEmpty()
                                  && !sourceSuperClasses.isEmpty()) {
                             EClass sourceClass = (EClass) superClasses.get(0);
                             EClass sourceObjClass = (EClass)
                                 sourceSuperClasses.get(0);
                              if (sourceClass.getName().equals(
                                      sourceObjClass.getName())) {
                                  inConnCount++;
                              }
                          }

                   }
               }
           }
        return inConnCount;

    }
    
    /**
     *@param notification - Notification
     */
    public void notifyChanged(Notification notification)
    {
        switch (notification.getEventType()) {
        case Notification.REMOVING_ADAPTER:
            break;
        case Notification.SET:
            if (!notification.isTouch()) {
            EObject eobj = (EObject) notification.getNotifier();
            if (eobj.eClass().getName().equals(
					ComponentEditorConstants.AUTO_NAME)) {
        		String connType = (String) EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_TYPE);
        		EAttribute feature = (EAttribute) notification.getFeature();
        		if (connType.equals(ComponentEditorConstants.PROXY_PROXIED_NAME)) {
        			if (feature.getName().equals(ComponentEditorConstants.CONNECTION_START)) {
        				EObject targetObj = (EObject) _cwkeyObjectMap.get(EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_END));
						if(targetObj.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
							String compType = EcoreUtils.getValue(targetObj, "property").toString();
							if(compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
								EcoreUtils.setValue(targetObj, "property", ComponentEditorConstants.PROXIED_PREINSTANTIABLE);
								EcoreUtils.setValue(targetObj, "terminateCommand", "");
				        		EcoreUtils.setValue(targetObj, "cleanupCommand", "");
							}
						}
					} else if (feature.getName().equals(ComponentEditorConstants.CONNECTION_END)) {
						EObject oldTargetObj = (EObject) _cwkeyObjectMap.get(notification.getOldValue());
						if(oldTargetObj.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
							String compType = EcoreUtils.getValue(oldTargetObj, "property").toString();
							if(!compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
								EcoreUtils.setValue(oldTargetObj, "property", ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE);
							}
						}	
						EObject targetObj = (EObject) _cwkeyObjectMap.get(EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_END));
						if(targetObj.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
							String compType = EcoreUtils.getValue(targetObj, "property").toString();
							if(compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
								EcoreUtils.setValue(targetObj, "property", ComponentEditorConstants.PROXIED_PREINSTANTIABLE);
								EcoreUtils.setValue(targetObj, "terminateCommand", "");
				        		EcoreUtils.setValue(targetObj, "cleanupCommand", "");
							}
						}
					}
				} else if (connType.equals(ComponentEditorConstants.ASSOCIATION_NAME)) {
					if (feature.getName().equals(ComponentEditorConstants.CONNECTION_START)) {
						EObject targetObj = (EObject) _cwkeyObjectMap.get(EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_END));
						if(targetObj.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
							if (!isProxied(targetObj)) {
								String compType = EcoreUtils.getValue(targetObj, "property").toString();
								if(!compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
									EcoreUtils.setValue(targetObj, "property", ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE);
								}
							}
						}
					} else if (feature.getName().equals(ComponentEditorConstants.CONNECTION_END)) {
						System.out.println("ASSOC END");	
					}
				} else if (connType.equals(ComponentEditorConstants.CONTAINMENT_NAME)) {
					if (feature.getName().equals(ComponentEditorConstants.CONNECTION_START)) {
						EObject targetObj = (EObject) _cwkeyObjectMap.get(EcoreUtils.getValue(eobj, ComponentEditorConstants.CONNECTION_END));
						if(targetObj.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
							if (!isProxied(targetObj)) {
								String compType = EcoreUtils.getValue(targetObj, "property").toString();
								if(!compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
									EcoreUtils.setValue(targetObj, "property", ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE);
								}
							}
						}
					} else if (feature.getName().equals(ComponentEditorConstants.CONNECTION_END)) {
						System.out.println("CONT END");	
					}
				}
			}
            /*if (eobj.eClass().getName().equals(
                    ClassEditorConstants.COMPOSITION_NAME)
             || eobj.eClass().getName().equals(
                     ClassEditorConstants.INHERITENCE_NAME)
                || eobj.eClass().getName().equals(
                        ClassEditorConstants.ASSOCIATION_NAME)) {
                if (!isConnectionValid(eobj)) {
                    MessageDialog.openError(
                    _shell, "Component Editor Validations", getMessage());
                    //_connectionList.remove(eobj);
                    //_elist.remove(eobj);
                }
            } else*/ {
                if (!isModelValid(eobj, notification)) {
                    MessageDialog.openError(
                    _shell, "Component Editor Validations", getMessage());
                }
            }
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
               if (obj.eClass().getName().equals(
                        ComponentEditorConstants.AUTO_NAME)) {
                    _connectionList.add(obj);
                    
                    updateProxyObject(obj);
                }
                if (!isModelValid(obj)) {
                    MessageDialog.openError(
                    _shell, "Component Editor Validations", getMessage());
                }
                
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
                    if (eObj.eClass().getName().equals(
                            ComponentEditorConstants.AUTO_NAME)) {
                        _connectionList.add(eObj);
                        
                        updateProxyObject(eObj);
                    } 
                   if (!isModelValid(eObj)) {
                       MessageDialog.openError(_shell,
                       "Component Editor Validations", getMessage());
                   }
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
                if (ob.eClass().getName().equals(
                        ComponentEditorConstants.AUTO_NAME)) {
                    _connectionList.remove(ob);
                    updateNonProxyObject(ob);
                }
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
                    if (o.eClass().getName().equals(
                            ComponentEditorConstants.AUTO_NAME)) {
                        _connectionList.remove(o);
                        updateNonProxyObject(o);
                    }
                }
            }
            break;
        }
    }
    
    private void updateProxyObject(EObject obj) {
    	String connType = (String) EcoreUtils.getValue(obj,
				ComponentEditorConstants.CONNECTION_TYPE);
        if(connType.equals(ComponentEditorConstants.PROXY_PROXIED_NAME)) {
        	EObject targetObj = (EObject) _cwkeyObjectMap.get(EcoreUtils.getValue(obj, ComponentEditorConstants.CONNECTION_END));
        	String compType = EcoreUtils.getValue(targetObj, "property").toString();
        	if(compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
        		EcoreUtils.setValue(targetObj, "property", ComponentEditorConstants.PROXIED_PREINSTANTIABLE);
        		EcoreUtils.setValue(targetObj, "terminateCommand", "");
        		EcoreUtils.setValue(targetObj, "cleanupCommand", "");
        	}
        }
    }
    private void updateNonProxyObject(EObject obj) {
    	String connType = (String) EcoreUtils.getValue(obj,
				ComponentEditorConstants.CONNECTION_TYPE);
        if(connType.equals(ComponentEditorConstants.PROXY_PROXIED_NAME)) {
        	EObject targetObj = (EObject) _cwkeyObjectMap.get(EcoreUtils.getValue(obj, ComponentEditorConstants.CONNECTION_END));
        	String compType = EcoreUtils.getValue(targetObj, "property").toString();
        	if(!compType.equals(ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE)) {
        		EcoreUtils.setValue(targetObj, "property", ComponentEditorConstants.NON_PROXIED_NON_PREINSTANTIABLE);
        	}
        }
    }
    private boolean isProxied(EObject obj) {
    	String targetKey = (String) EcoreUtils.getValue(obj,
    			ModelConstants.RDN_FEATURE_NAME);
    	for (int i = 0; i < _connectionList.size(); i++) {
    		EObject connObj = (EObject) _connectionList.get(i);
    		String connType = (String) EcoreUtils.getValue(connObj, ComponentEditorConstants.CONNECTION_TYPE);
    		if(connType.equals(ComponentEditorConstants.PROXY_PROXIED_NAME)) {
    			String key = (String) EcoreUtils.getValue(connObj, ComponentEditorConstants.CONNECTION_END);
    			if(key.equals(targetKey)) {
    				return true;
    			}
    		}
    	}
    	return false;
    }
}
