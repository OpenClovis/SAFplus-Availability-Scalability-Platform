/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/ListObjectListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui;

import java.util.List;

import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;

import org.eclipse.jface.viewers.Viewer;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;

import com.clovis.common.utils.ecore.EcoreUtils;
/**
 * This class listens for changes in EList and EObjects and keep
 * table updated.
 * @author Shubhada
 */
public class ListObjectListener extends AdapterImpl
    implements DisposeListener
{
    private Viewer _viewer;
    /**
     * Constructor.
     * @param viewer Viewer
     */
    public ListObjectListener(Viewer viewer)
    {
        _viewer = viewer;
    }
    /**
     * Listener for Table Dispose. It is required to remove
     * listener when table is disposed.
     * @param e Dispose Event
     */
    public void widgetDisposed(DisposeEvent e)
    {
        _viewer.getControl().removeDisposeListener(this);
        // Remove listener from input.
        EcoreUtils.removeListener(_viewer.getInput(), this, -1);
        _viewer = null;
    }
    /**
     * Notification Callback.
     * @param changeMesg Change Event
     */
    public void notifyChanged(Notification changeMesg)
    {
    	if (_viewer != null) {
	        switch (changeMesg.getEventType()) {
	            case Notification.REMOVING_ADAPTER: break;
	            case Notification.MOVE:
	                _viewer.refresh();
	                break;
	            case Notification.SET:
	                if (!changeMesg.isTouch()) {
	                    _viewer.refresh();
	                }
	                break;
	            case Notification.ADD:
	                EcoreUtils.addListener(changeMesg.getNewValue(), this, -1);
	                _viewer.refresh();
	                break;
	            case Notification.ADD_MANY:
	                List objs = (List) changeMesg.getNewValue();
	                for (int i = 0; i < objs.size(); i++) {
	                    EcoreUtils.addListener(objs.get(i), this, -1);
	                }
	                _viewer.refresh();
	                break;
	            case Notification.REMOVE_MANY:
	                objs = (List) changeMesg.getOldValue();
	                for (int i = 0; i < objs.size(); i++) {
	                    EcoreUtils.removeListener(objs.get(i), this, -1);
	                }
	                _viewer.refresh();
	                break;
	            case Notification.REMOVE:
	                EcoreUtils.removeListener(changeMesg.getOldValue(), this, -1);
	                _viewer.refresh();
	                break;
	        }
    	}
    }
}
