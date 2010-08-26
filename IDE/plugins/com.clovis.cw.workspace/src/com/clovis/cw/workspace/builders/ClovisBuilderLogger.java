/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/builders/ClovisBuilderLogger.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.builders;

import java.io.PrintStream;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsole;
import org.eclipse.ui.console.MessageConsole;

/**
 * @author Pushparaj
 * Logger for the Builder.
 */
public class ClovisBuilderLogger extends ClovisActionLogger {

	/**
     * Constructor.
     * Sets console title as well as success and fail messages.
     */
    public ClovisBuilderLogger()
    {
    	super();
    	_identifier = "BUILD PROJECT";
    	
    	IConsole oldconsoles[] = ConsolePlugin.getDefault().getConsoleManager().getConsoles();
    	StringBuffer buffer = new StringBuffer();
    	_MC = new MessageConsole("Build Project", null);
    	for (int i = 0; i < oldconsoles.length; i++) {
        	IConsole console = oldconsoles[i];
        	if(console.getName().equals("Configure Project")){
        		buffer = new StringBuffer();
        		final MessageConsole msgConsole = (MessageConsole) console;
        		buffer.append(msgConsole.getDocument().get());
        		if(msgConsole.getDocument().get().length() > 0) {
        			buffer.append("\n");
        			buffer.append("\n");
        		}
        		Display.getDefault().syncExec(new Thread(){
        			public void run() {
        				msgConsole.getDocument().set("");
        			}
        		});
        		_MC.getDocument().set(buffer.toString());
        	}
        }
        IConsole consoles[] = { _MC };
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
}
