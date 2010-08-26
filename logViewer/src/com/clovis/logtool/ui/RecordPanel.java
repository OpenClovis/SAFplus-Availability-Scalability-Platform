/**
 * 
 */
package com.clovis.logtool.ui;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

import com.clovis.logtool.record.manager.RecordConfiguration;
import com.clovis.logtool.record.manager.RecordManager;
import com.clovis.logtool.ui.viewer.RecordViewer;

/**
 * Represents the Panel to show the Records.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class RecordPanel extends Composite {

	/**
	 * Record Manager Instance.
	 */
	private RecordManager _recordManager;

	/**
	 * Record Viewer Instance.
	 */
	private RecordViewer _recordViewer;

	/**
	 * Creates the instance of this class.
	 * 
	 * @param parent
	 *            the parent Composite
	 */
	public RecordPanel(Composite parent, RecordConfiguration recordConfiguration) {
		super(parent, SWT.FLAT);

		Class clazz;
		try {
			clazz = Class
					.forName("com.clovis.logtool.record.manager.LogRecordManager");
			Constructor constructor = clazz.getConstructor(new Class[]{RecordConfiguration.class});
			_recordManager = (RecordManager) constructor.newInstance(new Object[]{recordConfiguration});
		} catch (ClassNotFoundException e) {
			System.out.println("Class could not be loaded " + e.getMessage());
			System.exit(-1);
		} catch (InstantiationException e) {
			System.out.println("Class could not be loaded " + e.getMessage());
			System.exit(-1);
		} catch (IllegalAccessException e) {
			System.out.println("Class could not be loaded " + e.getMessage());
			System.exit(-1);
		} catch (SecurityException e) {
			System.exit(-1);
		} catch (NoSuchMethodException e) {
			System.exit(-1);
		} catch (IllegalArgumentException e) {
			System.exit(-1);
		} catch (InvocationTargetException e) {
			System.exit(-1);
		}

		createControls();
	}

	/**
	 * Creates the controls for the record panel.
	 */
	private void createControls() {
		setLayout(new GridLayout());
		_recordViewer = new RecordViewer(this);
	}

	/**
	 * Sets the input data for the record viewer.
	 * 
	 * @param recordBatch
	 *            the Record Batch to be set as input
	 */
	public void setRecordViewerData(List recordBatch) {
		if (recordBatch != null && recordBatch.size() == 0) {
			return;
		}
		_recordViewer.setInput(recordBatch);
		_recordViewer.getTable().setSortColumn(null);
	}

	/**
	 * Updates the Record Viewer Data.
	 * 
	 * @param nextFlag
	 *            the flag to specify the fetch direction
	 */
	public void updateRecordViewerData(boolean nextFlag) {
		setRecordViewerData(_recordManager.getUIRecordBatch(nextFlag));
	}

	/**
	 * Returns the Record Manager.
	 * 
	 * @return the Record Manager
	 */
	public RecordManager getRecordManager() {
		return _recordManager;
	}

	/**
	 * Returns the Record Viewer.
	 * 
	 * @return the Record Viewer
	 */
	public RecordViewer getRecordViewer() {
		return _recordViewer;
	}
}
