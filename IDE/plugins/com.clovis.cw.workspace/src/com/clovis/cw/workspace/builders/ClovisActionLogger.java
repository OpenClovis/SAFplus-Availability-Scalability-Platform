package com.clovis.cw.workspace.builders;

import java.io.PrintStream;

import org.apache.tools.ant.BuildEvent;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DefaultLogger;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.util.StringUtils;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsole;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.console.MessageConsole;

/**
 * @author Matt
 * Generic logger for all project actions. Prints output and error in the
 * eclipse console instead of standard out and standard error.
 */

public class ClovisActionLogger extends DefaultLogger {

	protected MessageConsole _MC = null;
	protected IOConsoleOutputStream _outS = null;
	protected IOConsoleOutputStream _errS = null;
	
	protected String _identifier;
	public static final String _successMsg = "SUCCESSFUL";
	public static final String _failMsg = "FAILED";
	public static final String _cancelMsg = "CANCELLED";

	/** Time of the start of the action */
    protected long startTime = System.currentTimeMillis();

	/**
     * Set Error Stream. Does nothing as it is set in Constructor.
     * @param err Unused
     */
    public void setErrorPrintStream(PrintStream err)
    {
    }

    /**
     * Set Output Stream. Does nothing as it is set in Constructor.
     * @param output Unused
     */
    public void setOutputPrintStream(PrintStream output)
    {
    }
    /**
     * Empty Constructor.
     * Sub-Class should create console
     */
    public ClovisActionLogger() {
    	
    }
    /**
     * Constructor.
     * Create the console, add it to the console view and assign output and error
     * stream to console.
     */
    public ClovisActionLogger(String identifier)
    {
        super();

    	_identifier = identifier.toUpperCase();

    	_MC = new MessageConsole(identifier, null);
		IConsole[] consoles = { _MC };
		_outS = _MC.newOutputStream();
		_errS = _MC.newMessageStream();
		ConsolePlugin.getDefault().getConsoleManager().addConsoles(consoles);
		_MC.activate();
		ConsolePlugin.getDefault().getConsoleManager().showConsoleView(_MC);
		ConsolePlugin.getDefault().getConsoleManager().refresh(_MC);

		Display.getDefault().syncExec(new Runnable() {
			public void run() {
				_errS.setFontStyle(SWT.ITALIC);
				_errS.setColor(new Color(null, 255, 0, 0));
			}
		});
        err = new PrintStream(_errS);
        out = new PrintStream(_outS);
    }

    /**
     * Called when the action completes. Checks whether or not the build
     * was cancelled and prints a corresponding message.
     */
    public void buildFinished(BuildEvent event)
    {
    	try {
    		Thread.sleep(1000);
    	} catch (InterruptedException e) {
    		
    	}
        if (checkForCancel())
        {
        	out.flush();
        	err.flush();
            printMessage(_identifier + " " + _cancelMsg, err, Project.MSG_ERR);
            //printMessage("ACTION CANCELLED", err, Project.MSG_ERR);
            return;
        }

    	Throwable error = event.getException();
        StringBuffer message = new StringBuffer();

        String customError = null;
        if (error == null)
        {
        	customError = checkForCustomError();
        }

        if (error == null && customError == null) {
            message.append(StringUtils.LINE_SEP);
            message.append(_identifier + " " + _successMsg);
        } else {
            message.append(StringUtils.LINE_SEP);
            message.append(_identifier + " " + _failMsg);
            message.append(StringUtils.LINE_SEP);

            if (customError != null)
            {
            	message.append(customError).append(lSep);
            } else {
	            if (Project.MSG_VERBOSE <= msgOutputLevel
	                || !(error instanceof BuildException)) {
	                message.append(StringUtils.getStackTrace(error));
	            } else {
	                if (error instanceof BuildException) {
	                    message.append(error.toString()).append(lSep);
	                } else {
	                    message.append(error.getMessage()).append(lSep);
	                }
	            }
            }
        }
        message.append(StringUtils.LINE_SEP);
        message.append("Total time: ");
        message.append(formatTime(System.currentTimeMillis() - startTime));

        String msg = message.toString();
        if (error == null) {
            printMessage(msg, out, Project.MSG_VERBOSE);
        } else {
            printMessage(msg, err, Project.MSG_ERR);
        }
    }
    
	/**
	 * Prints a message to a PrintStream.
	 *
	 * @param message  The message to print.
	 *                 Should not be <code>null</code>.
	 * @param stream   A PrintStream to print the message to.
	 *                 Must not be <code>null</code>.
	 * @param priority The priority of the message.
	 *                 (Ignored in this implementation.)
	 */
	protected void printMessage(final String message, final PrintStream stream, final int priority)
	{
		if(!message.equals(_identifier + " " + _successMsg) && !message.equals("BUILD SUCCESSFUL"))
		{
			stream.println(message);
		}
	}

    /**
     * Checks the text in the console window for a message indicating that
     * the action was cancelled by the user.
     * @return
     */
    protected boolean checkForCancel()
    {
    	String docText = _MC.getDocument().get();
    	
    	if (docText.indexOf(ClovisProgressMonitorDialog.CANCEL_MESSAGE) >= 0
    	 || docText.indexOf("Build cancelled") >= 0)
    	{
    		return true;
    	}
    	
    	return false;
    }
    
    /**
     * Function to check for specific errors which are not detected by the build
     * scripts. Specific action loggers can override this method to check for and
     * return the errors. This will cause the action logger to display an error
     * message and report that the action failed. See CodeGenLogger.java for an
     * example.
     * @return
     */
    protected String checkForCustomError()
    {
    	return null;
    }

}
