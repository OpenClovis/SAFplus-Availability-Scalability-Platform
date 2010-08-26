/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/property/PropertyViewer.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.property;

import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.TableEditor;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionAdapter;


import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.ICellEditorListener;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.CellEditor;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.EnvironmentNotifierImpl;
import com.clovis.common.utils.ui.ListObjectListener;
import com.clovis.common.utils.ui.table.RowCellModifier;
import com.clovis.common.utils.ui.factory.CellEditorFactory;
/**
 * @author nadeem
 *
 * Property Viewer for name-value pairs.
 */
public class PropertyViewer extends Viewer
    implements Environment
{
    private static final int    EDITABLE_COLUMN = 1;
    public  static final String FEATURE_KEY     = "feature";

    private Object                  _input;
    private final Table             _table;
    private final TableEditor       _tableEditor;
    private ClassLoader             _classLoader;
    private ListObjectListener      _listener;
    private CellEditor              _cellEditor;
    private ICellEditorListener     _editorListener;
    private ICellModifier           _cellModifier;
    private TableItem               _tableItem;
    private EnvironmentNotifierImpl _notifier = new EnvironmentNotifierImpl();
    /**
     * Constructor.
     * @param parent Parent Composite
     * @param style  SWT Style.
     * @param loader ClassLoader.
     */
    public PropertyViewer(Composite parent, int style, ClassLoader loader)
    {
        _table         = new Table(parent, style);
        _classLoader   = loader;
        _listener      = new ListObjectListener(this);
        TableColumn c1 = new TableColumn(_table, SWT.NONE);
        TableColumn c2 = new TableColumn(_table, SWT.NONE);
        c2.setText("Value");
        c1.setText("Property");
        c2.pack();
        c1.pack();
        c1.setWidth(200);
        c2.setWidth(200);
        _cellModifier = new RowCellModifier();
        createEditorListener();

        _tableEditor = new TableEditor(_table);
        _table.setHeaderVisible(true);
        _table.setLinesVisible(true);

        _tableEditor.horizontalAlignment = SWT.LEFT;
        _tableEditor.grabHorizontal      = true;
        _table.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e)
            {
                handleSelect((TableItem) e.item);
            }
        });
        _table.addDisposeListener(_listener);
    }
    /**
     * Sets the input.
     * @param input New Input
     */
    public void setInput(Object input)
    {
        Object old = _input;
        _input = input;
        inputChanged(_input, old);
    }
    /**
     * Get Input.
     * @return the Input
     */
    public Object getInput()
    {
        return _input;
    }
    /**
     * Change in input, update listeners.
     * @param input    new input
     * @param oldInput old Input.
     */
    protected void inputChanged(Object input, Object oldInput)
    {
        EObject newObj = (EObject) input;
        EObject oldObj = (EObject) oldInput;
        // Add listener to new Input
        if (newObj != null) {
            EcoreUtils.addListener(newObj, _listener, 1);
        }
        // Remove listener from old input.
        if (oldObj != null) {
            EcoreUtils.removeListener(oldObj, _listener, 1);
        }
        deactivateCellEditor();
        refreshEntries();
    }
    /**
     * Implementation of Selection Listener.
     * Deactivate exising editor and activated new one.
     * @param item TableItem
     */
    private void handleSelect(TableItem item)
    {
        // Clean up any previous editor control
        deactivateCellEditor();
        // Identify the selected row
        if (item != null && !(item.isDisposed())) {
            activateCellEditor(item);
        }
    }
    /**
     * Activates the editor.
     * @param item TableItem
     */
    private void activateCellEditor(TableItem item)
    {
        _table.showSelection();
        _tableItem           = item;
        CellEditorFactory cf = CellEditorFactory.PROPERTY_INSTANCE;
        // The control that will be the editor.
        EStructuralFeature f = (EStructuralFeature) item.getData(FEATURE_KEY);
        String featureName   = f.getName();
        _cellEditor = cf.getEditor(f, _table, this);
        if (_cellEditor != null) {
            _cellEditor.activate();
            _cellEditor.setValue(_cellModifier.getValue(_input, featureName));
            Control control = _cellEditor.getControl();
            if (control == null) {
                _cellEditor.deactivate();
                _cellEditor = null;
                return;
            }
            // add our editor listener
            _cellEditor.addListener(_editorListener);

            // set the layout of the table tree editor to match the cell editor
            CellEditor.LayoutData layout = _cellEditor.getLayoutData();
            _tableEditor.horizontalAlignment = layout.horizontalAlignment;
            _tableEditor.grabHorizontal = layout.grabHorizontal;
            _tableEditor.minimumWidth = layout.minimumWidth;
            _tableEditor.setEditor(control, item, EDITABLE_COLUMN);

            // give focus to the cell editor
            _cellEditor.setFocus();
        }
    }
    /**
     * Returns Control for this Viewer
     * @return Control for this Viewer
     */
    public Control getControl()
    {
        return _table;
    }
    /**
     * Returns selection from Viewer.
     * @return selection from Viewer.
     */
    public ISelection getSelection()
    {
        return null;
    }
    /**
     * Deactivate the currently active cell editor.
     */
    void deactivateCellEditor()
    {
        _tableEditor.setEditor(null, null, EDITABLE_COLUMN);
        if (_cellEditor != null) {
            _cellEditor.deactivate();
            _cellEditor.removeListener(_editorListener);
            _cellEditor = null;
        }
    }
    /**
     * Sets the selection. Does nothing.
     * @param selection Selection
     * @param reveal    Reveal
     */
    public void setSelection(ISelection selection, boolean reveal)
    {
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
        return null;
    }
    /**
     * Get Value from Environment
     * @param property Key for the env variable.
     * @return Value for this property
     */
    public Object getValue(Object property)
    {
        if (property.toString().equalsIgnoreCase("model")) {
            return _input;
        } else if (property.toString().equalsIgnoreCase("shell")) {
            return _table.getShell();
        } else if (property.toString().equalsIgnoreCase("eclass")) {
            return ((EObject) _input).eClass();
        } else if (property.toString().equalsIgnoreCase("classloader")) {
            return _classLoader;
        }
        return null;
    }
    /**
     * 
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    public void setValue(Object property, Object value)
    {
             
    }
    /**
     * Refreshes the value of property values.
     */
    public void refresh()
    {
        EObject eObject   = (EObject) _input;
        TableItem items[] = _table.getItems();
        for (int i = 0; i < items.length; i++) {
            EStructuralFeature feature =
                (EStructuralFeature) items[i].getData(FEATURE_KEY);
            Object value   = (feature instanceof EReference)
                ? "..." : eObject.eGet(feature);
            String displayVal = value.toString();
            if (feature.getEType() instanceof EEnum) {
                EEnum uiEnum = EcoreUtils.getUIEnum(feature);
                if (uiEnum != null) {
                    EEnumLiteral literal = (EEnumLiteral) value;
                    displayVal = uiEnum.getEEnumLiteral(
                            literal.getValue()).getName();
                }
            }
            items[i].setText(EDITABLE_COLUMN, displayVal);
        }
    }
    /**
     * Redraw the Table, will be called after input changed.
     */
    public void refreshEntries()
    {
        EObject newObj = (EObject) _input;
        List features  = EcoreUtils.getFeatureList(newObj.eClass(), true);
        _table.removeAll();
        for (int i = 0; i < features.size(); i++) {
            EStructuralFeature feature = (EStructuralFeature) features.get(i);
            TableItem item = new TableItem(_table, SWT.NONE);
            item.setText(0, EcoreUtils.getLabel(feature));
            item.setData(FEATURE_KEY, feature);
        }
        refresh();
    }
    /**
     * Creates a new cell editor listener.
     */
    private void createEditorListener()
    {
        _editorListener = new ICellEditorListener() {
            /**
             * Cancel editting.
             */
            public void cancelEditor()
            {
                deactivateCellEditor();
            }
            /**
             * Sets Editor Value, It used RowCellModifier to save
             * the value in EObject.
             */
            public void applyEditorValue()
            {
                CellEditor c = _cellEditor;
                if (c != null) {
                    _cellEditor = null;
                    if (_tableItem != null && !(_tableItem.isDisposed())) {
                        EStructuralFeature f = (EStructuralFeature)
                            _tableItem.getData(FEATURE_KEY);
                        _cellModifier.modify(_input,
                                f.getName(), c.getValue());
                    }
                    _tableEditor.setEditor(null, null, EDITABLE_COLUMN);
                    c.removeListener(_editorListener);
                    
                    c.deactivate();
                }
            }
            /**
             * Not Used
             */
            public void editorValueChanged(boolean oldValid, boolean newValid)
            {
            }
        };
    }
}
