/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/BootConfigPushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.common.utils.ui.table.TableUI;

public class BootConfigPushButtonCellEditor extends PushButtonCellEditor {
	private EReference _ref = null;

	private Environment _parentEnv = null;
	
	private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
	/**
	 * @param parent - parent composite
	 * @param ref -EReference
	 * @param env - Parent Environment
	 */
	public BootConfigPushButtonCellEditor(Composite parent, EReference ref,
			Environment env) {
		super(parent, ref, env);
		_ref = ref;
		_parentEnv = env;
	}

	/**
	 * @param cellEditorWindow - Control
	 * @return null
	 */
	protected Object openDialogBox(Control cellEditorWindow) {
		 EReference nextRef = (EReference) _ref.getEReferenceType()
				.getEReferences().get(0);
		EObject eobj = (EObject) getValue();
		EList list = (EList) eobj.eGet(nextRef);
		new BootConfigPushButtonDialog(getControl().getShell(), nextRef
				.getEReferenceType(), list, _parentEnv).open();
		return null;
	}

	/**
	 *
	 * @param parent  Composite
	 * @param feature EStructuralFeature
	 * @param env Environment
	 * @return cell Editor
	 */
	public static CellEditor createEditor(Composite parent,
			EStructuralFeature feature, Environment env) {
		return new BootConfigPushButtonCellEditor(parent, (EReference) feature,
				env);
	}
	
	class BootConfigValidator extends DialogValidator 
	{
		protected TitleAreaDialog  _tdialog    = null;

	    protected PreferenceDialog _pdialog    = null;

	    protected DialogPage       _dialogPage = null;

