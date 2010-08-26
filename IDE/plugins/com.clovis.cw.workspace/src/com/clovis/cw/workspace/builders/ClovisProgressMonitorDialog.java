package com.clovis.cw.workspace.builders;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.StringTokenizer;

import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.swt.widgets.Shell;

/**
 * 
 * @author matt
 *
 * Clovis specific progress monitor to be used with long running 'build' type
 * actions. This class overloads the cancelPressed method to kill the running
 * task via a separate thread. It also places a message in the console that
 * can be detected by the associated logger class so that a final message can
 * be displayed to the user indicating that the action was cancelled.
 */
public class ClovisProgressMonitorDialog extends ProgressMonitorDialog {

	public static final String CANCEL_MESSAGE = "Cancel Button Pressed";
	String _actionCommand = null;

	/**
	 * Constuctor
	 * @param parent
	 * @param actionCommand
	 */
	public ClovisProgressMonitorDialog(Shell parent, String actionCommand)
	{
        super(parent);
        _actionCommand = actionCommand;
    }

	/**
	 * Constuctor
	 * @param parent
	 * @param actionCommand
	 */
	public ClovisProgressMonitorDialog(Shell parent)
	{
        super(parent);
        _actionCommand = null;
    }

    /**
     * Handles the cancel pressed action. This method uses a separate thread to
     * kill the process (and sub-processes) identified by the _actionCommand that
     * was passed to the constructor.
     */
	protected void cancelPressed()
    {
        super.cancelPressed();
        
        // if no action command then just print message and leave
        if (_actionCommand == null)
        {
    		System.out.println(CANCEL_MESSAGE);
    		System.out.flush();
    		return;
        }
    	
    	try
    	{
	    	// run a command to find the id and command of all running processes
    		String line;
	    	String cmnd = new String("ps -e -o pid,command");
	    	Process proc = Runtime.getRuntime().exec(cmnd);
	        BufferedReader input = new BufferedReader(new InputStreamReader(proc.getInputStream()));
	    	proc.waitFor();

	        // loop through the list of processes
	    	while ((line = input.readLine()) != null) {

		        // if the command used to start the process matches the action
	    		//  command then kill the process
	    		if (line.indexOf(_actionCommand) > -1)
	        	{
			        // output to the console that the action is being cancelled
	    			System.out.println(CANCEL_MESSAGE);
	        		System.out.flush();

	        		// get the process id
	        		StringTokenizer tokenizer = new StringTokenizer(line, " ", false);
	        		String pid = tokenizer.nextToken();
			    	
	        		// run a command to kill the process and its sub-processes
	        		Process procKill = Runtime.getRuntime().exec("pkill -P " + pid);
			        BufferedReader input2 = new BufferedReader(new InputStreamReader(procKill.getErrorStream()));
	        		int retVal = procKill.waitFor();
	        		
	        		// if the process could not be stopped then report what happened
	        		if (retVal > 0)
	        		{
				        System.out.println("Error stopping the action.");
				    	String line2;
	        			while ((line2 = input2.readLine()) != null)
				        {
			        		System.out.println(line2);
				        }
	        		}
	        		input2.close();
	        	}
	        }
	        input.close();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}
    }
}
