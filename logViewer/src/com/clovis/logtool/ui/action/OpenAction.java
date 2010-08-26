/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.net.URL;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;

import com.clovis.logtool.record.manager.RecordManager;
import com.clovis.logtool.stream.FileStream;
import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.RecordPanel;
import com.clovis.logtool.utils.LogConstants;
import com.clovis.logtool.utils.LogUtils;

/**
 * Open Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class OpenAction extends Action {

	/**
	 * Constructs Open Action instance.
	 */
	public OpenAction() {
		setText("&Open@Ctrl+O");
		setToolTipText("Open Stream");

		try {
			setImageDescriptor(ImageDescriptor.createFromURL(new URL(
					"file:icons/logOpen.png")));
		} catch (Exception e) {
			e.printStackTrace();
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

		if(recordPanelFolder.getItemCount() > 10) {
			LogUtils.displayMessage(LogConstants.DISPLAY_MESSAGE_INFORMATION,
					"Operation not Allowed",
					"You can not open more than 10 Streams at a same time.");
			return;
		}

		FileSelectionDialog fileOpenDialog = new FileSelectionDialog(logDisplay
				.getShell());
		if (fileOpenDialog.open() != Dialog.OK) {
			return;
		}

		String logFilePath = fileOpenDialog.getLogFilePath();
		RecordPanel recordPanel = new RecordPanel(recordPanelFolder,
				fileOpenDialog.getRecordConfiguration());

		RecordManager recordManager = recordPanel.getRecordManager();
		recordManager.getRecordFetcher().setStream(
				new FileStream(logFilePath));
 
		CTabItem tabItem = new CTabItem(recordPanelFolder, SWT.CLOSE);
		tabItem.setText(logFilePath.substring(logFilePath.lastIndexOf(System
				.getProperty("file.separator")) + 1));
		tabItem.setToolTipText(logFilePath);
		tabItem.setControl(recordPanel);
		logDisplay.getRecordPanelFolder().setSelection(tabItem);

		recordManager.clearRecordBatch();
		recordPanel.updateRecordViewerData(true);

		logDisplay.getFilterPanel().setButtonStatus(true, true);
		logDisplay.getNavigationPanel().setButtonStatus(true, true, true, true);
	}
}
