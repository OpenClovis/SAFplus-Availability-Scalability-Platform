/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import org.eclipse.emf.ecore.EObject;

/**
 * @author pushparaj
 * Utils class without any implementation.
 * Specific editors to override methods to provide
 * implementation.
 */
public class GEUtils
{
    /**
     * List of All EObjects.
     */
    protected Object[] _eObjects;
    /**
     * constructs GEUtils.
     * @param objs EObjects list
     */
    public GEUtils(Object[] objs)
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
}
