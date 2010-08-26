/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/tree/TreeUI.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.tree;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.ui.part.DrillDownAdapter;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ListObjectListener;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.EnvironmentNotifierImpl;
/**
 * @author ravik
 *
 *  TreeViewer Class
 */

public class TreeUI extends TreeViewer
    implements Environment, ISelectionChangedListener
{
    private EnvironmentNotifier  _notifier = new EnvironmentNotifierImpl();
    private DrillDownAdapter     _drillDownAdapter;
    private ClassLoader          _classLoader;
    private ListObjectListener   _listener;
    /**
     * constructor
     * @param parent - Tree
     * @param classLoader - ClassLoader
     */
    public TreeUI(Tree parent, ClassLoader classLoader)
    {
        super(parent);
        _listener    = new ListObjectListener(this);
        parent.addDisposeListener(_listener);
        setLabelProvider(new TreeLabelProvider());
        setContentProvider(new TreeContentProvider());
        _classLoader = classLoader;
        _drillDownAdapter = new DrillDownAdapter(this);
        expandAll();
        addSelectionChangedListener(this);
    }
    /**
     * @return the notifier.
     */
    public EnvironmentNotifier getNotifier()
    {
        return _notifier;
    }
    /**
     * Returns Parent Environment.
     * @return Parent Environment.
     */
    public Environment getParentEnv()
    {
        return null;
    }
    /**
     * Change in input, update listeners.
     * @param input - Object
     * @param oldInput - Object
     */
    protected void inputChanged(Object input, Object oldInput)
    {
        NotifyingList newList = (NotifyingList) input;
        NotifyingList oldList = (NotifyingList) oldInput;
        // Add listener to new Input
        if (newList != null) {
            EcoreUtils.addListener(newList, _listener, -1);
        }
        // Remove listener from old input.
        if (oldList != null) {
            EcoreUtils.removeListener(oldList, _listener, -1);
        }
        super.inputChanged(input, oldInput);
    }
    /**
     * @author shubhada
     * This class listens for selection changes and notifies Environment
     * listeners.
     * @param e SelectionChangedEvent
     */
    public void selectionChanged(SelectionChangedEvent e)
    {
        _notifier.fireEnvironmentChange("selection");
    }
    /**
     * @param property Object
     * @return Object
     * Method returns the value of the property string
     */
    public Object getValue(Object property)
    {
        if (property.toString().equalsIgnoreCase("selection")) {
            return (StructuredSelection) getSelection();
        } else if (property.toString().equalsIgnoreCase("model")) {
            return getInput();
        } else if (property.toString().equalsIgnoreCase("shell")) {
            return getTree().getShell();
        } else if (property.toString().equalsIgnoreCase("treeviewer")) {
            return this;
        } else if (property.toString().equalsIgnoreCase("classloader")) {
            return _classLoader;
        }
        return null;
    }
    /**
     * 
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    public void setValue(Object property, Object value)
    {
             
    }
}