	    protected Button           _okButton   = null;
	    /**
	     *
	     * @param obj -
	     *            Object
	     * @param model Model
	     *
	     */
	    public BootConfigValidator(Object obj, Model model)
	    {
	        this(obj, model, 2);
	    }
	    /**
	    *
	    * @param obj -
	    *            Object
	    * @param model Model
	    * @param depth - Depth at which listener to be attached
	    *
	    */
	   public BootConfigValidator(Object obj, Model model, int depth)
	   {
	       super(obj, model, depth);
	       if (obj instanceof TitleAreaDialog) {
	           _tdialog = (TitleAreaDialog) obj;
	       } else if (obj instanceof PreferenceDialog) {
	           _pdialog = (PreferenceDialog) obj;
	       } else if (obj instanceof DialogPage) {
	           _dialogPage = (DialogPage) obj;
	       }
	   }
	   /**
	    * @param message -
	    *            Message to set
	    *
	    */
	   public void setMessage(String message)
	   {
	       if (message != null) {
	           int type = isValid() ? IMessageProvider.NONE
	                   : IMessageProvider.ERROR;
	           if (_tdialog != null) {
	               _tdialog.setMessage(message, type);
	           } else if (_pdialog != null) {
	               _pdialog.setMessage(message, type);
	           } else if (_dialogPage != null) {
	           	if (_dialogPage instanceof PreferencePage
	           		&& type == IMessageProvider.ERROR) {
	           		((PreferencePage) _dialogPage).setValid(false);
	           	} else if (_dialogPage instanceof PreferencePage
	           		&& type == IMessageProvider.NONE) {
	           		((PreferencePage) _dialogPage).setValid(true);
	           	}
	               _dialogPage.setMessage(message, type);
	           }
	       }
	   }
	    /**
	     * @param valid - sets the model to be valid or not
	     */
	    public void setValid(boolean valid)
	    {
	        if (_okButton != null) {
	            if (valid) {
	                _okButton.setEnabled(true);
	            } else {
	                _okButton.setEnabled(false);
	            }
	        }
	        super.setValid(valid);
	    }
	    /**
	     *
	     * @param okButton OK Button instance
	     */
	    public void setOKButton(Button okButton)
	    {
	        _okButton = okButton;
	    }
	    /**
	     *
	     * @return true if the model is valid else false
	     */
	    public boolean isModelValid()
	    {
	    	for (int i = 0; i < _elist.size(); i++) {
	            EObject eobj = (EObject) _elist.get(i);
	            String message = isValidBootLevel(eobj);
	            if (message != null) {
	                setValid(false);
	                setMessage(message);
	                return false;
	            } else {
	                setValid(true);
	                setMessage("");
	            }
	        }
	        for (int i = 0; i < _elist.size(); i++) {
	            EObject eobj = (EObject) _elist.get(i);
	            String message = isValid(eobj);
	            if (message != null) {
	                setValid(false);
	                setMessage(message);
	                return false;
	            } else {
	                setValid(true);
	                setMessage("");
	            }
	        }
	        return true;
	    }
	    /**
	     * Check the boot level in EObject
	     * @param obj EObject
	     * @return Message
	     */
		public String isValidBootLevel(EObject obj)
		{
			String message = null;
			int maxBootLevel = Integer.parseInt(obj.eGet(
					obj.eClass().getEStructuralFeature("maxBootLevel"))
					.toString());
            int defaultBootLevel = Integer.parseInt(obj.eGet(
                    obj.eClass().getEStructuralFeature("defaultBootLevel"))
                    .toString());
			if (maxBootLevel < 5) {
				setValid(false);
				message = "Maximum bootLevel should be >= 5";
                setMessage(message);
                return message;
			}
            if (defaultBootLevel > maxBootLevel) {
                setValid(false);
                message = "Default boot level should be < maximum boot level";
                setMessage(message);
                return message;
            }
			return message;
		}
		/**
	     * @param notification -
	     *            Notification
	     */
	    public void notifyChanged(Notification notification)
	    {
	        switch (notification.getEventType()) {
	        case Notification.REMOVING_ADAPTER:
	            break;
	        case Notification.SET:
				boolean val = true;
				if (notification.getNotifier() instanceof EObject) {
					EObject ob = (EObject) notification.getNotifier();
					String message = isValidBootLevel(ob);
					if (message != null) {
						val = false;
					} 
				}
				if (val) {
					isModelValid();
				}
				break;
	        case Notification.ADD:
	            Object newVal = notification.getNewValue();
	            boolean isvalid = true;
	            if (newVal instanceof EObject) {
	                EObject obj = (EObject) newVal;
                    String message = isValidBootLevel(obj);
                    if (message != null) {
                        isvalid = false;
                    } 
	                String cwkey = (String) EcoreUtils.getValue(obj,
	                		ModelConstants.RDN_FEATURE_NAME);
	                _cwkeyObjectMap.put(cwkey, obj);
	                EcoreUtils.addListener(obj, this, 1);
	            }
	            if (isvalid) {
	            	isModelValid();
	            }
	            break;

	        case Notification.ADD_MANY:
	            List objs = (List) notification.getNewValue();
	            boolean valid = true;
	            for (int i = 0; i < objs.size(); i++) {
	                if (objs.get(i) instanceof EObject) {
	                    EObject eObj = (EObject) objs.get(i);
                        String message = isValidBootLevel(eObj);
                        if (message != null) {
                            valid = false;
                        } 
	                    String key = (String) EcoreUtils.getValue(eObj,
	                    		ModelConstants.RDN_FEATURE_NAME);
	                    _cwkeyObjectMap.put(key, eObj);
	                    EcoreUtils.addListener(eObj, this, 1);
	                }
	            }
	            if (valid) {
	            	isModelValid();
	            }
	            break;
	        case Notification.REMOVE:
	            Object obj = notification.getOldValue();
	            if (obj instanceof EObject) {
	                EObject ob = (EObject) obj;
	                String rkey = (String) EcoreUtils.getValue(ob,
	                		ModelConstants.RDN_FEATURE_NAME);
	                _cwkeyObjectMap.remove(rkey);
	                EcoreUtils.removeListener(ob, this, 1);
	            }
	            isModelValid();
	            break;
	        case Notification.REMOVE_MANY:
	            objs = (List) notification.getOldValue();
	            for (int i = 0; i < objs.size(); i++) {
	                if (objs.get(i) instanceof EObject) {
	                    EObject o = (EObject) objs.get(i);
	                    String rk = (String) EcoreUtils.getValue(o,
	                    		ModelConstants.RDN_FEATURE_NAME);
	                    _cwkeyObjectMap.remove(rk);
	                    EcoreUtils.removeListener(o, this, 1);
	                }
	            }
	            isModelValid();
	            break;
	        }
	    }
	}
	class BootConfigPushButtonDialog extends TitleAreaDialog {

		private Object      _value       = null;
	    private EClass      _eClass      = null;
	    private Model       _viewModel   = null;
	    private Environment _parentEnv   = null;
	    private Reader      _menuReader  = null;
	    private BootConfigValidator _dialogValidator = null;
	    
