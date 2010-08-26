/**
 * 
 */
package com.clovis.cw.editor.ca.dialog;

import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * Class for selecting the ASP SUs.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CpmAspSUListDialog extends TitleAreaDialog {

	private List _nodesList = null;
	
	private List _selList = null;

    private EObject _nodeObj = null;
    
	private String _cpmType = null;

	private EReference _aspSuRef = null;

	private CheckboxTableViewer _coreCompsViewer, _managCompsViewer, _misCompsViewer;
	
	private Button _corCheck, _managCheck, _misCheck; 
	/**
	 * Constructor.
	 * 
	 * @param parentShell
	 * @param title
	 * @param selList
	 * @param originalList
	 * @param cpmtype
	 * @param aspSuRef
	 */
	public CpmAspSUListDialog(Shell parentShell, EObject nodeObj, List modelList) {
		super(parentShell);
		_nodesList = modelList;
		_nodeObj = nodeObj;
		EReference aspSusRef = (EReference)nodeObj.eClass().
		getEStructuralFeature("aspServiceUnits");
		_aspSuRef = (EReference) aspSusRef.getEReferenceType().getEStructuralFeature("aspServiceUnit");
		EObject susObj = (EObject) _nodeObj.eGet(aspSusRef);
		if (susObj == null) {
			susObj = EcoreUtils.createEObject(aspSusRef.
					getEReferenceType(), true);
			nodeObj.eSet(aspSusRef, susObj);
		}
		_selList = (List) susObj.eGet(_aspSuRef);
		_cpmType = EcoreUtils.getValue(_nodeObj, "cpmType").toString();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.TitleAreaDialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		composite.setLayoutData(gridData);
		composite.setLayout(new GridLayout());

		LabelProvider labelProvider = new LabelProvider();
		_corCheck = new Button(composite, SWT.CHECK | SWT.BORDER);
		_corCheck.setText("Core ASP Components:");
		final String [] coreComps = new String[] {"gmsSU", "eventSU", "ckptSU"};
		_corCheck.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				_corCheck.setSelection(true);
			}
		});
		_corCheck.setSelection(true);
		_coreCompsViewer = CheckboxTableViewer.newCheckList(composite, SWT.BORDER);
		_coreCompsViewer.getControl().setLayoutData(
				new GridData(SWT.FILL, SWT.FILL, true, true));
		_coreCompsViewer.setLabelProvider(labelProvider);
		_coreCompsViewer.addCheckStateListener(new ICheckStateListener() {
			public void checkStateChanged(CheckStateChangedEvent event) {
				_coreCompsViewer.setChecked(event.getElement(), true);
			}
		});
		_coreCompsViewer.add(coreComps);
		setChecked(_coreCompsViewer, coreComps, true);
				
		_managCheck = new Button(composite, SWT.CHECK | SWT.BORDER);
		_managCheck.setText("Manageability ASP Components:");
		final String managComps[] = new String[] {"corSU", "txnSU", "oampSU"};
		final String payLoadManagComps[] = new String[] {"txnSU", "oampSU"};
		_managCheck.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				if(_managCheck.getSelection()) {
					if(_cpmType.equals("GLOBAL")) {
						setChecked(_managCompsViewer, managComps, true);
					} else {
						setChecked(_managCompsViewer, payLoadManagComps, true);
					}
				} else {
					setChecked(_managCompsViewer, managComps, false);
				}
			}
		});
		_managCompsViewer = CheckboxTableViewer.newCheckList(composite, SWT.BORDER);
		_managCompsViewer.getControl().setLayoutData(
				new GridData(SWT.FILL, SWT.FILL, true, true));
		_managCompsViewer.setLabelProvider(labelProvider);
		_managCompsViewer.addCheckStateListener(new ICheckStateListener() {
			public void checkStateChanged(CheckStateChangedEvent event) {
				if(_managCheck.getSelection()) {
					String selection = event.getElement().toString();
					if(_cpmType.equals("GLOBAL")) {
						if(selection.equals("oampSU")) {
							_managCompsViewer.setChecked(event.getElement(), event.getChecked());
						} else {
							_managCompsViewer.setChecked(event.getElement(), true);
						}
						if(Arrays.asList(_managCompsViewer.getCheckedElements()).contains("oampSU")){
							setChecked(_managCompsViewer, new String[]{"corSU", "txnSU"}, true);
						} else if(Arrays.asList(_managCompsViewer.getCheckedElements()).contains("corSU")){
							_managCompsViewer.setChecked("txnSU", true);
						}
					} else {
						if(event.getElement().toString().equals("corSU"))
							_managCompsViewer.setChecked(event.getElement(), false);
						else
							_managCompsViewer.setChecked(event.getElement(), event.getChecked());
						if(Arrays.asList(_managCompsViewer.getCheckedElements()).contains("oampSU")){
							_managCompsViewer.setChecked("txnSU", true);
							return;
						}
						if(_managCompsViewer.getCheckedElements().length == 0)
							_managCheck.setSelection(false);
					}
				} else {
					_managCompsViewer.setChecked(event.getElement(), false);
				}
			}
		});
		_managCompsViewer.add(managComps);
		setChecked(_managCompsViewer, getStringArray(_selList),true);
		if(_managCompsViewer.getCheckedElements().length > 0) {
			_managCheck.setSelection(true);
		}
		
		//If corSU is selected for payload nodes
		if(_cpmType.equals("LOCAL")) {
			_managCompsViewer.setChecked("corSU", false);
			_managCompsViewer.setGrayed("corSU", true);
		}
		
		_misCheck = new Button(composite, SWT.CHECK | SWT.BORDER);
		_misCheck.setText("Miscellaneous ASP Components:");
		final String misComps [] = new String[] {"logSU", "nameSU", "cmSU", "msgSU"};
		final String payLoadMisComps [] = new String[] {"logSU", "nameSU", "msgSU"};
		_misCheck.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				if(_misCheck.getSelection()) {
					if(_cpmType.equals("GLOBAL")) {
						setChecked(_misCompsViewer, misComps, true);
					} else {
						setChecked(_misCompsViewer, payLoadMisComps, true);
					}
				} else {
					setChecked(_misCompsViewer, misComps, false);
				}
			}
		});
		_misCompsViewer = CheckboxTableViewer.newCheckList(composite, SWT.BORDER);
		_misCompsViewer.getControl().setLayoutData(
				new GridData(SWT.FILL, SWT.FILL, true, true));
		_misCompsViewer.setLabelProvider(labelProvider);
		_misCompsViewer.addCheckStateListener(new ICheckStateListener() {
			public void checkStateChanged(CheckStateChangedEvent event) {
				if(_misCheck.getSelection()) {
					String selection = event.getElement().toString();
					if(_cpmType.equals("LOCAL")) {
						if(selection.equals("cmSU")) {
							_misCompsViewer.setChecked(event.getElement(), false);
						} else {
							_misCompsViewer.setChecked(event.getElement(), event.getChecked());
						}
					} else {
						_misCompsViewer.setChecked(event.getElement(), event.getChecked());
					}
					if(_misCompsViewer.getCheckedElements().length == 0)
						_misCheck.setSelection(false);
				} else {
					_misCompsViewer.setChecked(event.getElement(), false);
				}
			}
		});
		_misCompsViewer.add(misComps);
		setChecked(_misCompsViewer, getStringArray(_selList),true);
		if(_misCompsViewer.getCheckedElements().length > 0) {
			_misCheck.setSelection(true);
		}
		
		//If cmSU is selected for payload nodes
		if(_cpmType.equals("LOCAL")) {
			_misCompsViewer.setChecked("cmSU", false);
			_misCompsViewer.setGrayed("cmSU", true);
		}
		
		setTitle("Select ASP Service Units From List");
		getShell().setText("Select ASP Service Units");

		return composite;
	}

	/**
	 * Returns the String array for the Eobject name in the list.
	 * 
	 * @param list
	 * @return
	 */
	private String[] getStringArray(List list) {

		String str[] = new String[list.size()];
		int i = 0;

		Iterator itr = list.iterator();
		while (itr.hasNext()) {
			str[i++] = EcoreUtils.getName((EObject) itr.next());
		}

		return str;
	}

	/**
	 * Sets the selection state for the given elements in this viewer.
	 * @param viewer
	 * @param elements
	 * @param state
	 */
	protected void setChecked(CheckboxTableViewer viewer, String[] elements, boolean state){
		for(int i = 0; i < elements.length; i++) {
			viewer.setChecked(elements[i], state);
		}
	}
	
	/**
	 * Sets the grayed state for the given elements in this viewer.
	 * @param viewer
	 * @param elements
	 * @param state
	 */
	protected void setGrayed(CheckboxTableViewer viewer, String[] elements, boolean state){
		for(int i = 0; i < elements.length; i++) {
			viewer.setGrayed(elements[i], state);
		}
	}
	
	/**
	 * Upadtes the list with selected elements
	 * @param viewer
	 */
    private void addSelectedElementsInList(CheckboxTableViewer viewer){
    	Object elements[] = viewer.getCheckedElements();
		for(int i = 0; i < elements.length; i++) {
			EObject eObj = EcoreUtils.createEObject(_aspSuRef
					.getEReferenceType(), true);
			EcoreUtils.setValue(eObj, "name", elements[i].toString());
			_selList.add(eObj);
		}
    }
    
   	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		_selList.clear();
		addSelectedElementsInList(_coreCompsViewer);
		addSelectedElementsInList(_managCompsViewer);
		addSelectedElementsInList(_misCompsViewer);
		if(Arrays.asList(_managCompsViewer.getCheckedElements()).contains("oampSU")) {
			for(int i = 0; i < _nodesList.size(); i++) {
				EObject node = (EObject) _nodesList.get(i);
				String cpmType = EcoreUtils.getValue(node, "cpmType").toString();
				if(node != _nodeObj && cpmType.equals("GLOBAL")) {
					EReference aspSusRef = (EReference)node.eClass().
					getEStructuralFeature("aspServiceUnits");
					EReference aspSuRef = (EReference) aspSusRef.getEReferenceType().getEStructuralFeature("aspServiceUnit");
					EObject susObj = (EObject) node.eGet(aspSusRef);
					if (susObj == null) {
						susObj = EcoreUtils.createEObject(aspSusRef.
								getEReferenceType(), true);
						node.eSet(aspSusRef, susObj);
					}
					List selList = (List) susObj.eGet(aspSuRef);
					boolean corSelected = false;
					boolean txnSelected = false;
					for(int j = 0; j < selList.size(); j++) {
						EObject su = (EObject) selList.get(j);
						if(EcoreUtils.getValue(su, "name").equals("corSU"))
							corSelected = true;
						if(EcoreUtils.getValue(su, "name").equals("txnSU"))
							txnSelected = true;
					}
					if(!corSelected) {
						EObject eObj1 = EcoreUtils.createEObject(aspSuRef
								.getEReferenceType(), true);
						EcoreUtils.setValue(eObj1, "name", "corSU");
						selList.add(eObj1);
						if(!txnSelected) {
							EObject eObj2 = EcoreUtils.createEObject(aspSuRef
								.getEReferenceType(), true);
							EcoreUtils.setValue(eObj2, "name", "txnSU");
							selList.add(eObj2);
						}
					}
				}
			}
		}
		super.okPressed();
	}
	/**
	 * Label Provider
	 * @author Pushparaj
	 *
	 */
	class LabelProvider implements ITableLabelProvider {
		public Image getColumnImage(Object element, int columnIndex) {
			return null;
		}

		public String getColumnText(Object element, int columnIndex) {
			return (String) element;
		}

		public void addListener(ILabelProviderListener listener) {
		}

		public void dispose() {
		}

		public boolean isLabelProperty(Object element, String property) {
			return false;
		}

		public void removeListener(ILabelProviderListener listener) {
		}
	}
}
