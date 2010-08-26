/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/OperationPropertiesDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.io.InputStreamReader;

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.common.notify.NotifyingList;

import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.factory.WidgetProviderFactory;
import com.clovis.common.utils.ui.table.TableUI;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
/**
 * @author shubhada
 * This dialog shows property of operation (of Class).
 */
public class OperationPropertiesDialog extends TitleAreaDialog
    implements ClassEditorConstants
{

    private EObject               _eObject;
    private WidgetProviderFactory _wfactory;
    private DialogValidator _dialogValidator = null;
    private DialogValidator _paramValidator = null;
    /**
     * Constructor.
     * @param parentShell Parent Shell.
     * @param operObj     EObject for Operation.
     */
    public OperationPropertiesDialog(Shell parentShell, EObject operObj)
    {
        super(parentShell);
        _eObject = operObj;
        _wfactory = new WidgetProviderFactory(_eObject);
        Model model = new Model(null, operObj);
        _dialogValidator = new DialogValidator(this, model);
        _dialogValidator.setOKButton(getButton(IDialogConstants.OK_ID));
    }
    /**
     * @param  parent Parent Composite.
     * @return Control added to Dialog.
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite contents = new Composite(parent, org.eclipse.swt.SWT.NONE);
        GridLayout glayout = new GridLayout();
        //contents.setSize(100, 200);
        glayout.numColumns = 4;
        contents.setLayout(glayout);
        contents.setLayoutData(new GridData(GridData.FILL_BOTH));

        setMessage(METHOD_PROPERTIES_MESSAGE, MessageDialog.INFORMATION);
        Label nameLabel = new Label(contents, org.eclipse.swt.SWT.NONE);
        nameLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        nameLabel.setText(METHOD_NAME_LABEL);

        EClass operClass = _eObject.eClass();
        EStructuralFeature feature =
            operClass.getEStructuralFeature(METHOD_NAME);
        Text nameText = _wfactory.getTextBox(contents,
            SWT.SINGLE | SWT.LEFT | SWT.BORDER, feature);
        GridData gd = new GridData();
        gd.horizontalAlignment = GridData.FILL;
        gd.horizontalSpan = 3;
        nameText.setLayoutData(gd);


        Label typeLabel = new Label(contents, org.eclipse.swt.SWT.NONE);
        typeLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        typeLabel.setText(METHOD_TYPE_LABEL);

        feature = operClass.getEStructuralFeature(METHOD_RETURN_TYPE);
        CCombo typeCCombo = _wfactory.getComboBox(contents,
                SWT.BORDER | SWT.READ_ONLY, feature);
        typeCCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        gd = new GridData();
        gd.horizontalAlignment = GridData.FILL;
        gd.horizontalSpan = 3;
        typeCCombo.setLayoutData(gd);

        Label visibilityLabel = new Label(contents, SWT.NONE);
        visibilityLabel.setLayoutData(
                new GridData(GridData.BEGINNING | GridData.FILL));
        visibilityLabel.setText(VISIBILITY_LABLE_NAME);

        feature = operClass.getEStructuralFeature(METHOD_MODIFIER);
        Button [] buttons =
            _wfactory.getRadioButtons(contents, SWT.NONE, feature);
        buttons[0].setLayoutData(new GridData());
        GridData lastButtonData = new GridData(GridData.FILL_HORIZONTAL);
        lastButtonData.horizontalSpan = 2;
        buttons[1].setLayoutData(lastButtonData);

        EClass eClass = _eObject.eClass();
        Label modifierLabel = new Label(contents, SWT.NONE);
        modifierLabel.setLayoutData(
            new GridData(GridData.BEGINNING | GridData.FILL));
        modifierLabel.setText(MODIFIER_LABLE_NAME);

        feature = eClass.getEStructuralFeature(METHOD_IS_STATIC);
        Button staticButton = _wfactory.getButton(contents, SWT.CHECK, feature);
        staticButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

        feature = eClass.getEStructuralFeature(METHOD_IS_CONST);
        Button constButton = _wfactory.getButton(contents, SWT.CHECK, feature);
        constButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

        feature = eClass.getEStructuralFeature(METHOD_IS_ABSTRACT);
        Button absButton = _wfactory.getButton(contents, SWT.CHECK, feature);
        absButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

        createParametersGroup(contents);
        Label docLabel = new Label(contents, SWT.NONE);
        docLabel.setText(METHOD_DOCUMENTATION);
        GridData docgd = new GridData(GridData.BEGINNING);
        docgd.horizontalSpan = 4;
        docLabel.setLayoutData(docgd);

        feature = eClass.getEStructuralFeature(METHOD_DOCUMENTATION);
        Text doc = _wfactory.getTextBox(contents,
                SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL | SWT.BORDER, feature);
        GridData gdDoc = new GridData(GridData.FILL_BOTH);
        gdDoc.horizontalSpan = 4;
        gdDoc.verticalSpan = 30;
        doc.setLayoutData(gdDoc);
        contents.getShell().setText("Properties");
        this.setTitle("Operation Properties");
        EAnnotation ann = _eObject.eClass().getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("Help") != null ) 
			{	
				final String contextid = (String) ann.getDetails().get("Help");
				contents.addHelpListener(new HelpListener() {
					public void helpRequested(HelpEvent e) {
						PlatformUI.getWorkbench().getHelpSystem().displayHelp(
								contextid);
					}
				});       
			}
		}
        return contents;
    }
    /**
     * Create Table to show list of parameters.
     * @param parent Parent Composite.
     */
    private void createParametersGroup(Composite parent)
    {
        Group parametersGroup = new Group(parent, SWT.SHADOW_OUT);
        parametersGroup.setText(PARAMETERS_GROUP_NAME);
        GridData gd = new GridData();
        gd.horizontalSpan = 4;
        gd.horizontalAlignment = GridData.FILL;
        parametersGroup.setLayoutData(gd);
        parametersGroup.setLayout(new GridLayout(2, false));

        Table parametersTable =
            new Table(parametersGroup, SWT.BORDER | SWT.FULL_SELECTION);
        parametersTable.setLayoutData(new GridData(GridData.FILL_BOTH));
        parametersTable.setHeaderVisible(true);
        parametersTable.setLinesVisible(true);
        EClass operClass    = _eObject.eClass();
        EReference paramRef = (EReference) operClass.getEReferences().get(0);
        NotifyingList eObjects = (NotifyingList) _eObject.eGet(paramRef);
        Model model = new Model(null, eObjects, null);
        _paramValidator = new DialogValidator(this, model);
        _paramValidator.setOKButton(getButton(IDialogConstants.OK_ID));
        ClassLoader loader  = getClass().getClassLoader();
        TableUI tableViewer = new TableUI(parametersTable,
                                       paramRef.getEReferenceType(), loader);
        tableViewer.setInput(eObjects);
        parametersTable.setSelection(0);
        try {
            InputStreamReader xml = new InputStreamReader(
                getClass().getResourceAsStream("classdialogtoolbar.xml"));
            new MenuBuilder(xml, tableViewer).getToolbar(parametersGroup, 0);
        } catch (Exception exception) {
            CaPlugin.LOG.error("Error creating Parameters Toolbar", exception);
        }
    }
    /**
     * @return boolean
     * remove listeners on close
     */
    public boolean close()
    {
        _dialogValidator.removeListeners();
        _dialogValidator = null;
        _paramValidator.removeListeners();
        _paramValidator = null;
        return super.close();
    }
}
