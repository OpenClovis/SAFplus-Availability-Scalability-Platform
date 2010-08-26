package com.clovis.cw.editor.ca.handler;

import java.math.BigInteger;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.viewers.StructuredSelection;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.factory.CellEditorValidator;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.common.utils.ui.table.TableUI;

/**
 * Specific Validator for Attributes
 * @author Pushparaj
 *
 */
public class CellEditorValidatorFor64Bit extends CellEditorValidator{
	private EStructuralFeature _feature;
	private Environment _env;
	private final BigInteger INT64_MINVALUE = new BigInteger("-9223372036854775808");
	private final BigInteger INT64_MAXVALUE = new BigInteger("9223372036854775807");
	private final BigInteger UINT64_MINVALUE = new BigInteger("0");
	private final BigInteger UINT64_MAXVALUE = new BigInteger("18446744073709551615");
	/**
	 * Constructor 
	 * @param feature EStructuralFeature
	 * @param env Environment
	 */	 
	public CellEditorValidatorFor64Bit(EStructuralFeature feature, Environment env) {
		super(feature, env);
		_feature = feature;
		_env = env;
	}
	/**
	 * Creates CellEditorValidator
	 * @param feature EStructuralFeature
	 * @param env Environment
	 * @return CellEditorValidatorFor64Bitor
	 */
	public static CellEditorValidatorFor64Bit createCellEditorValidator(EStructuralFeature feature, Environment env) {
		return new CellEditorValidatorFor64Bit(feature, env);
	}
	/**
	 * @see com.clovis.common.utils.ui.factory.CellEditorValidator#isValid(java.lang.Object)
	 */
	public String isValid(Object value) {
		if(value == null || _feature == null)
			return super.isValid(value);
		try {
			String featureName = _feature.getName();
			EObject eobj = null;
			if(_env instanceof TableUI) {
				TableUI viewer = (TableUI) _env.getValue("tableviewer");
				StructuredSelection selection = (StructuredSelection) viewer.getSelection();
				eobj = (EObject) selection.getFirstElement();
			} else if(_env instanceof FormView) {
				eobj = (EObject) _env.getValue("model");
			} else {
				return super.isValid(value);
			}
			
			if (featureName.equals("dataType")) {
				String dataType = value.toString();
				BigInteger minValue = new BigInteger(EcoreUtils.getValue(eobj,
						"minValue").toString());
				BigInteger maxValue = new BigInteger(EcoreUtils.getValue(eobj,
						"maxValue").toString());
				BigInteger defaultValue = new BigInteger(EcoreUtils.getValue(
						eobj, "defaultValue").toString());
				if (dataType.equals("Int64")) {
					if (minValue.compareTo(INT64_MAXVALUE) > 0
							|| minValue.compareTo(INT64_MINVALUE) < 0
							|| maxValue.compareTo(INT64_MAXVALUE) > 0
							|| maxValue.compareTo(INT64_MINVALUE) < 0
							|| defaultValue.compareTo(INT64_MAXVALUE) > 0
							|| defaultValue.compareTo(INT64_MINVALUE) < 0) {
						String msg = "value is not in int64 range(-9223372036854775808 to 9223372036854775807)";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					} else {
						return null;
					}
				} else if (dataType.equals("Uint64")
						|| dataType.equals("Counter64")) {
					if (minValue.compareTo(UINT64_MAXVALUE) > 0
							|| minValue.compareTo(UINT64_MINVALUE) < 0
							|| maxValue.compareTo(UINT64_MAXVALUE) > 0
							|| maxValue.compareTo(UINT64_MINVALUE) < 0
							|| defaultValue.compareTo(UINT64_MAXVALUE) > 0
							|| defaultValue.compareTo(UINT64_MINVALUE) < 0) {
						String msg = "value is not in int64 range(-9223372036854775808 to 9223372036854775807)";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					} else {
						return null;
					}
				}
				return super.isValid(value);
			} else if (featureName.equals("minValue")
					|| featureName.equals("maxValue")
					|| featureName.equals("defaultValue")) {
				String dataType = String.valueOf(EcoreUtils.getValue(eobj,
						"dataType"));
				BigInteger data = new BigInteger(value.toString().trim());
				if (dataType.equals("Int64")) {
					if (data.compareTo(INT64_MAXVALUE) > 0
							|| data.compareTo(INT64_MINVALUE) < 0) {
						String msg = "value is not in int64 range(-9223372036854775808 to 9223372036854775807)";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					} else {
						return null;
					}
				} else if (dataType.equals("Uint64")
						|| dataType.equals("Counter64")) {
					if (data.compareTo(UINT64_MAXVALUE) > 0
							|| data.compareTo(UINT64_MINVALUE) < 0) {
						String msg = "value is not in uint64/counter64 range(0 to 18446744073709551615)";
						showMsg(msg, IMessageProvider.ERROR);
						return msg;
					} else {
						return null;
					}
				}
				return super.isValid(value);
			} else {
				return super.isValid(value);
			}
		} catch (Exception e) {
			String msg = "Value provided is not within proper range";
			showMsg(msg, IMessageProvider.ERROR);
			return msg;
		}
	}
}
