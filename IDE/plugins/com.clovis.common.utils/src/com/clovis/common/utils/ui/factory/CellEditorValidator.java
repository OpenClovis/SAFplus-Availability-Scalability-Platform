/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/factory/CellEditorValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.factory;

import java.math.BigInteger;
import java.util.regex.Pattern;

import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.ICellEditorValidator;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.DialogValidator;

/**
 * @author shubhada Cell Editor Validator
 */
public class CellEditorValidator implements ICellEditorValidator {
	private EStructuralFeature _feature = null;

	private Environment _env = null;

	private BigInteger maxBigInteger = new BigInteger("9223372036854775807");

	private BigInteger minBigInteger = new BigInteger("-9223372036854775808");

	/**
	 * @param feature
	 *            EStructuralFeature
	 * @param env
	 *            Environment
	 * 
	 */
	public CellEditorValidator(EStructuralFeature feature, Environment env) {
		_feature = feature;
		_env = env;
	}

	/**
	 * @param value -
	 *            Value to be validated
	 * @return the Error Message if invalid else return null
	 */
	public String isValid(Object value) {
		String pattern = EcoreUtils.getAnnotationVal(_feature, null, "pattern");
		String msg = EcoreUtils.getAnnotationVal(_feature, null, "message");
		if (pattern == null) {
			if ((_feature.getEType().getClassifierID() == EcorePackage.EBIG_INTEGER || _feature.getEType().getClassifierID() == EcorePackage.EINT
					|| _feature.getEType().getClassifierID() == EcorePackage.ELONG || _feature
					.getEType().getClassifierID() == EcorePackage.ESHORT)) {
				if (!Pattern.compile("^?[0-9][0-9]*$").matcher(
						value.toString()).matches()) {
					String fName = EcoreUtils.getAnnotationVal(_feature, null, "label");
					if(fName != null){
					msg = fName+" value is not valid";
					showMsg(msg, IMessageProvider.ERROR);
					return msg;
					}
					else{
						msg = _feature.getName()+" value is not valid";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					}
						
						
				} else {
					// Fix for bug # 3349 and similar cases where user enters
					// the
					// value which is greater than Max range of the data Type.
					try {

						if (_feature.getEType().getClassifierID() == EcorePackage.ESHORT
								&& (Short.parseShort(value.toString()) > 32767 || Short
										.parseShort(value.toString()) < -32768)) {
							msg = "Value provided is not within short range";
							showMsg(msg, IMessageProvider.ERROR);
							return msg;
						} else if (_feature.getEType().getClassifierID() == EcorePackage.EINT
								&& (Integer.parseInt(value.toString()) > 2147483647 || Integer
										.parseInt(value.toString()) < -2147483647)) {
							msg = "Value provided is not within integer range";
							showMsg(msg, IMessageProvider.ERROR);
							return msg;
						} else if (_feature.getEType().getClassifierID() == EcorePackage.ELONG) {
							BigInteger bgvalue = new BigInteger(value
									.toString());
							if (bgvalue.compareTo(maxBigInteger) > 0
									|| bgvalue.compareTo(minBigInteger) < 0) {
								msg = "Value provided is not within long range";
								showMsg(msg, IMessageProvider.ERROR);
								return msg;
							}
						} else if (_feature.getEType().getClassifierID() == EcorePackage.EBIG_INTEGER) {
							BigInteger bgvalue = new BigInteger(value
									.toString());
							if (bgvalue.compareTo(maxBigInteger) > 0
									|| bgvalue.compareTo(minBigInteger) < 0) {
								msg = "Value provided is not within 64 bit range";
								showMsg(msg, IMessageProvider.ERROR);
								return msg;
							}
						}
					} catch (NumberFormatException e) {
						msg = "Value provided is not within proper range";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					}
				}
			} else {
				msg = "";
				showMsg(msg, IMessageProvider.NONE);
				return null;
			}

		}
		else {
			if (!Pattern.compile(pattern).matcher(value.toString()).matches()) {
				showMsg(msg, IMessageProvider.ERROR);
				return msg;
			}			
			else {				
				try {					
					if (_feature.getEType().getClassifierID() == EcorePackage.ESHORT
							&& (Short.parseShort(value.toString()) > 32767 || Short
									.parseShort(value.toString()) < -32768)) {
						msg = "Value provided is not within short range";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					} else if (_feature.getEType().getClassifierID() == EcorePackage.EINT
							&& (Integer.parseInt(value.toString()) > 2147483647 || Integer
									.parseInt(value.toString()) < -2147483648)) {
						msg = "Value provided is not within integer range";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					} else if (_feature.getEType().getClassifierID() == EcorePackage.ELONG) {
						BigInteger bgvalue = new BigInteger(value.toString());
						if (bgvalue.compareTo(maxBigInteger) > 0
								|| bgvalue.compareTo(minBigInteger) < 0) {
							msg = "Value provided is not within long range";
							showMsg(msg, IMessageProvider.ERROR);
							return msg;
						}
					} else if (_feature.getEType().getClassifierID() == EcorePackage.EBIG_INTEGER) {
						BigInteger bgvalue = new BigInteger(value
								.toString());
						if (bgvalue.compareTo(maxBigInteger) > 0
								|| bgvalue.compareTo(minBigInteger) < 0) {
							msg = "Value provided is not within 64 bit range";
							showMsg(msg, IMessageProvider.ERROR);
							return msg;
						}
					}
				} catch (NumberFormatException e) {
					msg = "Value provided is not within proper range";
					showMsg(msg, IMessageProvider.ERROR);
					return msg;
				}
			}				
		}
		msg = "";
		showMsg(msg, IMessageProvider.NONE);
		return null;
	}

	/**
	 * @param msg -
	 *            Message to be Shown
	 * @param type -
	 *            Message Type
	 */
	protected void showMsg(String msg, int type) {
		if (_env != null) {
			Object container = _env.getValue("container");
			DialogValidator validator = (DialogValidator) _env
					.getValue("dialogvalidator");
			if (container != null) {
				if (validator == null) {
					if (container instanceof TitleAreaDialog) {
						((TitleAreaDialog) container).setMessage(msg, type);
					} else if (container instanceof PreferenceDialog) {
						((PreferenceDialog) container).setMessage(msg, type);
					} else if (container instanceof PreferencePage) {
						PreferencePage page = (PreferencePage) container;
						page.setMessage(msg, type);
						if (msg.equals("")) {
							page.setTitle(page.getTitle());
						}

					} else if (container instanceof DialogPage) {
						((DialogPage) container).setMessage(msg, type);
					}
				} else if (validator.isModelValid()) {
					validator.setValid(!(type == IMessageProvider.ERROR));
					validator.setMessage(msg);
					if (msg.equals("")) {
						validator.setTitle();
					}
				}
			}
		}

	}

}
