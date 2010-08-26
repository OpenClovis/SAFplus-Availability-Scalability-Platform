/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/tree/TreeNode.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.tree;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
/**
 *
 * @author shubhada
 *Tree Node representation from EObject input to a tree.
 */

public class TreeNode
{
    EStructuralFeature _feature = null;
    TreeNode _parent             = null;
    Object _value              = null;
    /**
     * Constructor
     * @param feature EStructuralFeature
     * @param parent EObject
     * @param value Object
     */
    public TreeNode(EStructuralFeature feature, TreeNode parent, Object value)
    {
        _feature = feature;
        _parent = parent;
        _value = value;
    }
    /**
     *
     * @return feature of the TreeNode.
     */
    public EStructuralFeature getFeature()
    {
        return _feature;
    }
    /**
     *
     * @return parent of the TreeNode.
     */
    public TreeNode getParent()
    {
        return _parent;
    }
    /**
     *
     * @return Value of the TreeNode.(i.e corresponding EObject)
     */
    public Object getValue()
    {
        return _value;
    }
    /**
     * @ return Name of the Node
     */
    public String getName() {
    	return EcoreUtils.getName((EObject)_value);
    }
}