	    /**
	     * @param shell   Shell
	     * @param eClass  Class
	     * @param value   Object
	     *
	     */
	    public BootConfigPushButtonDialog(Shell shell, EClass eClass, Object value)
	    {
	        this(shell, eClass, value, null);
	    }
	    /**
	     * @param shell   Shell
	     * @param eClass  Class
	     * @param value   Object
	     * @param parentEnv ParentEnvironment
	     */
	    public BootConfigPushButtonDialog(Shell shell, EClass eClass, Object value,
	            Environment parentEnv)
	    {
	        super(shell);
	        _value     = value;
	        _eClass    = eClass;
	        _parentEnv = parentEnv;
	        if (value instanceof List) {
	            NotifyingList origList = (NotifyingList) _value;
	            Model model = new Model(null, origList, _eClass.getEPackage());
	            _viewModel  = model.getViewModel();
	            _dialogValidator = new BootConfigValidator(this, _viewModel);
	        }
	    }
	    /**
	     * Save the Model.
	     */
	    protected void okPressed()
	    {
	        if (_viewModel != null) {
	            try {
	                _viewModel.save(false);
	            } catch (Exception e) {
	                LOG.error("Save Error.", e);
	            }
	        }
	        super.okPressed();
	    }
	    /**
	     * Closing Dialog.
	     * @return super.close()
	     */
	    public boolean close()
	    {
	        if (_viewModel != null) {
	            if (_dialogValidator != null) {
	            _dialogValidator.removeListeners();
	            _dialogValidator = null;
	            }
	            _viewModel.dispose();
	            _viewModel = null;
	        }
	        return super.close();
	    }
	    /**
	     * Set Specific Menus.
	     * @param reader Reader for menu information.
	     */
	    public void setMenuFileReader(Reader reader)
	    {
	        _menuReader = reader;
	    }
	    /**
	     * Get FileReader to load menus. Subclasses can override
	     * to install specific menus.
	     * @return Reader for menu
	     * @throws IOException In case of IO Errors
	     */
	    protected Reader getMenuFileReader()
	        throws IOException
	    {
	        if (_menuReader != null) {
	            return _menuReader;
	        }

	        String custom =
	            EcoreUtils.getAnnotationVal(_eClass, null, "menuxml");
	        if (custom != null) {
	            int index = custom.indexOf(':');
	            String plugin     = null;
	            String xmlName  = custom;
	            if (index != -1) {
	                plugin    = custom.substring(0, index);
	                xmlName = custom.substring(index + 1);
	            }
	            try {
	                if (plugin != null) {
	                    _menuReader = new InputStreamReader(Platform.
	                      getBundle(plugin).getResource(xmlName).openStream());
	                }
	            } catch (Exception e) {
	                LOG.error("plugin can not be loaded:", e);
	            }
	        }
	        if (_menuReader == null) {
	            _menuReader = new InputStreamReader(
	                getClass().getResourceAsStream("tabletoolbar.xml"));
	        }
	        return _menuReader;
	    }
	    /**
	     * @param parent Composite
	     * @return Control
	     */
	    protected Control createContents(Composite parent)
	    {
	        Control control = super.createContents(parent);
	        if (_dialogValidator != null) {
	        _dialogValidator.setOKButton(getButton(IDialogConstants.OK_ID));
	        }
	        return control;
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
	        container.setLayout(containerLayout);
	        ClassLoader loader = getClass().getClassLoader();
	        setTitle(EcoreUtils.getLabel(_eClass) + " Details");
	        getShell().setText(EcoreUtils.getLabel(_eClass) + " Details");
	        if (_value instanceof EObject) {
	            EObject eObject = (EObject) _value;
	            Model model = new Model(null, eObject);
	            _viewModel  = model.getViewModel();
	            EObject viewObject = _viewModel.getEObject();
	            FormView formView =
	                new FormView(container, SWT.NONE, viewObject, loader, this);
	            formView.setLayoutData(new GridData(GridData.FILL_BOTH));
	        } else if (_value instanceof NotifyingList) {
	            containerLayout.numColumns = 2;
	            int style = SWT.SINGLE | SWT.BORDER | SWT.H_SCROLL
	                | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
	            Table table = new Table(container, style);
	            TableUI tableViewer = new
	                TableUI(table, _eClass, loader, false, _parentEnv);
	            tableViewer.setValue("container", this);
	            tableViewer.setValue("dialogvalidator", _dialogValidator);
	            GridData gridData1 = new GridData();
	            gridData1.horizontalAlignment = GridData.FILL;
	            gridData1.grabExcessHorizontalSpace = true;
	            gridData1.grabExcessVerticalSpace = true;
	            gridData1.verticalAlignment = GridData.FILL;
	            gridData1.heightHint = container.getDisplay().getClientArea().height / 10;
	            table.setLayoutData(gridData1);
	            
	            table.setLinesVisible(true);
	            table.setHeaderVisible(true);
	            tableViewer.setInput(_viewModel.getEList());
	            table.setSelection(0);

	            //Add Buttons (toolbar) for table.
	            try {
	                Reader reader = getMenuFileReader();
	                new MenuBuilder(reader, tableViewer).getToolbar(container, 0);
	            } catch (Exception e) {
	                LOG.error("Toolbar could not be loaded, Using No Toolbar.", e);
	            }
	        }
	        EAnnotation ann = _eClass.getEAnnotation("CWAnnotation");
			if (ann != null) {
				if (ann.getDetails().get("Help") != null ) 
				{	
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
	}
}
