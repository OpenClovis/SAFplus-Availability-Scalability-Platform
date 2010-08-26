package com.clovis.common.log;

import  org.apache.log4j.Level;
import  org.apache.log4j.Logger;
/**
 * <p>Implementation of {@link Log} that maps directly to a Log4J
 * <strong>Logger</strong>.  Initial configuration of the corresponding
 * Logger instances should be done in the usual manner, as outlined in
 * the Log4J documentation.</p>
 */
public class Log implements org.apache.commons.logging.Log
{
	/** The fully qualified name. */
	private static final String FQCN = Log.class.getName();
    
	private String _name = null;
	private Logger _logger = null;
	/**
	 * Base constructor.
	 */
	public Log(String name) {
		this(Logger.getLogger(name));
	}
	/** For use with a log4j factory.
	 */
	public Log(Logger logger ) {
		_logger =logger;
		_name   = logger.getName();
	}
	// --------------------------------------------------------- Implementation
	/**
	 * Log a message to the Log4j Logger with <code>TRACE</code> priority.
	 * Currently logs to <code>DEBUG</code> level in Log4J.
	 */
	public void trace(Object message) {
		_logger.log(FQCN, Level.ALL, message, null );
	}
	/**
	 * Log an error to the Log4j Logger with <code>TRACE</code> priority.
	 * Currently logs to <code>DEBUG</code> level in Log4J.
	 */
	public void trace(Object message, Throwable t) {
		_logger.log(FQCN, Level.ALL, message, t );
	}
	/**
	 * Log a message to the Log4j Logger with <code>DEBUG</code> priority.
	 */
	public void debug(Object message) {
		_logger.log(FQCN, Level.DEBUG, message, null );
	}
	/**
	 * Log an error to the Log4j Logger with <code>DEBUG</code> priority.
	 */
	public void debug(Object message, Throwable t) {
		_logger.log(FQCN, Level.DEBUG, message, t );
	}
	/**
	 * Log a message to the Log4j Logger with <code>INFO</code> priority.
	 */
	public void info(Object message) {
		_logger.log(FQCN, Level.INFO, message, null );
	}
	/**
	 * Log an error to the Log4j Logger with <code>INFO</code> priority.
	 */
	public void info(Object message, Throwable t) {
		_logger.log(FQCN, Level.INFO, message, t );
	}
	/**
	 * Log a message to the Log4j Logger with <code>WARN</code> priority.
	 */
	public void warn(Object message) {
		_logger.log(FQCN, Level.WARN, message, null );
	}
	/**
	 * Log an error to the Log4j Logger with <code>WARN</code> priority.
	 */
	public void warn(Object message, Throwable t) {
		_logger.log(FQCN, Level.WARN, message, t );
	}
	/**
	 * Log a message to the Log4j Logger with <code>ERROR</code> priority.
	 */
	public void error(Object message) {
		_logger.log(FQCN, Level.ERROR, message, null );
	}
	/**
	 * Log an error to the Log4j Logger with <code>ERROR</code> priority.
	 */
	public void error(Object message, Throwable t) {
		_logger.log(FQCN, Level.ERROR, message, t );
	}
	/**
	 * Log a message to the Log4j Logger with <code>FATAL</code> priority.
	 */
	public void fatal(Object message) {
		_logger.log(FQCN, Level.FATAL, message, null );
	}
	/**
	 * Log an error to the Log4j Logger with <code>FATAL</code> priority.
	 */
	public void fatal(Object message, Throwable t) {
		_logger.log(FQCN, Level.FATAL, message, t );
	}
	/**
	 * Return the native Logger instance we are using.
	 */
	public Logger getLogger()       { return (_logger); }
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>DEBUG</code> priority.
	 */
	public boolean isDebugEnabled() { return _logger.isDebugEnabled(); }
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>INFO</code> priority.
	 */
	public boolean isInfoEnabled()  { return _logger.isInfoEnabled(); }
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>TRACE</code> priority.
	 * For Log4J, this returns the value of <code>isDebugEnabled()</code>
	 */
	public boolean isTraceEnabled() { return _logger.isDebugEnabled(); }
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>WARN</code> priority.
	 */
	public boolean isWarnEnabled()  { return _logger.isEnabledFor(Level.WARN); }
	 /**
	 * Check whether the Log4j Logger used is enabled for
     * <code>ERROR</code> priority.
	 */
	public boolean isErrorEnabled() { return _logger.isEnabledFor(Level.ERROR); }
	/**
	 * Check whether the Log4j Logger used is enabled for
     * <code>FATAL</code> priority.
	 */
	public boolean isFatalEnabled() { return _logger.isEnabledFor(Level.FATAL); }
}
