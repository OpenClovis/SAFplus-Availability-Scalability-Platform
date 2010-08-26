/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor.model;

import java.util.ArrayList;
import java.util.List;


/**
 *
 * @author pushparaj
 *
 * Class for Container Nodes
 */
public class ContainerNodeModel extends NodeModel
{

    public static final String CHILDREN     = "Children";

    protected List             _childrenList = new ArrayList();

    /**
     * Constructor
     *
     * @param name name for EClass
     */
    public ContainerNodeModel(String name)
    {
        super(name);    	
    }

    

    /**
     * add nodes into container(parent).
     *
     * @param child Node
     */
    public void addChild(Base child)
    {
        child.setParent(this);
        addChild(child, -1);
    }

    /**
     * add nodes into container(parent).
     *
     * @param child Node
     * @param index index
     */
    public void addChild(Base child, int index)
    {

        if (index >= 0) {
            _childrenList.add(index, child);
        } else {
            _childrenList.add(child);
        }
        child.setParent(this);
        _listeners.firePropertyChange(CHILDREN, null, child);
        _listeners.firePropertyChange("parent", null, this);
    }

    /**
     * returns childs list.
     *
     * @return List of children
     */
    public List getChildren()
    {
        return _childrenList;
    }

    /**
     * remove child from parent.
     *
     * @param child Node
     */
    public void removeChild(IElement child)
    {
        _childrenList.remove(child);
        _listeners.firePropertyChange(CHILDREN, null, child);
    }
}
