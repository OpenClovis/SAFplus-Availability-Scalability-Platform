/**
 * 
 */
package com.clovis.logtool.ui.filter;

import java.util.Calendar;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;

/**
 * Criterion composite for specifying the filter criteras.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CriterionComposite extends Composite {

	// Constants to find which contorl to show.
	static final int SINGLE_SPINNER = 0;

	static final int TWO_SPINNER = 1;

	static final int TEXT = 2;

	static final int SINGLE_TIMESTAMP = 3;

	static final int TWO_TIMESTAMP = 4;

	/**
	 * Specifies the type for this composite.
	 */
	private int _type;

	/**
	 * Composite for selecting single value.
	 */
	private Composite _singleSpinnerComposite;

	/**
	 * Composite for selecting range value.
	 */
	private Composite _twoSpinnerComposite;

	/**
	 * Composite for specifying text value.
	 */
	private Composite _textComposite;

	/**
	 * Composite for selecting single time stamp value.
	 */
	private Composite _singleTimeStampComposite;

	/**
	 * Composite for selecting range time stamp value.
	 */
	private Composite _twoTimeStampComposite;

	/**
	 * Constructs the Criterion composite.
	 * 
	 * @param parent
	 *            the parent composite
	 */
	public CriterionComposite(Composite parent) {
		super(parent, SWT.NONE);

		StackLayout criterionLayout = new StackLayout();
		criterionLayout.marginHeight = 0;
		criterionLayout.marginWidth = 0;
		setLayout(criterionLayout);

		GridData criterionData = new GridData(SWT.FILL, 0, true, false);
		setLayoutData(criterionData);
		createControls();
	}

	/**
	 * Creates controls for this composite.
	 */
	private void createControls() {
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;

		_singleSpinnerComposite = new Composite(this, SWT.NONE);
		_singleSpinnerComposite.setLayout(layout);

		Spinner singleSpinner = new Spinner(_singleSpinnerComposite, SWT.BORDER);
		singleSpinner.setMinimum(0);
		singleSpinner.setMaximum(Integer.MAX_VALUE);
		singleSpinner.setIncrement(1);
		singleSpinner.setPageIncrement(100);

		_twoSpinnerComposite = new Composite(this, SWT.NONE);
		layout = new GridLayout(3, false);
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		_twoSpinnerComposite.setLayout(layout);

		Spinner spinner1 = new Spinner(_twoSpinnerComposite, SWT.BORDER);
		spinner1.setMinimum(0);
		spinner1.setMaximum(Integer.MAX_VALUE);
		spinner1.setIncrement(1);
		spinner1.setPageIncrement(100);

		new Label(_twoSpinnerComposite, SWT.NONE).setText("-");

		Spinner spinner2 = new Spinner(_twoSpinnerComposite, SWT.BORDER);
		spinner2.setMinimum(0);
		spinner2.setMaximum(Integer.MAX_VALUE);
		spinner2.setIncrement(1);
		spinner2.setPageIncrement(100);

		_singleTimeStampComposite = new Composite(this, SWT.NONE);
		layout = new GridLayout(2, false);
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		_singleTimeStampComposite.setLayout(layout);

		new DateTime(_singleTimeStampComposite, SWT.DATE | SWT.BORDER);
		new DateTime(_singleTimeStampComposite, SWT.TIME | SWT.BORDER);

		_twoTimeStampComposite = new Composite(this, SWT.NONE);
		layout = new GridLayout(5, false);
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		_twoTimeStampComposite.setLayout(layout);

		new DateTime(_twoTimeStampComposite, SWT.DATE | SWT.BORDER);
		new DateTime(_twoTimeStampComposite, SWT.TIME | SWT.BORDER);

		new Label(_twoTimeStampComposite, SWT.NONE).setText("-");

		new DateTime(_twoTimeStampComposite, SWT.DATE | SWT.BORDER);
		new DateTime(_twoTimeStampComposite, SWT.TIME | SWT.BORDER);

		_textComposite = new Composite(this, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		_textComposite.setLayout(layout);
		new LTTextCellEditor(_textComposite);

		((StackLayout) getLayout()).topControl = _textComposite;
		_type = TEXT;
	}

	/**
	 * Returns the value for filter criterion.
	 * 
	 * @return the filter criterion value
	 */
	public String getCriterionValue() {
		String value = null;

		int typeSelection = ((Integer) ((FilterObjectPanel) getParent())
				.getTypeCombo().getValue()).intValue();

		switch (_type) {

		case SINGLE_SPINNER:
			switch (typeSelection) {
			case 0:
				value = String.valueOf(((Spinner) _singleSpinnerComposite
						.getChildren()[0]).getSelection());
				break;

			case 1:
				value = ":"	+ (((Spinner) _singleSpinnerComposite.getChildren()[0])
						.getSelection() - 1);
				break;

			case 2:
				value = (((Spinner) _singleSpinnerComposite.getChildren()[0])
						.getSelection() + 1) + ":";
				break;

			case 3:
				value = ":"	+ ((Spinner) _singleSpinnerComposite.getChildren()[0])
						.getSelection();
				break;

			case 4:
				value = ((Spinner) _singleSpinnerComposite.getChildren()[0])
						.getSelection()	+ ":";
				break;
			}
			break;

		case TWO_SPINNER:
			switch (typeSelection) {
			case 5:
				value = (((Spinner) _twoSpinnerComposite.getChildren()[0])
						.getSelection() - 1)
						+ ":"
						+ (((Spinner) _twoSpinnerComposite.getChildren()[2])
								.getSelection() - 1);
				break;

			case 6:
				value = ((Spinner) _twoSpinnerComposite.getChildren()[0])
						.getSelection()
						+ ":"
						+ ((Spinner) _twoSpinnerComposite.getChildren()[2])
								.getSelection();
				break;
			}
			break;

		case SINGLE_TIMESTAMP:
			DateTime date = (DateTime) _singleTimeStampComposite.getChildren()[0];
			DateTime time = (DateTime) _singleTimeStampComposite.getChildren()[1];
			long timeStamp = getTimeStampVal(date, time);
			timeStamp = (timeStamp / 1000000000) * 1000000000;

			switch (typeSelection) {
			case 0:
				value = String.valueOf(timeStamp);
				break;

			case 1:
				value = ":" + (timeStamp - 1000000000);
				break;

			case 2:
				value = (timeStamp + 1000000000) + ":";
				break;

			case 3:
				value = ":" + timeStamp;
				break;

			case 4:
				value = timeStamp + ":";
				break;
			}
			break;

		case TWO_TIMESTAMP:
			DateTime date1 = (DateTime) _twoTimeStampComposite.getChildren()[0];
			DateTime time1 = (DateTime) _twoTimeStampComposite.getChildren()[1];
			long timeStamp1 = getTimeStampVal(date1, time1);
			timeStamp1 = (timeStamp1 / 1000000000) * 1000000000;

			DateTime date2 = (DateTime) _twoTimeStampComposite.getChildren()[3];
			DateTime time2 = (DateTime) _twoTimeStampComposite.getChildren()[4];
			long timeStamp2 = getTimeStampVal(date2, time2);
			timeStamp2 = (timeStamp2 / 1000000000) * 1000000000;

			switch (typeSelection) {
			case 5:
				value = (timeStamp1 + 1000000000) + ":"
						+ (timeStamp2 - 1000000000);
				break;

			case 6:
				value = timeStamp1 + ":" + timeStamp2;
				break;
			}
			break;

		case TEXT:
			value = ((Text) _textComposite.getChildren()[0]).getText();
			break;
		}

		return value;
	}

	/**
	 * Selects the composite for the stack layout based on the type.
	 * 
	 * @param type
	 *            the type for the composite
	 */
	public void select(int type) {
		_type = type;

		switch (type) {

		case SINGLE_SPINNER:
			((StackLayout) getLayout()).topControl = _singleSpinnerComposite;
			break;

		case TWO_SPINNER:
			((StackLayout) getLayout()).topControl = _twoSpinnerComposite;
			break;

		case TEXT:
			((StackLayout) getLayout()).topControl = _textComposite;
			break;

		case SINGLE_TIMESTAMP:
			((StackLayout) getLayout()).topControl = _singleTimeStampComposite;
			break;

		case TWO_TIMESTAMP:
			((StackLayout) getLayout()).topControl = _twoTimeStampComposite;
			break;
		}

		layout();
	}

	/**
	 * Returns the timestamp value in nano seconds.
	 * 
	 * @param date
	 *            the date part of the timestamp
	 * @param time
	 *            the time part of the timestamp
	 * @return timestamp
	 */
	private long getTimeStampVal(DateTime date, DateTime time) {
		Calendar calendar = Calendar.getInstance();
		calendar.set(date.getYear(), date.getMonth(), date.getDay(), time
				.getHours(), time.getMinutes(), time.getSeconds());

		return calendar.getTimeInMillis() * 1000000;
	}
}
