/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/list/SelectionListView.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.list;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.Group;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.table.TableUI;
/**
 * @author shubhada
 * List View which shows the Set of objects in the form of list
 * from which the user can choose the objects which he wants.
 */
public class SelectionListView extends Composite
{
    protected final ClassLoader _classLoader;
    protected final List        _selList;
    protected final List        _originalList;
    protected List              _selNamesList = new Vector();
    protected EClass            _eClass;
    protected HashMap           _nameObjectMap = new HashMap();
    protected HashMap           _selNameObjectMap = new HashMap();
    private static final String LIST_ECORE_FILE_NAME = "selectionList.ecore";
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    private TableUI _tableViewer;

    /**
     * Constructor.
     * @param parent Parent Composite
     * @param style  SWT Style
     * @param originalList  - Original List
     * @param selList - Selection List
     * @param loader ClassLoader can be null
     * @param representation - Representation of Selection List
     * 0 - will show Selection List with check boxes.
     * 1 - will show Selection List with 2 List Views where user can
     * choose the from one list and put it in to another list
     */
    public SelectionListView(Composite parent, int style, List selList,
            List originalList, ClassLoader loader, boolean representation)
    {
        super(parent, style);
        _classLoader  = loader;
        _selList      = selList;
        _originalList = originalList;
        readEClass();
        populateListView(parent, representation);
    }
    /**
     *
     */
    private void readEClass()
    {
        try {
            URL mapURL = UtilsPlugin.getDefault().find(new Path("model"
                    + File.separator + LIST_ECORE_FILE_NAME));
            File ecoreFile = new Path(Platform.resolve(mapURL).getPath())
                    .toFile();
            EPackage listPackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            _eClass = (EClass) listPackage.getEClassifier("List");
        } catch (IOException ex) {
            LOG.error("Selection list ecore file cannot be read", ex);
        }
    }
    /**
     * Create Temporary EObjects to be shown in TableUI.
     * @return list of temp EObjects.
     */
    private NotifyingList createEObjects()
    {
        String name = null;
        for (int i = 0; i < _selList.size(); i++) {
            Object obj = _selList.get(i);
            if (obj instanceof EObject) {
                name = EcoreUtils.getName((EObject) obj);
                if (name == null) {
                    name = ((EObject) obj).eClass().getName();
                }
                _selNamesList.add(name);
                _selNameObjectMap.put(name, obj);
            } else {
            	_selNamesList.add(obj);
            	_selNameObjectMap.put(obj, obj);
            }
        }
        NotifyingList inputList = new ClovisNotifyingListImpl();
        for (int i = 0; i < _originalList.size(); i++) {
            Object obj   = _originalList.get(i);
            EObject eobj = EcoreUtils.createEObject(_eClass, true);

            if (obj instanceof EObject) {
                name = EcoreUtils.getName((EObject) obj);
                if (name == null) {
                    name = ((EObject) obj).eClass().getName();
                }
                // In case of EObject check for selected name List
                if (_selNamesList.contains(name)) {
                    EcoreUtils.setValue(eobj, "isSelected", "true");
                } else {
                    EcoreUtils.setValue(eobj, "isSelected", "false");
                }

            } else {
                name = obj.toString();
                if (_selList.contains(obj)) {
                    EcoreUtils.setValue(eobj, "isSelected", "true");
                } else {
                    EcoreUtils.setValue(eobj, "isSelected", "false");
                }
            }
            _nameObjectMap.put(name, obj);
            EcoreUtils.setValue(eobj, "Value", name);
            inputList.add(eobj);
        }
        //This code is to take care of any invalid entry in the selection list
        //which does not exist in the original list
        Iterator iterator = _selNameObjectMap.keySet().iterator();
        while (iterator.hasNext()) {
        	String selName = (String) iterator.next();
        	Object selObj = _selNameObjectMap.get(selName);
        	if (!_nameObjectMap.containsKey(selName)) {
        		_selList.remove(selObj);
        	}
        }
        addDisposeListener(new Listener(inputList));
        return inputList;
    }
    /**
     *@param parent -Parent Composite
     *@param rep - Representation
     */
    private void populateListView(Composite parent, boolean rep)
    {
        if (rep) {
            setLayout(new GridLayout());
            int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL
                | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
            Table table         = new Table(this, style);
            GridData gridData1 = new GridData();
            gridData1.horizontalAlignment = GridData.FILL;
            gridData1.grabExcessHorizontalSpace = true;
            gridData1.grabExcessVerticalSpace = true;
            gridData1.verticalAlignment = GridData.FILL;
            gridData1.heightHint = getDisplay().getClientArea().height / 10;
            table.setLayoutData(gridData1);
            ClassLoader loader  = getClass().getClassLoader();
            _tableViewer = new TableUI(table, _eClass, loader);
            table.setLinesVisible(true);
            _tableViewer.setInput(createEObjects());
        } else {
            GridLayout glayOut = new GridLayout();
            glayOut.numColumns = 3;
            setLayout(glayOut);
            int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL
                | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
            Group origGroup = new Group(this, SWT.SHADOW_OUT);
            origGroup.setText("Available Values");
            origGroup.setLayout(new GridLayout());
            GridData origgd = new GridData(GridData.FILL_BOTH);
            origgd.grabExcessHorizontalSpace = true;
            origGroup.setLayoutData(origgd);
            org.eclipse.swt.widgets.List origList =
                new org.eclipse.swt.widgets.List(origGroup, style);
            final ListView origViewer = new ListView(origList);

            origList.setLayoutData(new GridData(GridData.FILL_BOTH));
            origViewer.setInput(_originalList);

            Composite buttonComposite = new Composite(this, SWT.NONE);
            buttonComposite.setLayoutData(new GridData());
            buttonComposite.setLayout(new GridLayout());

            GridData gridData2 = new GridData(GridData.FILL_BOTH);
            Button selectButton = new Button(buttonComposite, SWT.PUSH);
            selectButton.setText(">>");
            selectButton.setLayoutData(gridData2);

            GridData gridData3 = new GridData(GridData.FILL_BOTH);
            Button unselectButton = new Button(buttonComposite, SWT.PUSH);
            unselectButton.setText("<<");
            unselectButton.setLayoutData(gridData3);

            Group attrGroup = new Group(this, SWT.SHADOW_OUT);
            attrGroup.setText("Selected Values");
            attrGroup.setLayout(new GridLayout());
            GridData gd = new GridData(GridData.FILL_BOTH);
            attrGroup.setLayoutData(gd);
            org.eclipse.swt.widgets.List selectionList =
                new org.eclipse.swt.widgets.List(attrGroup, style);
            final ListView selViewer = new ListView(selectionList);
            GridData gridData4 = new GridData(GridData.FILL_BOTH);
            selectionList.setLayoutData(gridData4);
            selViewer.setInput(_selList);

            //Adding Selection listener on button
            unselectButton.addSelectionListener(new SelectionAdapter() {
                public void widgetSelected(SelectionEvent e)
                {
                    StructuredSelection sel =
                        (StructuredSelection) selViewer.getSelection();
                    List list = sel.toList();
                    ((NotifyingList) origViewer.getInput()).addAll(list);
                    ((NotifyingList) selViewer.getInput()).removeAll(list);
                }
            });
            //Adding Selection listener on button
            selectButton.addSelectionListener(new SelectionAdapter() {
                public void widgetSelected(SelectionEvent e)
                {
                    StructuredSelection sel =
                        (StructuredSelection) origViewer.getSelection();
                    List list = sel.toList();
                    ((NotifyingList) selViewer.getInput()).addAll(list);
                    ((NotifyingList) origViewer.getInput()).removeAll(list);
                }
            });
        }
    }
    /**
     * Updates selection list based on the user input.
     * @author nadeem
     */
    private class Listener extends AdapterImpl
        implements DisposeListener
    {
        private EObject _eObjects[];
        /**
         * Constructor
         * @param list List of EObject shown
         */
        public Listener(List list)
        {
            _eObjects = (EObject[]) list.toArray(new EObject[0]);
            for (int i = 0; i < _eObjects.length; i++) {
                EcoreUtils.addListener(_eObjects[i], this, 1);
            }
        }
        /**
         * Release Listener
         * @param e Event
         */
        public void widgetDisposed(DisposeEvent e)
        {
            SelectionListView.this.removeDisposeListener(this);
            for (int i = 0; i < _eObjects.length; i++) {
                EcoreUtils.removeListener(_eObjects[i], this, 1);
            }
        }
        /**
         * Notification Callback.
         * @param msg Change Event
         */
        public void notifyChanged(Notification msg)
        {
        	if (!msg.isTouch() && msg.getEventType() == Notification.SET) {
                EObject obj   = (EObject) msg.getNotifier();
                boolean isSel = ((Boolean)
                    EcoreUtils.getValue(obj, "isSelected")).booleanValue();
                Object valStr = EcoreUtils.getValue(obj, "Value");
                Object value  = _nameObjectMap.get(valStr);
                if (isSel) {
                    if (value instanceof EObject) {
                        if (!(_selNamesList.contains(valStr))) {
                            _selNameObjectMap.put(valStr, value);
                            _selList.add(value);
                        }
                    } else {
                        if (!(_selList.contains(value))) {
                            _selList.add(value);
                            _selNameObjectMap.put(valStr, value);
                        }
                    }
                } else {
                    if (value instanceof EObject) {
                      EObject selObj = (EObject) _selNameObjectMap.get(valStr);
                      _selList.remove(selObj);
                      _selNameObjectMap.remove(selObj);
                      String name = EcoreUtils.getName((EObject) value);
                      if (name == null) {
                          name = ((EObject) value).eClass().getName();
                      }
                      _selNamesList.remove(name);
                    } else {
                    _selList.remove(value);
                    _selNameObjectMap.remove(value);
                    _selNamesList.remove(value);
                    }
                }
                LOG.debug(valStr + "SelectionListView: selection changed: " + isSel);
            }
        }
    }
    /**
     * Returns TableViewer
     * @return TableUI
     */
    public TableUI getTableViewer() {
    	return _tableViewer;
    }
    /**
     * Returns Selection List
     * @return List
     */    
    public List getSelectionList() {
    	return _selList;
    }
}
