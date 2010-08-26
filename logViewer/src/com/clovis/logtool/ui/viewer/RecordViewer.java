package com.clovis.logtool.ui.viewer;

import java.util.List;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;

import com.clovis.logtool.utils.LogUtils;
import com.clovis.logtool.utils.UIColumn;

/**
 * Viewer for viewing the Log Records. It extends the TableViewer class to show
 * the record in the table form.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class RecordViewer extends TableViewer {

	/**
	 * Constructs the log viewer. Assigns content provider, label provider and
	 * sorter for the same.
	 * 
	 * @param parent
	 *            the parent composite for this log viewer
	 */
	public RecordViewer(Composite parent) {
		super(parent);

		try {
			setContentProvider((RecordViewerContentProvider) Class.forName(
					"com.clovis.logtool.ui.viewer.RecordViewerContentProvider")
					.newInstance());

			setLabelProvider((RecordViewerLabelProvider) Class.forName(
					"com.clovis.logtool.ui.viewer.RecordViewerLabelProvider")
					.newInstance());

			setSorter((RecordViewerSorter) Class.forName(
					"com.clovis.logtool.ui.viewer.RecordViewerSorter")
					.newInstance()); 
		} catch (InstantiationException e) {
                        System.err.println("Error: Encountered problem " + e.getMessage());
                        System.exit(-1);
		} catch (IllegalAccessException e) {
                        System.err.println("Error: Encountered problem " + e.getMessage());
                        System.exit(-1);
		} catch (ClassNotFoundException e) {
                        System.err.println("Error: Encountered problem " + e.getMessage());
                        System.exit(-1);
		}

		setUpTable();
	}

	/**
	 * Sets up the table for the table viewer.
	 */
	private void setUpTable() {
		Table table = getTable();
		table.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		setUpColumns();

		table.setHeaderVisible(true);
		table.setLinesVisible(true);
	}

	/**
	 * Sets up the columns for the table.
	 */
	private void setUpColumns() {
		GC gc = new GC(getTable());
		int avgCharWidth = gc.getFontMetrics().getAverageCharWidth();
		gc.dispose();

		List<UIColumn> columnList = LogUtils.getUIColumns();
		for (int i = 0; i < columnList.size(); i++) {

			UIColumn column = columnList.get(i);
			if (column.isShowFlag()) {
				createColumn(column, avgCharWidth);
			}
		}
	}

	/**
	 * Creates a column for the table with given information.
	 * @param avgCharWidth 
	 * 
	 * @param index
	 *            the index of the column within table
	 * @param name
	 *            the header for the column
	 */
	private void createColumn(final UIColumn uiColumn, int avgCharWidth) {
		final Table table = getTable();
		final TableColumn column = new TableColumn(table, SWT.LEFT);

		column.setText(uiColumn.getName());
		column.setMoveable(true);
		column.setWidth(uiColumn.getWidth() * avgCharWidth);

		column.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				RecordViewerSorter sorter = (RecordViewerSorter) getSorter();

				sorter.setSort(true);
				sorter.doSort(uiColumn.getIndex());
				refresh();
				sorter.setSort(false);

				table.setSortColumn(column);
				table.setSortDirection(sorter.getDirection());
			}
		});
	}
}
