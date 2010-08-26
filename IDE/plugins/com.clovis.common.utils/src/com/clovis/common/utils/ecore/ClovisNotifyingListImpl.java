/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ecore;

import org.eclipse.emf.common.notify.impl.NotifierImpl;
import org.eclipse.emf.common.notify.impl.NotifyingListImpl;
/**
 * @author manish
 * This is NotifyingListImpl with notifier.
 */
public class ClovisNotifyingListImpl extends NotifyingListImpl
{
    //Notifier
    private NotifierImpl _notifier = new NotifierImpl();
    /**
     * Return notifier for this List.
     */
	public Object getNotifier()      {  return _notifier;	 }
    
    /**
     * Returns <code>true</code>.
     * @return <code>true</code>.
     */
    protected boolean isNotificationRequired() { return true; }
}
