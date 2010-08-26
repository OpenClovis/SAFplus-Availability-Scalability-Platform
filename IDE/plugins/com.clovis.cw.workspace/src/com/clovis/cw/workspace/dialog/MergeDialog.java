/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/MergeDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/28 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.dialog;

import java.util.List;
import java.util.Map;

import org.eclipse.core.resources.IFolder;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.cw.workspace.project.CwProjectPropertyPage;
import com.clovis.cw.workspace.utils.MergeUtils;

/**
 * @author pushparaj
 * Dialog for Merge
 */
public class MergeDialog extends TitleAreaDialog
{
	private IFolder _nextCodeGenFolder, _srcFolder/* , _lastCodeGenFolder */;
	private org.eclipse.swt.widgets.List _filesList;
	private List _changesList;
	private Map _fileNamesMap;
	private Button _mergeMode, _overrideMode, cancel;
	private String NORMAL_MESSAGE = "Select files for merge/override";
	private String CANCEL_MESSAGE = "You can use this Dialog to merge/override files. If you don't want to change anything in the source directory, Please click on 'Cancel' before doing merge/override.";
	//private String _mergeScriptFile;
	/**
	 * Constructor
	 * @param parentShell Shell
	 * @param changeList List of user modifiable files
	 * @param namesMap map for old and new file names
	 * @param next next code gen Folder
	 * @param src model location
	 * @param last last code gen Folder
	 */
	/*public MergeDialog(Shell parentShell, List changeList, Map namesMap,
			IFolder next, IFolder src, IFolder last, String scriptFile) {
		super(parentShell);
		setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
		_changesList = changeList;
		_fileNamesMap = namesMap;
		_nextCodeGenFolder = next;
		_srcFolder = src;
		_lastCodeGenFolder = last;
		_mergeScriptFile = scriptFile;
	}*/
	public MergeDialog(Shell parentShell, List changeList, Map namesMap,
			IFolder next, IFolder src) {
		super(parentShell);
		setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
		_changesList = changeList;
		_fileNamesMap = namesMap;
		_nextCodeGenFolder = next;
		_srcFolder = src;
		//_lastCodeGenFolder = last;
	}
	/**
	 * Adds file names to list
	 *
	 */
	private void addFiles() {
		for (int i = 0; i < _changesList.size(); i++)
			_filesList.add((String) _changesList.get(i));
	}
	/**
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		composite.setLayoutData(new GridData(GridData.FILL_BOTH));
		composite.setLayout(new GridLayout(4, false));
		getShell().setText("Merge");
		
		_filesList = new org.eclipse.swt.widgets.List(
				composite, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
		GridData gridData = new GridData(GridData.FILL_BOTH);
		gridData.widthHint = 300;
		gridData.heightHint = 400;
		gridData.horizontalSpan = 3;
		_filesList.setLayoutData(gridData);
		
		createButton(composite);
		
		_overrideMode = new Button(composite, SWT.RADIO);
		_overrideMode.setText("Always override application code");
		gridData = new GridData(GridData.FILL_BOTH);
		gridData.horizontalSpan = 4;
		_overrideMode.setLayoutData(gridData);
		
		_mergeMode = new Button(composite, SWT.RADIO);
		_mergeMode.setText("Always merge application code");
		gridData = new GridData(GridData.FILL_BOTH);
		gridData.horizontalSpan = 4;
		_mergeMode.setLayoutData(gridData);
		
		setTitle("Modified Files");
		setMessage(CANCEL_MESSAGE, IMessageProvider.INFORMATION);
		addFiles();
		return composite;
	}
	
	/**
	 * Add "Merge/Override" button
	 * @param parent the parent composite
	 */
	private void createButton(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		composite.setLayout(layout);
		GridData gridData = new GridData(GridData.GRAB_HORIZONTAL | GridData.GRAB_VERTICAL);
		gridData.horizontalSpan = 1;
		Button merge = new Button(composite, SWT.PUSH | SWT.CENTER);
		merge.setText("Merge");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 85;
		merge.setLayoutData(gridData);
		merge.addSelectionListener(new SelectionAdapter() {
     		public void widgetSelected(SelectionEvent e) {
				String files[] = _filesList.getSelection();
				MergeUtils.mergeFiles(files, _fileNamesMap, _srcFolder, _nextCodeGenFolder);
				_filesList.remove(_filesList.getSelectionIndices());
				if (_filesList.getItemCount() == 0) {
					windowClose();
				} else {
					setMessage(NORMAL_MESSAGE, IMessageProvider.INFORMATION);
					cancel.setEnabled(false);
				}
			}
		});
		Button mergeAll = new Button(composite, SWT.PUSH | SWT.CENTER);
		mergeAll.setText("Merge All");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 85;
		mergeAll.setLayoutData(gridData);
		mergeAll.addSelectionListener(new SelectionAdapter() {
       		public void widgetSelected(SelectionEvent e) {
				String files[] = _filesList.getItems();
				MergeUtils.mergeFiles(files, _fileNamesMap, _srcFolder, _nextCodeGenFolder);
				_filesList.removeAll();
				windowClose();
			}
		});
		Button override = new Button(composite, SWT.PUSH | SWT.CENTER);
		override.setText("Override");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 85;
		override.setLayoutData(gridData);
		override.addSelectionListener(new SelectionAdapter() {
      		public void widgetSelected(SelectionEvent e) {
      			String files[] = _filesList.getSelection();
      			MergeUtils.overrideFiles(files, _nextCodeGenFolder, _srcFolder);
      			_filesList.remove(_filesList.getSelectionIndices());
      			if(_filesList.getItemCount() == 0) {
					windowClose();
				} else {
					setMessage(NORMAL_MESSAGE, IMessageProvider.INFORMATION);
					cancel.setEnabled(false);
				}
			}
		});
		Button overrideall = new Button(composite, SWT.PUSH | SWT.CENTER);
		overrideall.setText("Override All");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 85;
		overrideall.setLayoutData(gridData);
		overrideall.addSelectionListener(new SelectionAdapter() {
      	
      		public void widgetSelected(SelectionEvent e) {
      			String files[] = _filesList.getItems();
      			MergeUtils.overrideFiles(files, _nextCodeGenFolder, _srcFolder);
      			_filesList.removeAll();
      			windowClose();
      		}
		});
		cancel = new Button(composite, SWT.PUSH | SWT.CENTER);
		cancel.setText("Cancel");
		gridData = new GridData (GridData.HORIZONTAL_ALIGN_BEGINNING);
		gridData.widthHint = 85;
		cancel.setLayoutData(gridData);
		cancel.addSelectionListener(new SelectionAdapter() {
      	
      		public void widgetSelected(SelectionEvent e) {
      			cancelPressed();
      		}
		});
	}
	/**
	 * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	protected void createButtonsForButtonBar(Composite parent) {
		// Not using OK, Cancel buttons
	}
	/**
	 * Notifies that the cancel button of this dialog has been pressed.
	 */
	protected void cancelPressed() {
		setReturnCode(CANCEL);
		super.close();
	}
	/**
	 * Closes Dialog
	 */
	private void windowClose() {
		CwProjectPropertyPage.setAlwaysOverrideMode(_srcFolder.getProject(),
				_overrideMode.getSelection());
		CwProjectPropertyPage.setAlwaysMergeMode(_srcFolder.getProject(),
				_mergeMode.getSelection());
		super.close();
	}
	/**
	 * @see org.eclipse.jface.window.Window#close()
	 */
	public boolean close() {
		if(MessageDialog.openQuestion(getShell(), "Confirm", "This will override all listed files. Do you want to continue?")) {
			String files[] = _filesList.getItems();
  			MergeUtils.overrideFiles(files, _nextCodeGenFolder, _srcFolder);
  			_filesList.removeAll();
  			CwProjectPropertyPage.setAlwaysOverrideMode(_srcFolder.getProject(),
  					_overrideMode.getSelection());
  			CwProjectPropertyPage.setAlwaysMergeMode(_srcFolder.getProject(),
  					_mergeMode.getSelection());
			super.close();
		}
		return false;
	}
}

