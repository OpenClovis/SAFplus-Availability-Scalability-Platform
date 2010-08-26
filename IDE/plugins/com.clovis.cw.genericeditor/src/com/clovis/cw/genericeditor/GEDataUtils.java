/*
 * @(#) $RCSfile: GEDataUtils.java,v $
 * $Revision: #5 $ $Date: 2007/01/03 $
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
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEDataUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

/**
 * 
 * @author shubhada
 *
 * GEDataUtils which has some utils methods.
 * Specific editors to override methods to provide
 * implementation.
 */
public class GEDataUtils
{
    /**
     * List of All EObjects.
     */
    protected List _eObjects;
    /**
     * constructs GEUtils.
     * @param objs EObjects list
     */
    public GEDataUtils(List objs)
    {
        _eObjects = objs;
    }
    /**
     * returns parent object.
     * @param key EObject
     * @return obj parent EObject
     */
    public EObject getParent(EObject key)
    {
        return null;
    }
    /**
     * returns target object for edges.
     * @param key EObject
     * @return obj Target EObject
     */
    public EObject getTarget(EObject key)
    {
        return null;
    }
    /**
     * returns source object for edges.
     * @param key EObject
     * @return Source EObject
     */
    public EObject getSource(EObject key)
    {
        return null;
    }
    /**
     * 
     * Returns the matched connection instances. 
     * 
     * @param connType Connection Type
     * @param sourceType Source Object Class Name
     * @param targetType Target Object Class Name
     * @return the list of filtered connections
     */
    public List getConnectionFrmType(String connType, String sourceType,
            String targetType)
    {
        return null;
    }
    /**
     * 
     * @param name - name to be checked
     * @return EObject if found else return null
     */
    public EObject getObjectFrmName(String name)
    {
     return null;   
    }
    /**
     * 
     * @param editorList - Editor List
     * @param nodeType - type of the node
     * @return List od Nodes of type nodeType
     */
    public static List getNodeListFromType(List editorList, String nodeType)
    {
    	EObject rootObject = (EObject) editorList.get(0);
    	List refList = rootObject.eClass().getEAllReferences();
    	for (int i = 0 ; i < refList.size(); i++) {
    		EReference ref = (EReference) refList.get(i);
    		if(ref.getEReferenceType().getName().equals(nodeType)) {
    			return (List) rootObject.eGet(ref);
    		}
    	}
    	return new ArrayList();
    }
}
