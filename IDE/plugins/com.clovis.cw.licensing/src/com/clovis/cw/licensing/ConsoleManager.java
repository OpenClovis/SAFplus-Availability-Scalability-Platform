package com.clovis.cw.licensing;

import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsole;
import org.eclipse.ui.console.IConsoleManager;
import org.eclipse.ui.console.IConsoleView;
import org.eclipse.ui.console.MessageConsole;
import org.eclipse.ui.console.MessageConsoleStream;

/**
 * 
 * @author pushparaj
 * Class to create console for IDE server and display message
 * while IDE communicating with IDE server 
 */
public class ConsoleManager {
	private String _consoleName = "IDE Server";
	private MessageConsole _remoteMsgConsole;
	
	/**
	 * Creates instance for ConsoleManager
	 */
	public ConsoleManager() {
		
	}
	private MessageConsole findConsole() {
		ConsolePlugin plugin = ConsolePlugin.getDefault();
		IConsoleManager conMan = plugin.getConsoleManager();
		IConsole[] existing = conMan.getConsoles();
		for (int i = 0; i < existing.length; i++)
			if (_consoleName.equals(existing[i].getName()))
				return (MessageConsole) existing[i];
		//no console found, so create a new one
		MessageConsole myConsole = new MessageConsole(_consoleName, null);
		conMan.addConsoles(new IConsole[] { myConsole });
		IWorkbenchPage page = PlatformUI.getWorkbench()
		.getActiveWorkbenchWindow().getActivePage();
		IConsoleView view = null;
		try {
			view = (IConsoleView) page.showView(_consoleName);
		} catch (PartInitException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		view.display(myConsole);

		return myConsole;
	}
	public void writeMessage(String msg) {
		if(_remoteMsgConsole == null)
			_remoteMsgConsole = findConsole();
		MessageConsoleStream out = _remoteMsgConsole.newMessageStream();
		out.println(msg);
	}
}
