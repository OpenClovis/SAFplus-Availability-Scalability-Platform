package com.clovis.cw.editor.ca.manageability.common;

import org.eclipse.jface.action.IStatusLineManager;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchPartSite;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.internal.PartSite;

/**
 * Class to set message in status bar
 * @author Pushparaj
 *
 */
public class StatusLineManagerUtils {

	/**
	 * Sets the error message in the status bar
	 * @param message
	 */
	public static void setErrorMessage(final String message) {
		PlatformUI.getWorkbench().getDisplay().syncExec(new Runnable() {
			public void run() {
				IWorkbenchPartSite site = PlatformUI.getWorkbench()
						.getActiveWorkbenchWindow().getActivePage()
						.getActivePart().getSite();
				PartSite pSite = (PartSite) site;
				IActionBars actionBars = pSite.getActionBars();
				if (actionBars == null)
					return;
				IStatusLineManager statusLineManager = actionBars
						.getStatusLineManager();
				if (statusLineManager == null)
					return;
				statusLineManager.setErrorMessage(message);
			}
		});
	}
	/**
	 * Set the message in the status bar
	 * @param message
	 */
	public static void setMessage(final String message) {
		PlatformUI.getWorkbench().getDisplay().syncExec(new Runnable() {
			public void run() {
				IWorkbenchPartSite site = PlatformUI.getWorkbench()
						.getActiveWorkbenchWindow().getActivePage()
						.getActivePart().getSite();
				PartSite pSite = (PartSite) site;
				IActionBars actionBars = pSite.getActionBars();
				if (actionBars == null)
					return;
				IStatusLineManager statusLineManager = actionBars
						.getStatusLineManager();
				if (statusLineManager == null)
					return;
				statusLineManager.setMessage(message);
			}
		});
	}
}
