/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/factory/WidgetProviderFactory.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.factory;

import java.util.List;
import java.util.regex.Pattern;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
/**
 * @author shubhada
 *Factory Class to provide contents of a dialog
 */
public class WidgetProviderFactory
{
    private EObject _eObj;
    private Object _container = null;
    private DialogValidator _validator = null;
    /**
     * Constructor
     * @param eobject EObject
     */
    public WidgetProviderFactory(EObject eobject)
    {
        _eObj = eobject;
    }
    /**
     * Constructor
     * @param eobject EObject
     * @param container Container Dialog
     * @param validator Validator
     */
    public WidgetProviderFactory(EObject eobject, Object container,
    		DialogValidator validator)
    {
        _eObj = eobject;
        _container = container;
        _validator = validator;
    }
    /**
     * Creates Text for this feature
     * @param  container Composite
     * @param  style     Style of Composite
     * @param  feature   Feature
     * @return Text
     */
    public Text getTextBox(Composite container, int style,
                           EStructuralFeature feature)
    {
        Text textBox = new Text(container, style);
        textBox.setText(_eObj.eGet(feature).toString());
        textBox.addModifyListener(new TextComboButtonListener(_eObj, feature));
        return textBox;
    }
    /**
     * Creates Combo for this feature, Type of feature should be EEnum
     * or EBoolean.
     * @param  container Composite
     * @param  style     Style of Composite
     * @param  feature   Feature
     * @return Combo
     */
    public  CCombo getComboBox(Composite container, int style,
                               EStructuralFeature feature)
    {
        CCombo cBox = new CCombo(container, style);
        String value = _eObj.eGet(feature).toString();
        if (feature.getEType().getClassifierID()
                == EcorePackage.EBOOLEAN) {
            cBox.add(Boolean.toString(true));
            cBox.add(Boolean.toString(false));
            cBox.select(Boolean.parseBoolean(value) ? 0 : 1);
        } else {
            EEnum eEn = (EEnum) feature.getEType();
            List literals  = eEn.getELiterals();
            for (int i = 0; i < literals.size(); i++) {
                cBox.add(((EEnumLiteral) literals.get(i)).getName());
            }
            cBox.select(EcoreUtils.getIndex(eEn, eEn.getEEnumLiteral(value)));
        }
        cBox.addModifyListener(new TextComboButtonListener(_eObj, feature));
        return cBox;
    }
    /**
     * Creates button for this feature
     * @param  container Composite
     * @param  style     Style of Composite
     * @param  feature   Feature
     * @return Button
     */
    public  Button getButton(Composite container, int style,
                             EStructuralFeature feature)
    {
        Button button = new Button(container, style);
        button.setText(EcoreUtils.getLabel(feature));
        button.setSelection(((Boolean) _eObj.eGet(feature)).
                booleanValue());
        button.addSelectionListener(new
                TextComboButtonListener(_eObj, feature));
        return button;
    }
    /**
     * Creates set of radio buttons for this feature
     * @param  container Composite
     * @param  style     Style of Composite
     * @param  feature   Feature
     * @return Buttons
     */
    public Button [] getRadioButtons(Composite container, int style,
                             EStructuralFeature feature)
    {
        Button [] buttons = null;
        if (feature.getEType() instanceof EEnum) {
            EEnum enums = (EEnum) feature.getEType();
            EList literals = enums.getELiterals();
            buttons  = new Button[literals.size()];
            for (int i = 0; i < literals.size(); i++) {
            buttons[i] = new Button(container, SWT.RADIO | style);
            buttons[i].setText(((EEnumLiteral) literals.get(i)).getName());
            EEnumLiteral sel = (EEnumLiteral) _eObj.eGet(feature);
            boolean val = sel.equals(literals.get(i));
            buttons[i].setSelection(val);
            buttons[i].addSelectionListener(new
                    TextComboButtonListener(_eObj, feature));
            }
        }
        return buttons;
    }
    /**
     * Create appropriate control for this feature.
     * @param  container Composite
     * @param  style     Style of Composite
     * @param  feature   Feature
     * @return control (Text, Button, PushbuttonDialog) based on feature
     */
    public Control getControl(Composite container, int style,
            EStructuralFeature feature)
    {
        Control control = null;
        if (feature.getEType() instanceof EEnum) {
            control = getComboBox(container, style, feature);
        } else if (feature instanceof EReference) {
            control = getPushButton(container, (EReference) feature);
        } else if (feature.getEType().getClassifierID()
                == EcorePackage.EBOOLEAN) {
            control = getComboBox(container, style, feature);
        } else {
            control = getTextBox(container, style, feature);
        }
        return control;
    }
    /**
     * Gets Push button to open dialog for references.
     * @param  container Parent Composite
     * @param  ref       Reference Feature
     * @return Button
     */
    public Control getPushButton(Composite container, EReference ref)
    {
        final Button result = new Button(container, SWT.DOWN);
        result.setText("...");
        final EClass refClass = ref.getEReferenceType();
        final Object value    = _eObj.eGet(ref);
        result.addSelectionListener(new SelectionListener() {
            public void widgetSelected(SelectionEvent e)
            {
                new PushButtonDialog(result.getShell(), refClass, value).open();
            }
            public void widgetDefaultSelected(SelectionEvent e)
            { }
        });
        return result;
    }
    /**
     * Listener for value changed in widgets.
     * @author shubhada
     * On Change sets the value of feature in the EObject.
     */
    private class TextComboButtonListener
        implements ModifyListener , SelectionListener
    {
        private EObject            _eobj;
        private EStructuralFeature _feature;
        /**
         * Constructor
         * @param eobj EObject
         * @param f    Feature
         */
        public TextComboButtonListener(EObject eobj, EStructuralFeature f)
        {
            super();
            _eobj    = eobj;
            _feature = f;
        }
        /**
         * Default Selection. does nothing
         * @param e Event
         */
        public void widgetDefaultSelected(SelectionEvent e)
        { }
        /**
         * When text/combo is modified.
         * @param e Event
         */
        public void modifyText(ModifyEvent e)
        {
            Object sel = e.getSource();
            if (sel instanceof Text) {
                String text = ((Text) sel).getText();
                String pattern =
                    EcoreUtils.getAnnotationVal(_feature, null, "pattern");
                String msg =
                    EcoreUtils.getAnnotationVal(_feature, null, "message");
                if (pattern == null) {
                    if ((_feature.getEType().getClassifierID() == EcorePackage.EINT 
                      || _feature.getEType().getClassifierID() == EcorePackage.ELONG)
                      && !Pattern.compile("^[0-9][0-9]*$").
                            matcher(text).matches()) {
                        msg = "The value is not a number";
                        updateStatus(msg, IMessageProvider.ERROR, false);
                    } else {
                        EcoreUtils.setValue(_eobj, _feature.getName(), text);
                        updateStatus("", IMessageProvider.NONE, true);
                    }

               } else {
                   if (!Pattern.compile(pattern).
                       matcher(text).matches()) {
                	   updateStatus(msg, IMessageProvider.ERROR, false);
                   }else {
                	   EcoreUtils.setValue(_eobj, _feature.getName(), text);
                	   updateStatus("", IMessageProvider.NONE, true);
                   }
               }
            } else if (sel instanceof CCombo) {
                String text = ((CCombo) sel).getText();
                if (_feature.getEType().getClassifierID()
                        == EcorePackage.EBOOLEAN) {
                    _eobj.eSet(_feature, Boolean.valueOf(text));
                } else {
                    EEnum eEnum = ((EEnum) _feature.getEType());
                    _eobj.eSet(_feature, eEnum.getEEnumLiteral(text));
                }
            }
        }
        /**
         * Button is selected/unselected.
         * @param e Event
         */
        public void widgetSelected(SelectionEvent e)
        {
            Button button = (Button) e.getSource();
            if (_feature.getEType().getClassifierID()
                    == EcorePackage.EBOOLEAN) {
                _eobj.eSet(_feature, Boolean.valueOf(button.getSelection()));
            } else if (_feature.getEType() instanceof EEnum) {
                EEnum eEnum = eEnum = ((EEnum) _feature.getEType());
                _eobj.eSet(_feature, eEnum.getEEnumLiteral(button.getText()));
            }
        }
        /**
         * 
         * @param msg String
         * @param type Message Type
         * @param validatorStatus Can be true/false
         */
        private void updateStatus(String msg, int type, boolean validatorStatus)
        {
        	if (_container != null) {
                if (_container instanceof PreferenceDialog) {
                    ((PreferenceDialog) _container).setMessage(msg, type);
                } else if (_container instanceof TitleAreaDialog) {
                    ((TitleAreaDialog) _container).setMessage(msg, type);
                } else if (_container instanceof DialogPage) {
                        ((DialogPage) _container).setMessage(msg, type);
                }
        	}
            if (_validator != null) {
            _validator.setValid(validatorStatus);
            }
            
        }
    }
}
