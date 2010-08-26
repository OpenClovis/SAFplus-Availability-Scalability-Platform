/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/list/ListView.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.list;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ListViewer;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.List;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.ListObjectListener;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.EnvironmentNotifierImpl;
/**
 * @author ravik
 * List Viewer Class
 */
public class ListView extends ListViewer
    implements Environment, ISelectionChangedListener
{
    private EnvironmentNotifier _notifier = new EnvironmentNotifierImpl();
    private ListObjectListener  _listener;

    /**
     * Constructor
     * @param parent List
     */
    public ListView(List parent)
    {
        super(parent);
        setLabelProvider(new ListLabelProvider());
        setContentProvider(new ListContentProvider());
        _listener = new ListObjectListener(this);
        parent.addDisposeListener(_listener);
        addSelectionChangedListener(this);
    }
    /**
     * @return notifier instance
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
     * @param input    new input
     * @param oldInput old Input.
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
     * Implementation of getValue method
     * @param property Object
     * @return value for the property
     */
    public Object getValue(Object property)
    {
        if (property.toString().equalsIgnoreCase("selection")) {
            return (StructuredSelection) getSelection();
        } else if (property.toString().equalsIgnoreCase("model")) {
            return getInput();
        } else if (property.toString().equalsIgnoreCase("shell")) {
            return getList().getShell();
        } else if (property.toString().equalsIgnoreCase("listviewer")) {
            return this;
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
