/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/TableUI.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.common.notify.NotifyingList;

import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.EnvironmentNotifierImpl;
import com.clovis.common.utils.ui.ListObjectListener;
import com.clovis.common.utils.ui.factory.CellEditorFactory;
/**
 * Provides TableViewer for EObjects
 * @author nadeem
 */
public class TableUI extends TableViewer
    implements Environment, ISelectionChangedListener
{
    private EClass                  _eClass;
    private ClassLoader             _classLoader;
    private boolean                 _isReadOnly;
    private ListObjectListener      _listener;
    private List                    _featureList;
    private Environment             _parentEnv;
    private Map                     _propertyValMap = new HashMap();
    private EnvironmentNotifierImpl _notifier = new EnvironmentNotifierImpl();
    
    /**
     * Constructor with RW table.
     * @param table  Table Composite
     * @param eClass EClass to be shown here.
     * @param l      Class Loader to load action classes.
     */
    public TableUI(Table table, EClass eClass, ClassLoader l)
    {
        this(table, eClass, l, false);
    }
    /**
     * Constructor.
     * @param table  Table Composite
     * @param eClass EClass to be shown here.
     * @param loader Class Loader to load action classes.
     * @param isReadOnly true for RO
     */
    public TableUI(Table table, EClass eClass,
                   ClassLoader loader, boolean isReadOnly)
    {
        this(table, eClass, loader, isReadOnly, null);
    }
    /**
     * Constructor.
     * @param table  Table Composite
     * @param eClass EClass to be shown here.
     * @param loader Class Loader to load action classes.
     * @param isReadOnly true for RO
     * @param parentEnv  Parent Environment
     */
    public TableUI(Table table, EClass eClass,
        ClassLoader loader, boolean isReadOnly, Environment parentEnv)
    {
        super(table);
        _eClass      = eClass;
        _classLoader = loader;
        _isReadOnly  = isReadOnly;
        _featureList = EcoreUtils.getFeatureList(_eClass, true);
        _listener    = new ListObjectListener(this);
        _parentEnv   = parentEnv;
        table.addDisposeListener(_listener);
        setUseHashlookup(true);
        setLabelProvider(new RowLabelProvider(this));
        setContentProvider(new TableContentProvider());
        setUpColumns();
        setUpEditors();
        setCellModifier(new RowCellModifier());
        addSelectionChangedListener(this);
    }
    /**
     * Constructor.
     * @param parent  parent Composite
     * @param eClass EClass to be shown here.
     * @param loader Class Loader to load action classes.
     * @param isReadOnly true for RO
     * @param parentEnv  Parent Environment
     */
    public TableUI(Composite parent, int style, EClass eClass,
        ClassLoader loader, boolean isReadOnly, Environment parentEnv)
    {
        super(parent, style);
        _eClass      = eClass;
        _classLoader = loader;
        _isReadOnly  = isReadOnly;
        _featureList = EcoreUtils.getFeatureList(_eClass, true);
        _listener    = new ListObjectListener(this);
        _parentEnv   = parentEnv;
        parent.addDisposeListener(_listener);
        setUseHashlookup(true);
        setLabelProvider(new RowLabelProvider(this));
        setContentProvider(new TableContentProvider());
        setUpColumns();
        setUpEditors();
        setCellModifier(new RowCellModifier());
        addSelectionChangedListener(this);
    }
    /**
     * Returns Notifier for Environment.
     * @return the notifier instance
     */
    public EnvironmentNotifier getNotifier()
    {
        return _notifier;
    }
    /**
     * Returns Parent Environment.
     * @return Parent Environment.
     */
    public Environment getParentEnv()
    {
        return _parentEnv;
    }
    /**
     * Returns list of viewable features
     * @return list of viewable features
     */
    public List getFeatures()
    {
        return _featureList;
    }
    /**
     * Creates column according to _eClass attributes.
     * SubClasses may override this to put custom labels.
     */
    protected void setUpColumns()
    {
        int colCount = _featureList.size();
        String[] colNames         = new String[colCount];
        for (int i = 0; i < colCount; i++) {
            EStructuralFeature efeature =
                (EStructuralFeature) _featureList.get(i);
            colNames[i] = efeature.getName();
            TableColumn column = new TableColumn(getTable(), SWT.NONE);
            column.setText(EcoreUtils.getLabel(efeature));
            column.pack();
        }
        setColumnProperties(colNames);
    }
    /**
     * Sets Up Editors.
     * SubClasses may override this to put custom editors.
     */
    protected void setUpEditors()
    {
        if (_isReadOnly) {
        	setCellEditors(new CellEditor[]{});
        } else {
            int colCount = _featureList.size();
            CellEditor[] cellEditors  = new CellEditor [colCount];
            CellEditorFactory factory = CellEditorFactory.TABLE_INSTANCE;
            for (int i = 0; i < colCount; i++) {
                EStructuralFeature efeature =
                    (EStructuralFeature) _featureList.get(i);
                cellEditors[i] = factory.getEditor(efeature, getTable(), this);
            }
            setCellEditors(cellEditors);
        }
    }
    /**
     * @author shubhada
     * This class listens for selection changes and notifies Environment
     * listeners.
     * @param e SelectionChangedEvent
     */
    public void selectionChanged(SelectionChangedEvent e)
    {
        _notifier.fireEnvironmentChange("selection");
    }
    /**
     * Change in input, update listeners.
     * @param input    new input
     * @param oldInput old Input.
     */
    protected void inputChanged(Object input, Object oldInput)
    {
        NotifyingList newList = (NotifyingList) input;
        NotifyingList oldList = (NotifyingList) oldInput;
        if(newList != oldList) {
        // Add listener to new Input
        if (newList != null) {
            EcoreUtils.addListener(newList, _listener, -1);
        }
        // Remove listener from old input.
        if (oldList != null) {
            EcoreUtils.removeListener(oldList, _listener, -1);
        }
        }
        super.inputChanged(input, oldInput);
    }
    /**
     * Implementation of getValue method
     * @param  property property in the env
     * @return Value for property in env.
     */
    public Object getValue(Object property)
    {
        if (property.toString().equalsIgnoreCase("selection")) {
            return (StructuredSelection) getSelection();
        } else if (property.toString().equalsIgnoreCase("rows")) {
            return getTable().getItems();
        } else if (property.toString().equalsIgnoreCase("model")) {
            return getInput();
        } else if (property.toString().equalsIgnoreCase("table")) {
            return getTable();
        } else if (property.toString().equalsIgnoreCase("shell")) {
            return getTable().getShell();
           } else if (property.toString().equalsIgnoreCase("eclass")) {
            return _eClass;
        } else if (property.toString().equalsIgnoreCase("classloader")) {
            return _classLoader;
        } else if (property.toString().equalsIgnoreCase("tableviewer")) {
            return this;
        } else {
            return _propertyValMap.get(property);
        }
    }
    /**
     * 
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    public void setValue(Object property, Object value)
    {
        _propertyValMap.put(property, value);
    }
}
