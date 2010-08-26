/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ComponentPropertiesDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.regex.Pattern;

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.emf.ecore.util.EcoreUtil;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;

/**
 * Properties Dialog for Non SAF Component
 * @author Pushparaj
 *
 */
public class NonSAFComponentPropertiesDialog extends TitleAreaDialog{

	private EClass      _eClass      = null;	
	private EObject 	_compObject;
	private Model       _viewModel   = null;
	private Text terminateText, cleanupText, nameText, imageText, inslevelText,
			insdelayText, maxinsText, maxinsdelayText, maxtermText,
			maxactiveCSIText, maxstandbyCSIText;
	private CCombo propertyCombo, /*processCombo,*/ restartableCombo, rebootCombo,
			recoveryCombo;
	private Button cmdlineButton, envarButton, timeoutButton,
			healthCheckButton;
	private static final Log LOG = Log.getLog(CaPlugin.getDefault());
	private boolean isProxied = false;
	private Map recoveryTable = new HashMap();
	private Map boolTable = new HashMap();
	private Map propertyTable = new HashMap();

	private BigInteger maxBigInteger = new BigInteger("9223372036854775807");

	private BigInteger minBigInteger = new BigInteger("-9223372036854775808");
	
	/**
     * create the contents of the Dialog.
     * @param  parent Parent Composite
     * @return Dialog area.
     */
	public NonSAFComponentPropertiesDialog(Shell shell, EObject obj, boolean proxied){
		super(shell);
		setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
		isProxied = proxied;
		Model model = new Model(null, obj);
		_viewModel  = model.getViewModel();
		_compObject = _viewModel.getEObject();
		_eClass = _compObject.eClass();
		fillTables();
	}
	private void fillTables() {
		boolTable.put("true", "CL_TRUE");
		boolTable.put("false", "CL_FALSE");
		if (isProxied) {
			propertyTable.put("Proxied Preinstantiable",
					"CL_AMS_PROXIED_PREINSTANTIABLE");
			propertyTable.put("Proxied Non Preinstantiable",
					"CL_AMS_PROXIED_NON_PREINSTANTIABLE");
		} else {
			propertyTable.put("Non Proxied Non Preinstantiable",
					"CL_AMS_NON_PROXIED_NON_PREINSTANTIABLE");
		}
		recoveryTable.put("No recommendation", "CL_AMS_RECOVERY_NO_RECOMMENDATION");
		recoveryTable.put("Internally recovered", "CL_AMS_RECOVERY_INTERNALLY_RECOVERED");
		recoveryTable.put("Component restart", "CL_AMS_RECOVERY_COMP_RESTART");
		recoveryTable.put("Component failover", "CL_AMS_RECOVERY_COMP_FAILOVER");
		recoveryTable.put("Node switchover", "CL_AMS_RECOVERY_NODE_SWITCHOVER");
		recoveryTable.put("Node failover", "CL_AMS_RECOVERY_NODE_FAILOVER");
		recoveryTable.put("Node failfast", "CL_AMS_RECOVERY_NODE_FAILFAST");
		recoveryTable.put("Cluster reset", "CL_AMS_RECOVERY_CLUSTER_RESET");
	}
	protected Control createDialogArea(Composite parent)
    {
		setTitle(EcoreUtils.getLabel(_eClass) + " Details");
		getShell().setText(EcoreUtils.getLabel(_eClass) + " Details");
		
		Composite container = new Composite(parent, SWT.NONE);
		container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
               
        Label nameLabel = new Label(container, SWT.NONE);
        nameLabel.setText("Name:");
        nameLabel.setLayoutData(new GridData(GridData.BEGINNING));
        nameText = new Text(container, SWT.BORDER);
        nameText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        setTextValue(nameText, _eClass.getEStructuralFeature(
                ComponentEditorConstants.NAME));
        nameText.addModifyListener(new TextChangeAdapter1(nameText, _eClass.getEStructuralFeature(
                ComponentEditorConstants.NAME)));
             
        Label propertyLabel = new Label(container, SWT.NONE);
		propertyLabel.setText("Component type:");
		propertyLabel.setLayoutData(new GridData(GridData.BEGINNING));
		propertyCombo = new CCombo(container, SWT.BORDER);
		propertyCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		setComboValue(propertyCombo, _eClass.getEStructuralFeature("property"),
				propertyTable);

		if (isProxied) {
			Label inslevelLabel = new Label(container, SWT.NONE);
			inslevelLabel.setText("Instantiate level:");
			inslevelLabel.setLayoutData(new GridData(GridData.BEGINNING));
			inslevelText = new Text(container, SWT.BORDER);
			inslevelText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(inslevelText, _eClass
					.getEStructuralFeature("instantiateLevel"));
			inslevelText.addModifyListener(new TextChangeAdapter2(inslevelText,
					_eClass.getEStructuralFeature("instantiateLevel")));

			Label insdelayLabel = new Label(container, SWT.NONE);
			insdelayLabel.setText("Instantiate delay(milliseconds):");
			insdelayLabel.setLayoutData(new GridData(GridData.BEGINNING));
			insdelayText = new Text(container, SWT.BORDER);
			insdelayText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(insdelayText, _eClass
					.getEStructuralFeature("instantiateDelay"));
			insdelayText.addModifyListener(new TextChangeAdapter2(insdelayText,
					_eClass.getEStructuralFeature("instantiateDelay")));

			Label maxinsLabel = new Label(container, SWT.NONE);
			maxinsLabel.setText("Maximum number of instantiations:");
			maxinsLabel.setLayoutData(new GridData(GridData.BEGINNING));
			maxinsText = new Text(container, SWT.BORDER);
			maxinsText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(maxinsText, _eClass
					.getEStructuralFeature("numMaxInstantiate"));
			maxinsText.addModifyListener(new TextChangeAdapter2(maxinsText,
					_eClass.getEStructuralFeature("numMaxInstantiate")));

			Label maxinsdelayLabel = new Label(container, SWT.NONE);
			maxinsdelayLabel
					.setText("Maximum number of instantiations after delay:");
			maxinsdelayLabel.setLayoutData(new GridData(GridData.BEGINNING));
			maxinsdelayText = new Text(container, SWT.BORDER);
			maxinsdelayText
					.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(maxinsdelayText, _eClass
					.getEStructuralFeature("numMaxInstantiateWithDelay"));
			maxinsdelayText
					.addModifyListener(new TextChangeAdapter2(
							maxinsdelayText,
							_eClass
									.getEStructuralFeature("numMaxInstantiateWithDelay")));

			Label maxtermLabel = new Label(container, SWT.NONE);
			maxtermLabel.setText("Maximum number of terminations:");
			maxtermLabel.setLayoutData(new GridData(GridData.BEGINNING));
			maxtermText = new Text(container, SWT.BORDER);
			maxtermText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(maxtermText, _eClass
					.getEStructuralFeature("numMaxTerminate"));
			maxtermText.addModifyListener(new TextChangeAdapter2(maxtermText,
					_eClass.getEStructuralFeature("numMaxTerminate")));

/*			Label maxactiveCSILabel = new Label(container, SWT.NONE);
			maxactiveCSILabel.setText("Maximum number of active CSIs:");
			maxactiveCSILabel.setLayoutData(new GridData(GridData.BEGINNING));
			maxactiveCSIText = new Text(container, SWT.BORDER);
			maxactiveCSIText.setLayoutData(new GridData(
					GridData.FILL_HORIZONTAL));
			setTextValue(maxactiveCSIText, _eClass
					.getEStructuralFeature("numMaxActiveCSIs"));
			maxactiveCSIText.addModifyListener(new TextChangeAdapter2(
					maxactiveCSIText, _eClass
							.getEStructuralFeature("numMaxActiveCSIs")));

			Label maxstandbyCSILabel = new Label(container, SWT.NONE);
			maxstandbyCSILabel.setText("Maximum number of standby CSIs:");
			maxstandbyCSILabel.setLayoutData(new GridData(GridData.BEGINNING));
			maxstandbyCSIText = new Text(container, SWT.BORDER);
			maxstandbyCSIText.setLayoutData(new GridData(
					GridData.FILL_HORIZONTAL));
			setTextValue(maxstandbyCSIText, _eClass
					.getEStructuralFeature("numMaxStandbyCSIs"));
			maxstandbyCSIText.addModifyListener(new TextChangeAdapter2(
					maxstandbyCSIText, _eClass
							.getEStructuralFeature("numMaxStandbyCSIs")));

			Label processLabel = new Label(container, SWT.NONE);
			processLabel.setText("Process:");
			processLabel.setLayoutData(new GridData(GridData.BEGINNING));
			processCombo = new CCombo(container, SWT.BORDER);
			processCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			processCombo.add("Single process");
			processCombo.select(0);
			processCombo.setEnabled(false);

			Label restartableText = new Label(container, SWT.NONE);
			restartableText.setText("Is restartable:");
			restartableText.setLayoutData(new GridData(GridData.BEGINNING));
			restartableCombo = new CCombo(container, SWT.BORDER);
			restartableCombo.setLayoutData(new GridData(
					GridData.FILL_HORIZONTAL));
			setComboValue(restartableCombo, _eClass
					.getEStructuralFeature("isRestartable"), boolTable);

			Label rebootText = new Label(container, SWT.NONE);
			rebootText.setText("Reboot node on cleanup failure:");
			rebootText.setLayoutData(new GridData(GridData.BEGINNING));
			rebootCombo = new CCombo(container, SWT.BORDER);
			rebootCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setComboValue(rebootCombo, _eClass
					.getEStructuralFeature("nodeRebootCleanupFail"), boolTable);
*/
			Label recoveryLabel = new Label(container, SWT.NONE);
			recoveryLabel.setText("Recovery action an error:");
			recoveryLabel.setLayoutData(new GridData(GridData.BEGINNING));
			recoveryCombo = new CCombo(container, SWT.BORDER);
			recoveryCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setComboValue(recoveryCombo, _eClass
					.getEStructuralFeature("recoveryOnTimeout"), recoveryTable);

			Label timeoutLabel = new Label(container, SWT.NONE);
			timeoutLabel.setText("Timeouts:");
			timeoutLabel.setLayoutData(new GridData(GridData.BEGINNING));
			timeoutButton = new Button(container, SWT.BORDER | SWT.PUSH);
			timeoutButton.setText("Edit...");
			addPushButton(timeoutButton, "timeouts");
			timeoutButton.setLayoutData(new GridData(
					GridData.HORIZONTAL_ALIGN_END));

			Label healthCheckLabel = new Label(container, SWT.NONE);
			healthCheckLabel.setText("Healthcheck:");
			healthCheckLabel.setLayoutData(new GridData(GridData.BEGINNING));
			healthCheckButton = new Button(container, SWT.BORDER | SWT.PUSH);
			healthCheckButton.setText("Edit...");
			addPushButton(healthCheckButton, "healthCheck");
			healthCheckButton.setLayoutData(new GridData(
					GridData.HORIZONTAL_ALIGN_END));

		} else {
			Label imageLabel = new Label(container, SWT.NONE);
			imageLabel.setText("Binary:");
			imageLabel.setLayoutData(new GridData(GridData.BEGINNING));
			imageText = new Text(container, SWT.BORDER);
			imageText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(imageText, _eClass
					.getEStructuralFeature("instantiateCommand"));
			imageText.addModifyListener(new TextChangeAdapter1(imageText,
					_eClass.getEStructuralFeature("instantiateCommand")));
			
			Label cmdlineLabel = new Label(container, SWT.NONE);
			cmdlineLabel.setText("Command line arguments:");
			cmdlineLabel.setLayoutData(new GridData(GridData.BEGINNING));
			cmdlineButton = new Button(container, SWT.BORDER | SWT.PUSH);
			cmdlineButton.setText("Edit...");
			addPushButton(cmdlineButton, "commandLineArgument");
			cmdlineButton.setLayoutData(new GridData(
					GridData.HORIZONTAL_ALIGN_END));

			Label envarLabel = new Label(container, SWT.NONE);
			envarLabel.setText("Environment variables:");
			envarLabel.setLayoutData(new GridData(GridData.BEGINNING));
			envarButton = new Button(container, SWT.BORDER | SWT.PUSH);
			envarButton.setText("Edit...");
			addPushButton(envarButton, "environmentVariable");
			envarButton.setLayoutData(new GridData(
					GridData.HORIZONTAL_ALIGN_END));
			
			Label terminateLabel = new Label(container, SWT.NONE);
			terminateLabel.setText("Termination command:");
			terminateLabel.setLayoutData(new GridData(GridData.BEGINNING));
			terminateText = new Text(container, SWT.BORDER);
			terminateText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(terminateText, _eClass
					.getEStructuralFeature("terminateCommand"));
			terminateText.addModifyListener(new TextChangeAdapter1(terminateText,
					_eClass.getEStructuralFeature("terminateCommand")));

			Label cleanupLabel = new Label(container, SWT.NONE);
			cleanupLabel.setText("Cleanup command:");
			cleanupLabel.setLayoutData(new GridData(GridData.BEGINNING));
			cleanupText = new Text(container, SWT.BORDER);
			cleanupText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			setTextValue(cleanupText, _eClass
					.getEStructuralFeature("cleanupCommand"));
			cleanupText.addModifyListener(new TextChangeAdapter1(cleanupText,
					_eClass.getEStructuralFeature("cleanupCommand")));

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
	private void setTextValue(Text text, EStructuralFeature feature)
	{
		String  value = EcoreUtils.getValue(_compObject, feature.getName()).toString();
		text.setText(value);
	}
	private void setComboValue(CCombo combo, EStructuralFeature feature, Map items)
	{
		String item = EcoreUtils.getValue(_compObject, feature.getName()).toString();
		Object keys[] = items.keySet().toArray();
		int index = -1;
		for(int i = 0; i < keys.length; i++) {
			String key = (String) keys[i];
			String value = (String)items.get(key);
			combo.add(key);
			if(item.equals(value)){
				index = i;
			}
		}
		combo.select(index);
	}
	private void addPushButton(Button button, final String name) 
	{
		EReference reference = (EReference) _eClass.getEStructuralFeature(name);
		final EClass eClass = reference.getEReferenceType();
		final Shell shell = button.getShell();
		final Object value = _compObject.eGet(reference);
		button.addSelectionListener(new SelectionListener() {
            public void widgetSelected(SelectionEvent e)
            {
            	if(name.equals("timeouts")) {
            		setTimeoutsHidden((EObject) value, "true");
            	}
                new PushButtonDialog(shell, eClass, value).open();
            	if(name.equals("timeouts")) {
            		setTimeoutsHidden((EObject) value, "false");
            	}
            }
            public void widgetDefaultSelected(SelectionEvent e)
            { }
        });
	}

	private void setTimeoutsHidden(EObject timeoutsObj, String hidden) {
		ArrayList<String> excludeTimeout = new ArrayList<String>();

		if (propertyCombo.getText().equals("Proxied Preinstantiable")) {
			Collections.addAll(excludeTimeout, "instantiateTimeout",
					"cleanupTimeout");
		} else if (propertyCombo.getText()
				.equals("Proxied Non Preinstantiable")) {
			Collections.addAll(excludeTimeout, "instantiateTimeout",
					"terminateTimeout", "cleanupTimeout",
					"proxiedCompInstantiateTimeout");
		} else {
			return;
		}

		EStructuralFeature feature;
		Iterator<EStructuralFeature> featureItr = timeoutsObj.eClass()
				.getEAllStructuralFeatures().iterator();

		while (featureItr.hasNext()) {
			feature = featureItr.next();

			if (excludeTimeout.contains(feature.getName())) {
				EcoreUtil.setAnnotation(feature, EcoreUtils.CW_ANNOTATION_NAME,
						"isHidden", hidden);
			}
		}
	}

	private void updateViewModel()
	{
		EcoreUtils.setValue(_compObject, "name", nameText.getText());
		EcoreUtils.setValue(_compObject, "property", propertyTable.get(
				propertyCombo.getText()).toString());

		if (isProxied) {
			EcoreUtils.setValue(_compObject, "instantiateLevel", inslevelText
					.getText());
			EcoreUtils.setValue(_compObject, "instantiateDelay", insdelayText
					.getText());
			EcoreUtils.setValue(_compObject, "numMaxInstantiate", maxinsText
					.getText());
			EcoreUtils.setValue(_compObject, "numMaxInstantiateWithDelay",
					maxinsdelayText.getText());
			EcoreUtils.setValue(_compObject, "numMaxTerminate", maxtermText
					.getText());
			EcoreUtils.setValue(_compObject, "recoveryOnTimeout", recoveryTable
					.get(recoveryCombo.getText()).toString());

		} else {
			EcoreUtils.setValue(_compObject, "instantiateCommand", imageText
					.getText());
			EcoreUtils.setValue(_compObject, "terminateCommand", terminateText
					.getText());
			EcoreUtils.setValue(_compObject, "cleanupCommand", cleanupText
					.getText());
		}
	}
	/**
     * Save the Model.
     */
	public void okPressed()
	{
		updateViewModel();
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
     * 
     * @return view model
     */
    public Model getViewModel()
    {
        return _viewModel;
    }
    private boolean isValidString(String text, EStructuralFeature feature)
    {
    	String message = EcoreUtils.getAnnotationVal(feature, null, "message");
		if(message == null) {
			String label = EcoreUtils.getAnnotationVal(feature, null, "label");
			if(label == null) {
				label = feature.getName(); 
			}
			message = "Invalid " + label;
 		}
    	String pattern = EcoreUtils.getAnnotationVal(feature, null, "pattern");
    	if(pattern == null) {
    		setMessage("");
    		return true;
    	}
    	if(!Pattern.compile(pattern).matcher(text).matches()) {
    		setMessage(message, IMessageProvider.ERROR);
    		return false;
    	} else {
    		setMessage("");
    		return true;
    	}
    }
    private boolean isValidNumber(String value, EStructuralFeature feature)
    {
    	String message = EcoreUtils.getAnnotationVal(feature, null, "message");
		if(message == null) {
			String label = EcoreUtils.getAnnotationVal(feature, null, "label");
			if(label == null) {
				label = feature.getName(); 
			}
			message = "Value is not a number";
 		}
    	String pattern = EcoreUtils.getAnnotationVal(feature, null, "pattern");
    	if(pattern == null) {
    		pattern = "^-?[0-9][0-9]*$";
    	}
    	if(!Pattern.compile(pattern).matcher(value).matches()){
    		setMessage(message, IMessageProvider.ERROR);
    		return false;
    	} else if (feature.getEType().getClassifierID() == EcorePackage.ESHORT
				&& (Short.parseShort(value) > 32767 || Short
						.parseShort(value) < -32768)) {
			setMessage("Value provided is not within short range",
					IMessageProvider.ERROR);
			return false;
		} else if (feature.getEType().getClassifierID() == EcorePackage.EINT
				&& (Integer.parseInt(value) > 2147483647 || Integer
						.parseInt(value) < -2147483648)) {
			setMessage("Value provided is not within integer range",
					IMessageProvider.ERROR);
			return false;
		} else if (feature.getEType().getClassifierID() == EcorePackage.ELONG) {
			BigInteger bgvalue = new BigInteger(value
					.toString());
			if (bgvalue.compareTo(maxBigInteger) > 0
					|| bgvalue.compareTo(minBigInteger) < 0) {
				setMessage("Value provided is not within long range",
						IMessageProvider.ERROR);
				return false;
			}
		
		} else {
			setMessage("");
			return true;
		}
    	return true;
    }
    private boolean isValidDatas()
    {
    	if (!isValidString(nameText.getText(), _eClass
				.getEStructuralFeature("name"))) {
			return false;
		}

    	if (isProxied) {
			if (!isValidNumber(inslevelText.getText(), _eClass
					.getEStructuralFeature("instantiateLevel"))) {
				return false;
			}
			if (!isValidNumber(insdelayText.getText(), _eClass
					.getEStructuralFeature("instantiateDelay"))) {
				return false;
			}
			if (!isValidNumber(maxinsText.getText(), _eClass
					.getEStructuralFeature("numMaxInstantiate"))) {
				return false;
			}
			if (!isValidNumber(maxinsdelayText.getText(), _eClass
					.getEStructuralFeature("numMaxInstantiateWithDelay"))) {
				return false;
			}
			if (!isValidNumber(maxtermText.getText(), _eClass
					.getEStructuralFeature("numMaxTerminate"))) {
				return false;
			}

		} else {
			if (!isValidString(imageText.getText(), _eClass
					.getEStructuralFeature("instantiateCommand"))) {
				return false;
			}
			if (!isValidString(terminateText.getText(), _eClass
					.getEStructuralFeature("terminateCommand"))) {
				return false;
			}
			if (!isValidString(cleanupText.getText(), _eClass
					.getEStructuralFeature("cleanupCommand"))) {
				return false;
			}
		}
		return true;
    }
    class TextChangeAdapter1 implements ModifyListener
    {
    	Text text;
    	EStructuralFeature feature;
    	public TextChangeAdapter1(Text text, EStructuralFeature feature)
    	{
    		this.text = text;
    		this.feature = feature;
    		
    	}
		public void modifyText(ModifyEvent e)
		{
			if(isValidString(text.getText(), feature))
			{
				if (isValidDatas())
					getButton(OK).setEnabled(true);
				else
					getButton(OK).setEnabled(false);	
			}else {
				getButton(OK).setEnabled(false);
			}
		}
    }
    class TextChangeAdapter2 implements ModifyListener
    {
    	Text text;
    	EStructuralFeature feature;
    	public TextChangeAdapter2(Text text, EStructuralFeature feature)
    	{
    		this.text = text;
    		this.feature = feature;
    		
    	}
		public void modifyText(ModifyEvent e) {
			String value = text.getText();
			try {
				if (isValidNumber(value, feature)) {
					if (isValidDatas())
						getButton(OK).setEnabled(true);
					else
						getButton(OK).setEnabled(false);	
				} else {
					getButton(OK).setEnabled(false);
				}
			} catch (NumberFormatException ex) {
				setMessage("Value provided is not within proper range",
						IMessageProvider.ERROR);
			}
		}
    }
}
