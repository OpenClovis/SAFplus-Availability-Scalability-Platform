/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.net.URL;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;

import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Close Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CloseAction extends Action {

	/**
	 * Flag to identify simple close or close all type.
	 */
	private boolean _allFlag;

	/**
	 * Constructs Close Action instance.
	 */
	public CloseAction(boolean allFlag) {
		_allFlag = allFlag;

		if(_allFlag) {
			setText("Close &All@Ctrl+Shift+C");
			setToolTipText("Close All Streams");
		} else {
			setText("&Close@Ctrl+C");
			setToolTipText("Close Stream");
			try {
				setImageDescriptor(ImageDescriptor.createFromURL(new URL(
						"file:icons/logClose.png")));
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.IAction#run()
	 */
	public void run() {
		LogDisplay logDisplay = LogDisplay.getInstance();
		CTabFolder recordPanelFolder = logDisplay.getRecordPanelFolder();

		if(recordPanelFolder.getItemCount() == 0) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_INFORMATION,
					"Stream not Opened",
					"There is no stream opened at present.");
			return;
		}

		if(_allFlag) {
			CTabItem tabItems[] = recordPanelFolder.getItems();

			for(int i=0 ; i<tabItems.length ; i++) {
				tabItems[i].getControl().dispose();
				tabItems[i].dispose();
			}
		} else {
			recordPanelFolder.getSelection().getControl().dispose();
			recordPanelFolder.getSelection().dispose();
		}

		if(logDisplay.getRecordPanelFolder().getItemCount() == 0) {
			logDisplay.getFilterPanel().setButtonStatus(false, false);
			logDisplay.getNavigationPanel().setButtonStatus(false, false, false, false);
		}
	}
}
