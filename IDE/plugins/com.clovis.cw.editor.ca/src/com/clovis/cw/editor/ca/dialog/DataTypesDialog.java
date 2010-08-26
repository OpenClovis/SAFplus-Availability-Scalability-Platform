/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/DataTypesDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;

/**
 * @author shubhada
 *
 * Dialog for User Defined Data Types
 */
public class DataTypesDialog extends PreferenceDialog
{
    private PreferenceManager _preferenceManager;
    private String  _dialogDescription = "IDL Specs";
    private Model _viewModel = null;
    private EObject _selObj = null;
    private boolean _isGlobalDataType;
    private String 			  _contextHelpId = null;
    /**
     * @param parentShell -Parent Shell
     * @param manager - Preference Manager
     */
    public DataTypesDialog(Shell parentShell, PreferenceManager manager,
            boolean isGlobalDataType, EObject selObj)
    {
        super(parentShell, manager);
        _preferenceManager = manager;
        _selObj = selObj;
        _isGlobalDataType = isGlobalDataType;
        _contextHelpId     = EcoreUtils.getAnnotationVal(RMDDialog.getInstance().getIDLSpecsClass(), null, "Help");
        addPreferenceNodes();
    }
    /**
     * @see org.eclipse.jface.preference.PreferenceDialog#createContents(org.eclipse.swt.widgets.Composite)
     */
    protected Control createContents(Composite parent) {
		Control control = super.createContents(parent);
		if (_contextHelpId != null) {
			control.addHelpListener(new HelpListener() {
				public void helpRequested(HelpEvent e) {
					PlatformUI.getWorkbench().getHelpSystem().displayHelp(
							_contextHelpId);
				}
			});
		}
		return control;
    }
    /**
     * Adds Preference Nodes to the Dialog
     */
    private void addPreferenceNodes()
    {
        EReference structRef = null;
        EReference unionRef = null;
        EReference enumRef = null;
        NotifyingList structList = null;
        NotifyingList unionList = null;
        NotifyingList enumList = null;
        if (_isGlobalDataType) {
            EClass idlClass = RMDDialog.getInstance().getIDLSpecsClass();
            EObject idlObj = RMDDialog.getInstance().getIDLSpecsObject();
            Model model = new Model(null, idlObj);
            _viewModel = model.getViewModel();
            structRef = (EReference) idlClass.
                getEStructuralFeature("Struct");
            structList = (NotifyingList) _viewModel.getEObject().
            	eGet(structRef);
            
            unionRef = (EReference) idlClass.
                getEStructuralFeature("Union");
            unionList = (NotifyingList) _viewModel.getEObject().
            	eGet(unionRef);
            
            enumRef = (EReference) idlClass.
                getEStructuralFeature("Enum");
            enumList = (NotifyingList) _viewModel.getEObject().
            	eGet(enumRef);
            
        } else {
            // In this case user defines the data structures local to his EO
            Model model = new Model(null, _selObj);
            _viewModel = model.getViewModel();
            structRef = (EReference) _selObj.eClass().
                getEStructuralFeature("Struct");
            structList = (NotifyingList) _viewModel.getEObject().
                eGet(structRef);
            
            unionRef = (EReference) _selObj.eClass().
                getEStructuralFeature("Union");
            unionList = (NotifyingList) _viewModel.getEObject().
                eGet(unionRef);
            
            enumRef = (EReference) _selObj.eClass().
                getEStructuralFeature("Enum");
            enumList = (NotifyingList) _viewModel.getEObject().
                eGet(enumRef);
        }
        _preferenceManager.addToRoot(new PreferenceNode("Struct",
                new GenericTablePage("Define Struct", structRef.
                        getEReferenceType(), structList)));
        
        _preferenceManager.addToRoot(new PreferenceNode("Union",
                new GenericTablePage("Define Union", unionRef.
                        getEReferenceType(), unionList)));
        
        _preferenceManager.addToRoot(new PreferenceNode("Enum",
                new GenericTablePage("Define Enum", enumRef.
                        getEReferenceType(), enumList)));
    }
    /**
     * @param shell - Shell
     */
    protected void configureShell(Shell shell)
    {
        super.configureShell(shell);
        shell.setText(_dialogDescription);
        //this.setMinimumPageSize(400, 185);
    }
    /**
     * Close the dialog.
     * @return super.close()
     */
    public boolean close()
    {
        if (_viewModel != null) {
            _viewModel.dispose();
            _viewModel = null;
        }
        return super.close();
    }
    /**
     * OK pressed, so save the original model 
     */
    protected void okPressed()
    {
        _viewModel.save(false);
        super.okPressed();
    }
    /***** This code needs to be removed in future ******/
    protected Control createHelpControl(Composite parent) {
		Image helpImage = JFaceResources.getImage(DLG_IMG_HELP);
		if (helpImage != null) {
			return createHelpImageButton(parent, helpImage);
		}
		return createHelpLink(parent);
    }
	private ToolBar createHelpImageButton(Composite parent, Image image) {
        ToolBar toolBar = new ToolBar(parent, SWT.FLAT | SWT.NO_FOCUS);
        ((GridLayout) parent.getLayout()).numColumns++;
		toolBar.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		final Cursor cursor = new Cursor(parent.getDisplay(), SWT.CURSOR_HAND);
		toolBar.setCursor(cursor);
		toolBar.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				cursor.dispose();
			}
		});		
        ToolItem item = new ToolItem(toolBar, SWT.NONE);
		item.setImage(image);
		item.setToolTipText(JFaceResources.getString("helpToolTip")); //$NON-NLS-1$
		item.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
            }
        });
		return toolBar;
	}
	private Link createHelpLink(Composite parent) {
		Link link = new Link(parent, SWT.WRAP | SWT.NO_FOCUS);
        ((GridLayout) parent.getLayout()).numColumns++;
		link.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		link.setText("<a>"+IDialogConstants.HELP_LABEL+"</a>"); //$NON-NLS-1$ //$NON-NLS-2$
		link.setToolTipText(IDialogConstants.HELP_LABEL);
		link.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
            }
        });
		return link;
	}
	/***** This code needs to be removed in future ******/
}
