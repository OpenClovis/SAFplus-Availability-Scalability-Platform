package com.clovis.cw.editor.ca.dialog;

import java.util.Arrays;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.List;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;

public class LibsPushButtonDialog extends PushButtonDialog {
	
	List _libsList, _pathsList;
	public LibsPushButtonDialog(Shell shell, EClass eClass,
			Object value, Environment parentEnv) {
		super(shell, eClass, value, parentEnv);
		super.setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
		//EObject obj = (EObject) value;
        //_viewModel  = new Model(null, obj).getViewModel();
	}
	/**
     * create the contents of the Dialog.
     * @param  parent Parent Composite
     * @return Dialog area.
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 1;
        //containerLayout.marginHeight = 40;
        container.setLayout(containerLayout);
        setTitle("Please enter libs and lib paths");
        getShell().setText("Libraries Details");
        
        java.util.List<String> libsList = (java.util.List) EcoreUtils.getValue(_viewModel.getEObject(), "libName");
        java.util.List<String> pathsList = (java.util.List) EcoreUtils.getValue(_viewModel.getEObject(), "libPath");
        String[] libNames = libsList.toArray(new String[libsList.size()]);
        String[] pathNames = pathsList.toArray(new String[pathsList.size()]);;
        Group group1 = new Group(container, SWT.BORDER);
        group1.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        GridLayout groupLayout = new GridLayout();
        groupLayout.numColumns = 2;
        group1.setLayout(groupLayout);
        Label label1 = new Label(group1, SWT.NONE);
        label1.setText("Libraries (-l):");
        createButtonContainer(group1, "LIB");
        _libsList = new List(group1, SWT.BORDER);
        GridData data1 = new GridData(GridData.FILL_HORIZONTAL);
        data1.horizontalSpan = 2;
        data1.heightHint = 100;
        data1.grabExcessHorizontalSpace = true;
        _libsList.setLayoutData(data1);
        _libsList.setItems(libNames);
        
        Group group2 = new Group(container, SWT.BORDER);
        group2.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        group2.setLayout(groupLayout);
        Label label2 = new Label(group2, SWT.NONE);
        label2.setText("Library search paths (-L):");
        createButtonContainer(group2, "PATH");
        _pathsList = new List(group2, SWT.BORDER);
        _pathsList.setLayoutData(data1);
        _pathsList.setItems(pathNames);
        return container;
    }
    /**
     * 
     * @param container
     * @return
     */
    private Composite createButtonContainer(Group container, String type){
    	Composite composite = new Composite(container, SWT.NONE);
    	composite.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_END));
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        composite.setLayout(containerLayout);
        Button addBtn = new Button(composite, SWT.BOTTOM);
        addBtn.setText("Add..");
        addBtn.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_END));
        addBtn.addSelectionListener(new AddSelectionAdapter(type));
        Button delBtn = new Button(composite, SWT.BOTTOM);
        delBtn.setText("Delete");
        delBtn.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_END));
        delBtn.addSelectionListener(new DeleteSelectionAdapter(type));
    	return composite;
    }
    class AddSelectionAdapter extends SelectionAdapter {
    	String type;
    	public AddSelectionAdapter(String type) {
    		this.type = type;
    	}
    	public void widgetSelected(SelectionEvent e) {
			if(type.equals("LIB")) {
				class LibDialog extends Dialog {
					Text text;
					String libName;
					public LibDialog(Shell shell) {
						super(shell);
					}
					protected Control createContents(Composite parent) {
						Composite composite = new Composite(parent, SWT.NONE);
						GridLayout layout = new GridLayout();
						layout.marginTop = 25;
						composite.setLayout(layout);
						composite.setLayoutData(new GridData(GridData.FILL_BOTH));
						Label label = new Label(composite, SWT.NONE);
						label.setText("Enter library Name:");
						text = new Text(composite, SWT.BORDER);
						GridData data = new GridData(GridData.FILL_HORIZONTAL);
						text.setLayoutData(data);
						return super.createContents(parent);
					}
					protected void okPressed() {
						libName = text.getText().trim();
						if(!libName.equals(""))
							_libsList.add(libName);
						super.okPressed();
					}
					
				}
				LibDialog dialog = new LibDialog(getShell());
				dialog.open();
			} else if(type.equals("PATH")) {
				class LibPathDialog extends Dialog {
					Text text;
					String libPath;
					public LibPathDialog(Shell shell) {
						super(shell);
					}
					protected Control createContents(Composite parent) {
						Composite composite = new Composite(parent, SWT.NONE);
						GridLayout layout = new GridLayout();
						layout.numColumns = 2;
						layout.marginTop = 25;
						composite.setLayout(layout);
						composite.setLayoutData(new GridData(GridData.FILL_BOTH));
						Label label = new Label(composite, SWT.NONE);
						label.setText("Enter library Path:");
						GridData data = new GridData(GridData.FILL_HORIZONTAL);
						data.horizontalSpan = 2;
						label.setLayoutData(data);
						text = new Text(composite, SWT.BORDER);
						data = new GridData(GridData.FILL_HORIZONTAL);
						text.setLayoutData(data);
						Button browseBtn = new Button(composite, SWT.BORDER);
						browseBtn.setText("File system...");
						browseBtn.addSelectionListener(new SelectionAdapter() {
	                        public void widgetSelected(SelectionEvent e)
	                        {
	                            DirectoryDialog dialog =
	                                new DirectoryDialog(getShell(), SWT.NONE);
	                            String dirName = dialog.open();
	                            if (dirName != null && !dirName.equals("")) {
	                                text.setText(dirName);
	                            }
	                        }
						});
						return super.createContents(parent);
					}
					protected void okPressed() {
						libPath = text.getText().trim();
						if(!libPath.equals(""))
							_pathsList.add(libPath);
						super.okPressed();
					}
					
				}
				LibPathDialog dialog = new LibPathDialog(getShell());
				dialog.open();
			}
		}
    }
    class DeleteSelectionAdapter extends SelectionAdapter {
    	String type;
    	public DeleteSelectionAdapter(String type) {
    		this.type = type;
    	}
    	public void widgetSelected(SelectionEvent e) {
    		if(type.equals("LIB")) {
    			_libsList.remove(_libsList.getSelectionIndices());
    		} else if(type.equals("PATH")) {
    			_pathsList.remove(_pathsList.getSelectionIndices());
    		}
		}
    }
    /**
     * Save the Model.
     */
    protected void okPressed()
    {
    	java.util.List<String> libsList = (java.util.List) EcoreUtils.getValue(_viewModel.getEObject(), "libName");
		libsList.clear();
		java.util.List<String> pathsList = (java.util.List) EcoreUtils.getValue(_viewModel.getEObject(), "libPath");
		pathsList.clear();
		libsList.addAll(Arrays.asList(_libsList.getItems()));
		pathsList.addAll(Arrays.asList(_pathsList.getItems()));
		_viewModel.save(false);
		setReturnCode(OK);
		close();
    }
	/**
     * Closing Dialog.
     * @return super.close()
     */
    public boolean close()
    {
        return super.close();
    }
}