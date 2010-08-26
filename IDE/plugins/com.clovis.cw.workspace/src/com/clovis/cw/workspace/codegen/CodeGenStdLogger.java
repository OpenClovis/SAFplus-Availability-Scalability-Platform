/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/codegen/CodeGenStdLogger.java $
 * $Author: swapnesh $
 * $Date: 2007/01/11 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.codegen;

import java.io.PrintStream;

import org.apache.tools.ant.BuildEvent;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DefaultLogger;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.util.StringUtils;


/**
 * @author Pushparaj
 * Logger for the Builder. Prints output and error in the console
 * of ClovisWorks instead of Std Out and Std Error.
 */
public class CodeGenStdLogger extends DefaultLogger
{
	/** Time of the start of the code generation */
    private long startTime = System.currentTimeMillis();
    
    /**
     * Responds to a code generation being started by just remembering the current time.
     *
     * @param event Ignored.
     */
    public void buildStarted(BuildEvent event) {
        startTime = System.currentTimeMillis();
    }
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
     * Prints whether the code generation succeeded or failed,
     * any errors the occurred during the code generation, and
     * how long the code generation took.
     *
     * @param event An event with any relevant extra information.
     *              Must not be <code>null</code>.
     */
    public void buildFinished(BuildEvent event) {
    	Throwable error = event.getException();
        StringBuffer message = new StringBuffer();

        if (error == null) {
            message.append(StringUtils.LINE_SEP);
            message.append("CODE GENERATION SUCCESSFUL");
        } else {
            message.append(StringUtils.LINE_SEP);
            message.append("CODE GENERATION FAILED");
            message.append(StringUtils.LINE_SEP);

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
        message.append(StringUtils.LINE_SEP);
        message.append("Total time: ");
        message.append(formatTime(System.currentTimeMillis() - startTime));

        String msg = message.toString();
        if (error == null) {
            printMessage(msg, out, Project.MSG_VERBOSE);
        } else {
            printMessage(msg, err, Project.MSG_ERR);
        }
        log(msg);
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
    protected void printMessage(final String message, final PrintStream stream,
			final int priority) {
    	if(!message.equals("BUILD SUCCESSFUL"))
    		stream.println(message);
	}
    /**
     * Constructor.
     * Adds console in console view and assign output and error
     * stream to console.
     */
    public CodeGenStdLogger()
    {
        super();
	err = new PrintStream(System.err);
	out = new PrintStream(System.out);
    }
}
