/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/log/Log.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.log;

import java.io.File;
import  java.util.Hashtable;

import  org.apache.log4j.Level;
import  org.apache.log4j.Logger;

import  org.osgi.framework.Bundle;

import org.eclipse.core.runtime.ILog;
import org.eclipse.core.runtime.IStatus;
import  org.eclipse.core.runtime.Plugin;
import org.eclipse.core.runtime.Status;

/**
 * <p>Implementation of {@link Log} that maps directly to a Log4J
 * <strong>Logger</strong>.  Initial configuration of the corresponding
 * Logger instances should be done in the usual manner, as outlined in
 * the Log4J documentation.</p>
 */
public class Log implements org.apache.commons.logging.Log
{
	private ILog _log = null; // for displaying in Log view
	private Logger _logger = null; // for writing into .log file
	private String _pluginID = "";
	private static final Hashtable LOGS = new Hashtable();
	private static final String    FQCN = Log.class.getName();
	
	/**
	 * Constructor
	 * @param log ILog
	 * @param id plugin ID
	 */
	private Log(ILog log, String id) {
		_log = log;
		_pluginID = id;
		
		// Check .clovis folder in ${user.home} location and creates if does not exist
		File logFileLoc = new File(System.getProperty("user.home") + File.separator + ".clovis");
		if(!logFileLoc.exists())
			logFileLoc.mkdir();
		
		_logger = Logger.getLogger(id);
		LOGS.put(_pluginID, this);
	}
	/**
	 * Get Log instance for a bundle.
	 * @param plugin Plugin
	 * @return Log
	 */
	public static Log getLog(Plugin plugin)
    {
		String id = "";
		Bundle bundle = plugin.getBundle();
		if (bundle != null) {
			id = bundle.getSymbolicName();
		} else {
			id = plugin.toString();
		}
		Log log = (Log)LOGS.get(id);
		if (log == null) {
			log = new Log(plugin.getLog(), id); 
		}
		return log;
    }
	/**
	 * Log an error to the Log4j Logger with <code>INFO</code> priority.
	 */
	public void info(Object message) {
		/*_log.log(new Status(IStatus.INFO, _pluginID, IStatus.INFO, message
				.toString(), null));*/
		_logger.log(FQCN, Level.INFO, message, null);
	}
	/**
	 * Log an error to the Log4j Logger with <code>INFO</code> priority.
	 */
	public void info(Object message, Throwable t) {
		/*_log.log(new Status(IStatus.INFO, _pluginID, IStatus.INFO, message
				.toString(), t));*/
		_logger.log(FQCN, Level.INFO, message, t);
	}
	/**
	 * Log an error to the Log4j Logger with <code>WARN</code> priority.
	 */
	public void warn(Object message) {
		_log.log(new Status(IStatus.WARNING, _pluginID, IStatus.WARNING,
				message.toString(), null));
		_logger.log(FQCN, Level.WARN, message, null);
	}
	/**
	 * Log an error to the Log4j Logger with <code>WARN</code> priority.
	 */
	public void warn(Object message, Throwable t) {
		_log.log(new Status(IStatus.WARNING, _pluginID, IStatus.WARNING,
				message.toString(), t));
		_logger.log(FQCN, Level.WARN, message, t);
	}
	/**
	 * Log an error to the Log4j Logger with <code>ERROR</code> priority.
	 */
	public void error(Object message) {
		_log.log(new Status(IStatus.ERROR, _pluginID, IStatus.ERROR,
				message.toString(), null));
		_logger.log(FQCN, Level.ERROR, message, null);
	}
	/**
	 * Log an error to the Log4j Logger with <code>ERROR</code> priority.
	 */
	public void error(Object message, Throwable t) {
		_log.log(new Status(IStatus.ERROR, _pluginID, IStatus.ERROR,
				message.toString(), t));
		_logger.log(FQCN, Level.ERROR, message, t) ;
	}
	/**
	 * Log an error to the Log4j Logger with <code>TRACE</code> priority.
	 */
	public void trace(Object message) {
		_logger.log(FQCN, Level.ALL, message, null);
	}
	/**
	 * Log an error to the Log4j Logger with <code>TRACE</code> priority.
	 */
	public void trace(Object message, Throwable t) {
		_logger.log(FQCN, Level.ALL, message, t);
	}
	/**
	 * Log an error to the Log4j Logger with <code>DEBUG</code> priority.
	 */
	public void debug(Object message) {
		_logger.log(FQCN, Level.DEBUG, message, null);
	}
	/**
	 * Log an error to the Log4j Logger with <code>DEBUG</code> priority.
	 */
	public void debug(Object message, Throwable t) {
		_logger.log(FQCN, Level.DEBUG, message, t);
	}
	/**
	 * Log an error to the Log4j Logger with <code>FATAL</code> priority.
	 */
	public void fatal(Object message) {
		_logger.log(FQCN, Level.FATAL, message, null);
	}
	/**
	 * Log an error to the Log4j Logger with <code>FATAL</code> priority.
	 */
	public void fatal(Object message, Throwable t) {
		_logger.log(FQCN, Level.FATAL, message, t);
	}
	/**
	 * Return the native Logger instance we are using.
	 */
	public Logger getLogger()
	{
		return (_logger);
	}
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>DEBUG</code> priority.
	 */
	public boolean isDebugEnabled()
	{
		return _logger.isDebugEnabled();
	}
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>INFO</code> priority.
	 */
	public boolean isInfoEnabled()
	{
		return _logger.isInfoEnabled();
	}
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>TRACE</code> priority.
	 * For Log4J, this returns the value of <code>isDebugEnabled()</code>
	 */
	public boolean isTraceEnabled()
	{
		return _logger.isDebugEnabled();
	}
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>WARN</code> priority.
	 */
	public boolean isWarnEnabled()
	{
		return _logger.isEnabledFor(Level.WARN);
	}
	 /**
	 * Check whether the Log4j Logger used is enabled for
     * <code>ERROR</code> priority.
	 */
	public boolean isErrorEnabled()
	{
		return _logger.isEnabledFor(Level.ERROR);
	}
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>FATAL</code> priority.
	 */
	public boolean isFatalEnabled()
	{ 
		return _logger.isEnabledFor(Level.FATAL);
	}
}
 
