/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import com.clovis.logtool.record.manager.RecordConfiguration;
import com.clovis.logtool.utils.LogConstants;

/**
 * Dialog for selecting the log file and the config file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FileSelectionDialog extends TitleAreaDialog {

	private Text _logFilePathControl;

	private Text _configFilePathControl;

	private String _logFilePath;

	private String _configFilePath;

	private RecordConfiguration _recordConfiguration;

	/**
	 * Constructor.
	 * 
	 * @param parentShell
	 */
	public FileSelectionDialog(Shell parentShell) {
		super(parentShell);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Group(parent, SWT.SHADOW_ETCHED_IN);
		composite.setLayout(new GridLayout(3, false));
		composite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		Label label = new Label(composite, SWT.NONE);
		label.setText("Log File:");

		_logFilePathControl = new Text(composite, SWT.BORDER | SWT.READ_ONLY);
		_logFilePathControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL,
				true, false));

		Button button = new Button(composite, SWT.NONE);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				selectLogFile();
			}
		});

		label = new Label(composite, SWT.NONE);
		label.setText("Config File:");

		_configFilePathControl = new Text(composite, SWT.BORDER | SWT.READ_ONLY);
		_configFilePathControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL,
				true, false));

		button = new Button(composite, SWT.NONE);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				selectConfigFile();
			}
		});

		getShell().setText("File Selection Dialog");
		setTitle("Select required files");
		return composite;
	}

	/**
	 * Called when log file is selected.
	 */
	private void selectLogFile() {
		FileDialog fileDialog = new FileDialog(getShell(), SWT.OPEN);
		fileDialog.setText("Select Log File");
		fileDialog.setFilterPath(System.getProperty("user.home"));

		String logFilePath = fileDialog.open();
		if (logFilePath == null) {
			if (_logFilePath == null) {
				setErrorMessage("Log file is not selected");
			}
			return;
		} else {
			_logFilePathControl.setText(logFilePath);
			_logFilePath = logFilePath;
			setErrorMessage(null);
		}
	}

	/**
	 * Called when config file is selected.
	 */
	private void selectConfigFile() {
		FileDialog fileDialog = new FileDialog(getShell(), SWT.OPEN);
		fileDialog.setText("Select Config File");
		fileDialog.setFilterPath(System.getProperty("user.home"));

		String configFilePath = fileDialog.open();
		if (configFilePath == null) {
			if (_configFilePath == null) {
				setErrorMessage("Config file is not selected");
			}
			return;
		} else {
			_configFilePathControl.setText(configFilePath);
			_configFilePath = configFilePath;
			setErrorMessage(null);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {

		if (_configFilePath == null) {
			setErrorMessage("Config file not selected");
			return;
		} else if (_logFilePath == null) {
			setErrorMessage("Log file not selected");
			return;
		}

		try {
			if (!isValidFile(_configFilePath,
					LogConstants.CFG_FILE_HEADER_STRING, 256,
					LogConstants.FILE_HEADER_SIZE)) {
				setErrorMessage("Selected file is not an OpenClovis log Config file");
				return;
			}

		} catch (FileNotFoundException e) {
			setErrorMessage("Selected Config File does not exist");
			return;

		} catch (IOException e) {
			setErrorMessage("Unable to read the selected Config File");
			return;
		}

		_recordConfiguration = new RecordConfiguration(_configFilePath);

		try {
			if (!isValidFile(_logFilePath, LogConstants.LOG_FILE_HEADER_STRING,
					0, _recordConfiguration.getRecordLength())) {
				setErrorMessage("Selected file is not an OpenClovis log file");
				return;
			}

		} catch (FileNotFoundException e) {
			setErrorMessage("Selected Log File does not exist");
			return;

		} catch (IOException e) {
			setErrorMessage("Unable to read the selected Log File");
			return;
		}

		super.okPressed();
	}

	/**
	 * Checks whether the file specified by the file path is valid and does
	 * exist or not.
	 * 
	 * @param filePath
	 * @param fileHeader
	 * @param headerLength
	 * @return true if file is valid, false otherwise
	 * @throws FileNotFoundException
	 * @throws IOException
	 */
	private boolean isValidFile(String filePath, String fileHeader,
			int headerStart, int headerLength) throws FileNotFoundException,
			IOException {

		if (headerLength < fileHeader.length()) {
			fileHeader = fileHeader.substring(0, headerLength);
		}

		RandomAccessFile file = new RandomAccessFile(filePath, "r");
		byte bytes[] = new byte[headerLength];
		file.seek(headerStart);
		file.read(bytes);
		file.close();

		if (new String(bytes).startsWith(fileHeader)) {
			return true;
		}
		return false;
	}

	/**
	 * Returns the record configuration.
	 * 
	 * @return the recordConfiguration
	 */
	public RecordConfiguration getRecordConfiguration() {
		return _recordConfiguration;
	}

	/**
	 * Returns the Log File Path.
	 * 
	 * @return the logFilePath
	 */
	public String getLogFilePath() {
		return _logFilePath;
	}
}
