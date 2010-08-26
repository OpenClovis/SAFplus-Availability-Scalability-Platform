package com.clovis.cw.editor.ca.dialog;

import java.util.StringTokenizer;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ICellEditorListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.common.utils.ui.property.PropertyViewer;
import com.clovis.common.utils.ui.table.FormView;


public class EditAttributeAction extends IActionClassAdapter {
	/**
     * Visible only for Attributes.
     * @param environment Environment
     * @return true for method, false otherwise.
     */
    public boolean isVisible(Environment environment) {
		EClass eclass = (EClass) environment.getValue("eclass");
		return eclass.getName().equals("Attribute")
				|| eclass.getName().equals("ProvAttribute");
	}
    /**
     * Enabled only for mib attributes\
     * @param environment Environment
     * @return true for mib attribute, false otherwise.
     */
    public boolean isEnabled(Environment environment) {
		StructuredSelection sel = (StructuredSelection) environment
				.getValue("selection");
		if (sel.getFirstElement() instanceof EObject) {
			EObject obj = (EObject) sel.getFirstElement();
			boolean isImported = Boolean.parseBoolean(String.valueOf(EcoreUtils
					.getValue(obj, "isImported")));
			if (isImported) {
				return true;
			}
		}
		return false;
	}
    /**
	 * Action Class.
	 * 
	 * @param args
	 *            Action args.
	 * @return true if action is successful else false
	 */
    public boolean run(Object[] args)
    {
    	EClass eClass = (EClass) args[2];
        IStructuredSelection selection = (IStructuredSelection) args[1];
        EObject sel  = (EObject) selection.getFirstElement();
        EditMibAttributeDialog dialog = new EditMibAttributeDialog((Shell)args[0], eClass, sel);
        dialog.open();
        return true;
    }
 
}
class EditMibAttributeDialog extends PushButtonDialog {
	private EClass _eClass = null;

	private Object _value = null;

	private Model _viewModel = null;

	private Model _model = null;

	//private DialogValidator _dialogValidator = null;

	/**
	 * @param shell   Shell
	 * @param eClass  Class
	 * @param value   Object
	 *
	 */
	public EditMibAttributeDialog(Shell shell, EClass eClass, Object value) {
		super(shell, eClass, value, null);
		_eClass = eClass;
		_value = value;
		EObject eObject = (EObject) _value;
		_model = new Model(null, eObject);
		_viewModel = _model.getViewModel();
		//_dialogValidator = new DialogValidator(this, _viewModel);
	}

	/**
	 * create the contents of the Dialog.
	 * @param  parent Parent Composite
	 * @return Dialog area.
	 */
	protected Control createDialogArea(Composite parent) {
		Composite container = new Composite(parent, SWT.NONE);
		container.setLayoutData(new GridData(GridData.FILL_BOTH));
		GridLayout containerLayout = new GridLayout();
		container.setLayout(containerLayout);
		ClassLoader loader = getClass().getClassLoader();
		setTitle(EcoreUtils.getLabel(_eClass) + " Details");
		getShell().setText(EcoreUtils.getLabel(_eClass) + " Details");
		EObject viewObject = _viewModel.getEObject();
		EditMibAttributeView formView = new EditMibAttributeView(container, SWT.NONE, viewObject,
		 loader, this);
		formView.setLayoutData(new GridData(GridData.FILL_BOTH));
		formView.setValue("container", this);
		//formView.setValue("dialogvalidator", _dialogValidator);
		formView.setValue("containerModel", _model);
		EAnnotation ann = _eClass.getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("Help") != null) {
				final String contextid = (String) ann.getDetails().get("Help");
				container.addHelpListener(new HelpListener() {

					public void helpRequested(HelpEvent e) {
						PlatformUI.getWorkbench().getHelpSystem().displayHelp(
								contextid);
					}
				});

			}
		}
		return container;
	}

	/**
	 * Save the Model.
	 */
	protected void okPressed() {
		if (_viewModel != null) {
			try {
				_viewModel.save(true);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		super.okPressed();
	}
}

class EditMibAttributeView extends FormView {
	/**
	 * 
	 * @param parent
	 * @param style
	 * @param obj
	 * @param loader
	 * @param container
	 */
	public EditMibAttributeView(Composite parent, int style, EObject obj,
			ClassLoader loader, Object container) {
		super(parent, style);
		_input = obj;
		_classLoader = loader;
		_container = container;
		populateForm();
	}
	/**
	 * @see com.clovis.common.utils.ui.table.FormView#activateCellEditor(org.eclipse.jface.viewers.CellEditor)
	 */
	protected void activateCellEditor(CellEditor editor) {
		final Control control = editor.getControl();
        EStructuralFeature f = (EStructuralFeature)
            control.getData(PropertyViewer.FEATURE_KEY);
        String featureName   = f.getName();
        if(featureName.equals("isImported")) {
        	control.setEnabled(false);
        } else {
        	control.setEnabled(true);
        }
        attachEditorListener(editor);
        editor.setValue(cellModifier.getValue(_input, featureName));
	}
	protected void attachEditorListener(final CellEditor editor)
    {
        editor.addListener(new ICellEditorListener() {
            private boolean _isFocussed = true;
            /**
             * Cancel editting.
             */
            public void cancelEditor()
            {
                //Give focus to parent.
               if (_isFocussed) {
                   _isFocussed = false;
                   editor.getControl().getParent().forceFocus();
                   _isFocussed = true;
               }
            }
            /**
             * Not Used
             * @param o Is Old Valid
             * @param n Is New Valid
             */
            public void editorValueChanged(boolean o, boolean n)
            {
            }
            /**
             * Sets Editor Value, It used RowCellModifier to save
             * the value in EObject.
             */
            public void applyEditorValue()
            {
                EStructuralFeature f = (EStructuralFeature)
                    editor.getControl().getData(PropertyViewer.FEATURE_KEY);
                if (!f.getName().equals("isImported")) {
                	cellModifier.modify(_input, f.getName(), editor.getValue());
                }
            }

        });
    }
}