/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/log/LogAppender.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.log;

import  org.eclipse.core.runtime.ILog;
import  org.eclipse.core.runtime.Status;

import  org.apache.log4j.Priority;
import  org.apache.log4j.AppenderSkeleton;
import  org.apache.log4j.spi.LoggingEvent;
import  org.apache.log4j.spi.ThrowableInformation;

import  com.clovis.common.utils.UtilsPlugin;
/**
 * @author nadeem
 *
 * Appender class to log events in Eclipse Log Format.
 */
public class LogAppender extends AppenderSkeleton
{
    private final String _id;
    private final ILog _iLog;

    public LogAppender(ILog iLog) {
        _iLog = iLog;
       _id   = _iLog.getBundle().getSymbolicName();
    }
    /**
     * Close Implementation (Does nothing.)
     */
    public void close() {}
    /**
     * If Layout is required.
     * @return false.
     * @see org.apache.log4j.Appender#requiresLayout()
     */
    public boolean requiresLayout() { return false; }
    /**
     * Brings ErrorLog View on top.
     */
    private static void bringErrorViewOnTop()
    {
        try {
            UtilsPlugin.getDefault().getWorkbench().
                getActiveWorkbenchWindow().getActivePage().
                    showView("org.eclipse.pde.runtime.LogView");
        } catch (Exception e) {}
    }
    /**
     * Log the Event in Eclipse Log.
     * @param event LogEvent to Log.
     */
    protected void append(LoggingEvent event) {
        int sev   = Status.OK;
        int level = ((Priority) event.getLevel()).toInt();
        switch (level) {
            case Priority.FATAL_INT:    
            case Priority.ERROR_INT: sev = Status.ERROR;   break;
            case Priority.INFO_INT:  sev = Status.INFO;    break;
            case Priority.WARN_INT:  sev = Status.WARNING; break;
            default: return;
        }
        bringErrorViewOnTop();
        ThrowableInformation tInfo = event.getThrowableInformation();
        Throwable t = (tInfo != null) ? tInfo.getThrowable() : null;
        _iLog.log(new Status(sev, _id, sev, event.getMessage().toString(), t));
    }
}
