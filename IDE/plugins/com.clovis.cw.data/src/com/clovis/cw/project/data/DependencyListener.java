/*
 * @(#) $RCSfile: DependencyListener.java,v $
 * $Revision: #4 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/DependencyListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: DependencyListener.java,v $
 * $Revision: #4 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.project.data;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.URL;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;

/**
 * 
 * @author shubhada
 *
 * Generic Listener Class to listen to the change in the object
 * on which other objects are dependent.
 */
public class DependencyListener extends AdapterImpl
{
    private ProjectDataModel _pdm = null;
    private int _objectsType;
    private static final Log LOG = Log.getLog(DataPlugin.getDefault());
    public static final int MODEL_OBJECT = 0;
    public static final int VIEWMODEL_OBJECT = 1;
    public static final int MODEL_VIEWMODEL_OBJECT = 2;
    public static final int OTHER_OBJECT = 3;
    /**
     * 
     * @param pdm ProjectDataModel
     */
    public DependencyListener(int objectsType)
    {
        this(null, objectsType);
    }
    /**
     * 
     * @param pdm ProjectDataModel
     */
    public DependencyListener(ProjectDataModel pdm, int objectsType)
    {
        _pdm = pdm;
        _objectsType = objectsType;
    }
    /**
     * @param notification -
     *            Notification
     */
    public void notifyChanged(Notification notification)
    {
        if (notification.isTouch()) {
            return;
        }
        switch (notification.getEventType()) {
        case Notification.REMOVING_ADAPTER:
            break;
        case Notification.SET:
            if (notification.getNotifier() instanceof EObject) {
                EObject eobj = (EObject) notification.getNotifier();
                processDependents(notification, eobj);
            }
            break;
        case Notification.ADD:
            Object newVal = notification.getNewValue();
            if (newVal instanceof EObject) {
                EObject obj = (EObject) newVal;
                EcoreUtils.addListener(obj, this, -1);
                processDependents(notification, obj);
            }
            break;
        case Notification.REMOVE:
            Object oldVal = notification.getOldValue();
            if (oldVal instanceof EObject) {
                EObject obj = (EObject) oldVal;
                processDependents(notification, obj);
                EcoreUtils.removeListener(obj, this, -1);
            }
            break;
            
        case Notification.ADD_MANY:
            List objs = (List) notification.getNewValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject eObj = (EObject) objs.get(i);
                    EcoreUtils.addListener(eObj, this, -1);
                    processDependents(notification, eObj);
                }
            }
            break;
        case Notification.REMOVE_MANY:
            List objects = (List) notification.getOldValue();
            if (objects != null) {
                for (int i = 0; i < objects.size(); i++) {
                    if (objects.get(i) instanceof EObject) {
                        EObject eObj = (EObject) objects.get(i);
                        processDependents(notification, eObj);
                        EcoreUtils.removeListener(eObj, this, -1);
                    }
                }
             }
            break;
        }
    }
    /**
     * checks for "dependents" annotation specified in the Changed Object
     * to notify dependent objects that I am changed
     * @param notification - Notification
     * @param eobj - Changed EObject
     */
    private void processDependents(Notification notification, EObject eobj)
    {
        String dependentsFeatureNames = EcoreUtils.getAnnotationVal(eobj.eClass(),
                null, "dependentsFeatureNames");
        if (dependentsFeatureNames != null) {
            String [] dependentFeatures = dependentsFeatureNames.split(",");
            for (int i = 0; i < dependentFeatures.length; i++) {
                String dependentFeature = dependentFeatures[i];
                // using the dependentFeature find the Structural Feature
                // which has the annotation to findout the dependents
                //who are interested changes to this feature
                EStructuralFeature feature = eobj.eClass().
                    getEStructuralFeature(dependentFeature);
                if (notification.getEventType() == Notification.SET
                        && notification.getFeature() instanceof EAttribute) {
                     String changedFeatureName = ((EAttribute) notification.
                             getFeature()).getName();
                     String dependents = EcoreUtils.getAnnotationVal(feature,
                             null, "dependents");
                     if (feature.getName().equals(changedFeatureName)
                             && dependents != null) {
                         processDependentsString(notification, eobj, dependents);
                     }
                } else {
                    
                    String dependents = EcoreUtils.getAnnotationVal(feature,
                            null, "dependents");
                    if (dependents != null) {
                        processDependentsString(notification, eobj, dependents);
                    }
                }
            } 
         } 
        // search the eclass for the dependents annotation directly
        String dependents = EcoreUtils.getAnnotationVal(eobj.eClass(),
                null, "dependents");
        if (dependents != null) {
            processDependentsString(notification, eobj, dependents);
        }
        
        
    }
    /**
     * Processes the dependents annotation
     * @param notification - Notification
     * @param eobj - Changed Object
     * @param dependents - Dependents Annotation String
     */
    private void processDependentsString(Notification notification,
            EObject eobj, String dependents)
    {
        try {
            List dependentsInfoList = new Vector();
            StringTokenizer tokenizer = new StringTokenizer(dependents, ";");
            while (tokenizer.hasMoreTokens()) {
                String dependent = tokenizer.nextToken();
                dependentsInfoList.add(dependent);
            }
            for (int i = 0; i < dependentsInfoList.size(); i++) {
                String dependentInfo = (String) dependentsInfoList.get(i);
                tokenizer = new StringTokenizer(dependentInfo, "@");
                String handlerTypeVal = tokenizer.nextToken();
                dependentInfo = tokenizer.nextToken();
                int handlerType = Integer.parseInt(handlerTypeVal);
                
                switch (_objectsType) {
                case MODEL_OBJECT:
                	switch (handlerType) {
                		case VIEWMODEL_OBJECT: continue;
                		case OTHER_OBJECT: continue;						
                	}
                	break;
                case VIEWMODEL_OBJECT:
                	switch (handlerType) {
            		case MODEL_OBJECT: continue;
            		case OTHER_OBJECT: continue;						
                	}
                	break;
                case OTHER_OBJECT:
                	switch (handlerType) {
            		case MODEL_OBJECT: continue;
            		case VIEWMODEL_OBJECT: continue;	
            		case MODEL_VIEWMODEL_OBJECT: continue;
                	}
                	break;
                }
                tokenizer = new StringTokenizer(dependentInfo, "!");
                String relativeEcoreName = tokenizer.nextToken();
                dependentInfo = tokenizer.nextToken();
                
                tokenizer = new StringTokenizer(dependentInfo, "%");
                String dependentsPath = tokenizer.nextToken();
                dependentInfo = tokenizer.nextToken();
                
                tokenizer = new StringTokenizer(dependentInfo, "|");
                String [] featuresNames = tokenizer.nextToken().split(",");
                dependentInfo = tokenizer.nextToken();
                
                tokenizer = new StringTokenizer(dependentInfo, "#");
                String resourcePath = tokenizer.nextToken();
                String handlerName = tokenizer.nextToken();
                
                
                readEcoreFile(relativeEcoreName);
                
                Resource resource = readResource(resourcePath);
                
                String [] refPaths = dependentsPath.split("$");
                if (resource != null) {
                   Object dependent = 
                       getDependentObj(resource, dependentsPath);               
                	   callHandler(notification, eobj, dependent,
                               featuresNames, refPaths, handlerName);
                   try {
                    EcoreModels.save(resource);
                    } catch (IOException e) {
                        LOG.warn("Could not update the dependent resource", e);
                    }
                } else if (!resourcePath.equals("None") && resource == null) {
                	if (handlerType == MODEL_VIEWMODEL_OBJECT
							&& _objectsType == VIEWMODEL_OBJECT) {
						callHandler(notification, eobj, null, featuresNames,
								refPaths, handlerName);
					} else {
						continue;
					}
                } else {
                    resource = getResource(eobj);
                    String refPath = refPaths[0];
                    String [] references = refPath.split(",");
                    if (refPaths.length == 1 && references[0].equals("None")) {
                        callHandler(notification, eobj, eobj, featuresNames,
                                refPaths, handlerName);
                        continue;
                    } 
                    if (resource == null) {
                        callHandler(notification, eobj, null,
                                featuresNames, refPaths, handlerName);
                    } else {
                        Object dependent = getDependentObj(resource, dependentsPath);
                        callHandler(notification, eobj, dependent,
                                featuresNames, refPaths, handlerName);
                    }
                }
           }
       } catch (Exception e)
       {
            LOG.warn("Could not update the dependent object", e);
       }
    }
    /**
     * 
     * @param eobj - EObject
     * @return Resource
     */
    private Resource getResource(EObject eobj)
    {
        Resource resource = eobj.eResource();
        if (resource != null) {
            return resource;
        } 
        EObject containerObj = eobj.eContainer();
        if (resource == null && containerObj == null) {
            return null;
        }
        if (resource == null && containerObj != null) {
            resource = containerObj.eResource();
            if (resource != null) {
                return resource;
            } else {
                return getResource(containerObj);
            }
        }
        return resource;
    }
    /**
     * 
     * @param resource - Dependent Resource
     * @param dependentsPath - specification of all feature names which will 
     * help to reach dependent object
     * @param dependentsPath- Navigation path List from the top level resource object
     * Same feature having multiple path is separated by '&'
     * @return the Dependent Object/List
     */
   private Object getDependentObj(Resource resource,
           String dependentsPath)
    {
       List dependentObjects = new Vector();
       try {
           List resourceContents = resource.getContents();
           String [] dependentRefs =  dependentsPath.split("\\$");
           for (int i = 0; i < dependentRefs.length; i++) {
               String [] references = dependentRefs[i].split(",");
                int index = 0;
                if (references[index].equals("None")) {
                    return resourceContents;
                }
                for (int j = 0; j < resourceContents.size(); j++) {
                    EObject eobj = (EObject) resourceContents.get(j);
                    getObject(dependentObjects, references, eobj, 0);
                }
               
           }
        } catch (Exception e)
          {
               LOG.warn("Failed to get dependent object", e);
         }
        
        return dependentObjects;
    }
   /**
    * recursively traverses thru all the list objects and object to
    * get the dependent object/list.  
    * @param dependentObjects - dependent Obj List
    * @param referenceList - Reference Features for traversal
    * @param eobj - current  position EObject
    * @param index - index of referenceList
    */
    public static void getObject(List dependentObjects, String[] references,
            EObject eobj, int index)
    {
        try {
            if (index < references.length) {
                Object obj = EcoreUtils.getValue(eobj, references[index]);
                index ++;
                if (index == references.length) {
                    if (obj instanceof List) {
                        List objList = (List) obj;
                        for (int i = 0; i < objList.size(); i++) {
                            Object dependentobj = objList.get(i);
                            if (dependentobj != null) {
                                dependentObjects.add(dependentobj);
                            }
                        }
                        
                    } else {
                        if (obj != null) {
                        dependentObjects.add(obj);
                        }
                    }
                    return;
                } 
                if (obj instanceof EObject) {
                    EObject nextObj = (EObject) obj;
                    getObject(dependentObjects, references, nextObj, index);
                    
                } else if (obj instanceof List) {
                    List list = (List) obj;
                    for (int i = 0; i < list.size(); i++) {
                        EObject nextObj = (EObject) list.get(i);
                        getObject(dependentObjects,
                                references, nextObj, index);
                    }
                }
            }
            
        } catch (Exception e) {
            LOG.warn("Could not trace the dependent object", e);
        }
    }
    /**
     * Loads the handler class and calls the handler method with arguments
     * @param n - Notification
     * @param changedObj - Object which triggered notification
     * @param dependent - Dependent Object(s)
     * @param featureNames - dependent feature names
     * @param handlerName - Handler plugin qualified name
     */
    private void callHandler(Notification n, EObject changedObj,
            Object dependent, String [] featureNames,
            String [] referencePaths, String handlerName)
    {
        Class handlerClass = ClovisUtils.loadClass(handlerName);
        try {
            Class[] argType = {
                    Notification.class,
                    Object.class,
                    Object.class,
                    String [].class,
                    String [].class,
                    ProjectDataModel.class
            };
            if (handlerClass != null) {
                Method met  = handlerClass.getMethod("processNotifications", argType);
                Object[] args = {n, changedObj, dependent, featureNames, referencePaths, _pdm};
                met.invoke(null, args);
            }
        } catch (NoSuchMethodException e) {
            LOG.error("Handler does not have processNotifications() method", e);
        } catch (Throwable th) {
            LOG.error("Unhandled error while calling handler:" + handlerName, th);
        }
    
    }
    /**
     * 
     * @param resourcePath - Resource Path relative to project dir in workspace
     * @return the Resource or null if the resourcePath is specified as "None"
     */
    private Resource readResource(String resourcePath)
    {
        try {
            if (!resourcePath.equals("None") && _pdm != null) {
                resourcePath = resourcePath.replace('/', File.separatorChar);
    
                //Now get the xmi file from the project which may or
                //may not have data
                String dataFilePath = _pdm.getProject().getLocation().toOSString()
                                      + File.separator + resourcePath;
                URI uri = URI.createFileURI(dataFilePath);
                File xmiFile = new File(dataFilePath);
                if (xmiFile.exists()) {
                    Resource resource = null;
                    if (EcoreModels.isResourceLoaded(uri)) {
                        resource = EcoreModels.getResource(uri);
                        
                    } else {
                        resource = EcoreModels.getUpdatedResource(uri);
                    }
                    return resource;
                } 
            }
           
        } catch (Exception exception) {
            LOG.warn("Failed while loading resource", exception);
        }
        return null;
    }
    /**
     * reads the ecore file
     * @param relativeEcoreName - Ecore Path Name
     */
    private void readEcoreFile(String relativeEcoreName)
    {
        try {
            if (!relativeEcoreName.equals("None")) {
                relativeEcoreName = relativeEcoreName.replace('/', File.separatorChar);
                URL url = DataPlugin.getDefault().find(new Path(relativeEcoreName));
                File ecoreFile = new Path(Platform.resolve(url).getPath())
                        .toFile();
                EcoreModels.get(ecoreFile.getAbsolutePath());
            }
        } catch (IOException ex) {
            LOG.warn("Ecore file cannot be read", ex);
        }
        
    }

}
