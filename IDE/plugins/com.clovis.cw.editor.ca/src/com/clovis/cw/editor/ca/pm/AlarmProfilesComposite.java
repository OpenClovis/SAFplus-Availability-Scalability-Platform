/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.ListObjectListener;

/**
 * Composite for alarm profile selection.
 * 
 * @author Suraj Rajyaguru
 */
public class AlarmProfilesComposite extends Composite {

	private PMEditor _editor;;

	private Model _viewModel;

	private CheckboxTableViewer _alarmProfilesViewer;
	
	private Map<String, String> _alarmThresholdMap = new HashMap<String, String>();

	/**
	 * Constructor.
	 * 
	 * @param parent
	 * @param editor
	 */
	public AlarmProfilesComposite(Composite parent, PMEditor editor) {
		super(parent, SWT.NONE);
		_editor = editor;
		createControls();
	}

	/**
	 * Creates the child controls.
	 */
	private void createControls() {
		GridLayout layout = new GridLayout();
		layout.marginWidth = layout.marginHeight = 0;
		setLayout(layout);
		setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		Group alarmProfilesGroup = new Group(this, SWT.None);
		alarmProfilesGroup.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		alarmProfilesGroup.setText("Alarm Profiles");
		alarmProfilesGroup.setLayout(new GridLayout());
		alarmProfilesGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true,
				true));

		_viewModel = _editor.getProjectDataModel().getAlarmProfiles()
				.getViewModel();
		EObject alarmInfoObj = (EObject) _viewModel.getEList().get(0);
		List<EObject> alarmProfileList = (List<EObject>) EcoreUtils.getValue(
				alarmInfoObj, "AlarmProfile");

		int style = SWT.MULTI| SWT.H_SCROLL | SWT.V_SCROLL
				| SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.CHECK;
		Table table = new Table(alarmProfilesGroup, style);
		TableColumn tc0 = new TableColumn(table,SWT.NONE);
        tc0.setText("Alarm Profile Name");
        tc0.pack();
        TableColumn tc1 = new TableColumn(table,SWT.NONE);
        tc1.setText("Created Threshold Value");
        tc1.pack();
		table.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		_alarmProfilesViewer = new CheckboxTableViewer(table);
		table.setLinesVisible(true);
        table.setHeaderVisible(true);
		_alarmProfilesViewer
				.setContentProvider(new AlarmProfilesViewerContentProvider());
		_alarmProfilesViewer
				.setLabelProvider(new AlarmProfilesViewerLabelProvider());
		_alarmProfilesViewer.setInput(alarmProfileList);
		EcoreUtils.addListener(alarmProfileList, new ListObjectListener(
				_alarmProfilesViewer), -1);

		_alarmProfilesViewer.addCheckStateListener(new ICheckStateListener() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.jface.viewers.ICheckStateListener#checkStateChanged(org.eclipse.jface.viewers.CheckStateChangedEvent)
			 */
			public void checkStateChanged(CheckStateChangedEvent event) {
				_alarmProfilesViewer.setAllChecked(false);
				_alarmProfilesViewer.setChecked(event.getElement(), true);
			}
		});
	}

	/**
	 * Returns checked items.
	 * 
	 * @return
	 */
	public Object[] getCheckedItems() {
		return _alarmProfilesViewer.getCheckedElements();
	}

	/**
	 * Sets the given items checked in the viewer.
	 * 
	 * @param elements
	 */
	public void setCheckedItems(List elements) {
		List<EObject> alarmProfileList = (List<EObject>) _alarmProfilesViewer.getInput();
		EObject alarmObj;
		
		for (int i=0 ; i<alarmProfileList.size() ; i++) {
			alarmObj = alarmProfileList.get(i);

			if(elements.contains(EcoreUtils.getValue(alarmObj, "alarmID"))) {
				_alarmProfilesViewer.setChecked(alarmObj, true);
			} else {
				_alarmProfilesViewer.setChecked(alarmObj, false);
			}
		}
	}
	/**
	 * Select the items in the viewer
	 * @param names
	 * @param elements
	 */
	public void setSelection(List names, List elements) {
		List<EObject> alarmProfileList = (List<EObject>) _alarmProfilesViewer.getInput();
		
		List<EObject> list = new ArrayList<EObject>();
		for (int i=0 ; i<alarmProfileList.size() ; i++) {
			EObject alarmObj = alarmProfileList.get(i);
			if(names.contains(EcoreUtils.getValue(alarmObj, "alarmID"))) {
				list.add(alarmObj);
			}
		}
		Object selection[] = list.toArray();
		_alarmThresholdMap.clear();
		updateThresholdValues(elements);
		_alarmProfilesViewer.setInput(alarmProfileList);
		_alarmProfilesViewer.setSelection(new StructuredSelection(selection));
	}
	/**
	 * Updates the alarm threshold value maps
	 * @param elements
	 */
    private void updateThresholdValues(List elements) {
		for (int i = 0; i < elements.size(); i++) {
			EObject alarmObj = (EObject)elements.get(i);
			String name = (String) EcoreUtils.getValue(alarmObj, "alarmID");
			String thresholdValue = EcoreUtils.getValue(alarmObj, "thresholdValue").toString();
			_alarmThresholdMap.put(name, thresholdValue);
		}
	}
	/**
	 * Content provider for the alarm profiles viewer.
	 * 
	 * @author Suraj Rajyaguru
	 * 
	 */
	class AlarmProfilesViewerContentProvider implements
			IStructuredContentProvider {

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
		 */
		public Object[] getElements(Object inputElement) {
			return ((List) inputElement).toArray();
		}

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
		 */
		public void dispose() {
		}

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer,
		 *      java.lang.Object, java.lang.Object)
		 */
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		}
	}

	/**
	 * Label provider for the alarm profiles viewer.
	 * 
	 * @author Suraj Rajyaguru
	 * 
	 */
	class AlarmProfilesViewerLabelProvider extends LabelProvider implements
			ITableLabelProvider {

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object,
		 *      int)
		 */
		public Image getColumnImage(Object element, int columnIndex) {
			return null;
		}

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object,
		 *      int)
		 */
		public String getColumnText(Object element, int columnIndex) {
			if(columnIndex == 0)
				return EcoreUtils.getValue((EObject) element, "alarmID").toString();
			else {
				String thresholdValue = _alarmThresholdMap.get(EcoreUtils.getValue((EObject) element, "alarmID").toString());
				return thresholdValue != null ? thresholdValue: "";
			}
		}
	}
}
