/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/dialog/PushButtonDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.dialog;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.common.utils.ui.table.TableUI;
/**
 * @author shubhada
 *
 * Dialog to display parameters.
 */
public class PushButtonDialog extends TitleAreaDialog
{
    protected Object      _value       = null;
    protected EClass      _eClass      = null;
    protected Model       _viewModel   = null;
    protected Model       _model       = null;
    protected Environment _parentEnv   = null;
    private Reader      _menuReader  = null;
    protected DialogValidator _dialogValidator = null;
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    
    /**
     * @param shell   Shell
     *
     */
    public PushButtonDialog(Shell shell)
    {
        super(shell);
    }
    /**
     * @param shell   Shell
     * @param eClass  Class
     * @param value   Object
     *
     */
    public PushButtonDialog(Shell shell, EClass eClass, Object value)
    {
        this(shell, eClass, value, null);
    }
    /**
     * @param shell   Shell
     * @param eClass  Class
     * @param value   Object
     * @param parentEnv ParentEnvironment
     */
    public PushButtonDialog(Shell shell, EClass eClass, Object value,
            Environment parentEnv)
    {
        super(shell);
        super.setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
        _value     = value;
        _eClass    = eClass;
        _parentEnv = parentEnv;
        if (value instanceof List) {
            NotifyingList origList = (NotifyingList) _value;
            _model = new Model(null, origList, _eClass.getEPackage());
            _viewModel  = _model.getViewModel();
        } else if (value instanceof EObject) {
            EObject eObject = (EObject) _value;
            _model = new Model(null, eObject);
            _viewModel  = _model.getViewModel();
        }
        _dialogValidator = new DialogValidator(this, _viewModel);
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
            if (custom.equals("no")) {
                return null;
            }
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
                LOG.error("Plugin can not be loaded:", e);
            }
        }
        if (_menuReader == null) {
            _menuReader = new InputStreamReader(
                getClass().getResourceAsStream("tabletoolbar.xml"));
        }
        return _menuReader;
    }
    /**
     * 
     * @return view model
     */
    public Model getViewModel()
    {
        return _viewModel;
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
            EObject viewObject = _viewModel.getEObject();
            FormView formView =
                new FormView(container, SWT.NONE, viewObject, loader, this);
            formView.setLayoutData(new GridData(GridData.FILL_BOTH));
            formView.setValue("container", this);
            formView.setValue("dialogvalidator", _dialogValidator);
            formView.setValue("containerModel", _model);
        } else if (_value instanceof NotifyingList) {
            containerLayout.numColumns = 2;
            int style = SWT.SINGLE | SWT.BORDER | SWT.H_SCROLL
                | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
            Table table = new Table(container, style);
            TableUI tableViewer = new
                TableUI(table, _eClass, loader, false, _parentEnv);
            tableViewer.setValue("container", this);
            tableViewer.setValue("dialogvalidator", _dialogValidator);
            tableViewer.setValue("containerModel", _model);
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
                if (reader == null) {
                    return container;
                }
                new MenuBuilder(reader, tableViewer).getToolbar(container, 0);
            } catch (Exception e) {
                LOG.error("Toolbar could not be loaded, using no toolbar.", e);
            }
        }
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
}
