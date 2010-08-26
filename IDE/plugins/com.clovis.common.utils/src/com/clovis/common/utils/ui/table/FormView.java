/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/FormView.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ICellEditorListener;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.ui.editor.FileChooserCellEditor;
import com.clovis.common.utils.ui.factory.CellEditorFactory;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.common.utils.ui.property.PropertyViewer;
/**
 * FormView composite.
 * @author Nadeem
 */
public class FormView  extends ScrolledComposite implements Environment
{
    private Map  _propertyValMap = new HashMap();
    protected EObject                _input = null;
    protected ClassLoader            _classLoader = null;
    protected Object                 _container = null;
    protected static ICellModifier cellModifier = new RowCellModifier();
    protected Map _featureCellEditorMap = new HashMap();
    private FormViewRefresher _formViewRefresher;
    
    public FormView(Composite parent, int style)
    {
    	super(parent, style | SWT.H_SCROLL | SWT.V_SCROLL);
    }
    /**
     * Constructor.
     * @param parent Parent Composite
     * @param style  SWT Style
     * @param obj    EObject to show
     * @param loader ClassLoader can be null
     */
    public FormView(Composite parent,
                    int style, EObject obj, ClassLoader loader, Object container)
    {
        super(parent, style | SWT.H_SCROLL | SWT.V_SCROLL);
        _input        = obj;
        _classLoader  = loader;
        _container = container;
        _formViewRefresher = new FormViewRefresher(this);
        populateForm();
        addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				EcoreUtils.removeListener(_input, _formViewRefresher, 1);
			}
		});
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
            return getShell();
        } else if (property.toString().equalsIgnoreCase("eclass")) {
            return _input.eClass();
        } else if (property.toString().equalsIgnoreCase("classloader")) {
            return _classLoader;
        } else if (property.toString().equalsIgnoreCase("container")) {
            return _container;
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
    /**
     * Returns Notifier.
     * @return null
     */
    public EnvironmentNotifier getNotifier()
    {
        return null;
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
     * Gets list of Features to be shown in Form. Subclasses may override
     * to control this list.
     * @return List of features for this EClass.
     */
    protected List getFeatures()
    {
        EAnnotation eAnn = _input.eClass().getEAnnotation("CWAnnotation");
		String details = (eAnn != null) ? (String) eAnn.getDetails().get(
				AnnotationConstants.CONDITIONAL_HIDDEN) : null;
		List<String> conditionalHiddens = null;

		if (details != null) {
			String[] detailsArray = details.split("#");

			Class handlerClass = ClovisUtils.loadClass(detailsArray[0]);
			Class[] argType = { EObject.class, String.class };
			Object[] args = { _input, detailsArray[2] };

			try {
				Method method = handlerClass
						.getMethod(detailsArray[1], argType);
				conditionalHiddens = (List<String>) method.invoke(null, args);

			} catch (SecurityException e) {
				e.printStackTrace();
			} catch (NoSuchMethodException e) {
				e.printStackTrace();
			} catch (IllegalArgumentException e) {
				e.printStackTrace();
			} catch (IllegalAccessException e) {
				e.printStackTrace();
			} catch (InvocationTargetException e) {
				e.printStackTrace();
			}
		}

		Vector newList = new Vector();
		List origList = _input.eClass().getEAllStructuralFeatures();

		for (int i = 0; i < origList.size(); i++) {
			EStructuralFeature f = (EStructuralFeature) origList.get(i);

			boolean isHidden = EcoreUtils.isHidden(f);
			if (!(isHidden || (conditionalHiddens != null && conditionalHiddens
					.contains(f.getName())))) {
				newList.add(f);
			}
		}

		return newList;
    }
    /**
	 * Activate the editor.
	 * 
	 * @param editor
	 *            CellEditor
	 */
    protected void activateCellEditor(CellEditor editor)
    {
        final Control control = editor.getControl();
        EStructuralFeature f = (EStructuralFeature)
            control.getData(PropertyViewer.FEATURE_KEY);
        String featureName   = f.getName();
        if (cellModifier.canModify(_input, featureName)) {
            control.setEnabled(true);
        } else {
            control.setEnabled(false);
            if (editor instanceof FileChooserCellEditor
					|| editor instanceof PushButtonCellEditor) {
				((Composite) control).getChildren()[1].setEnabled(false);
            }
        }
        
        attachEditorListener(editor);
        Object value = cellModifier.getValue(_input, featureName);
        if(value != null) {
            editor.setValue(value);
        }
    }
    /**
     * Attach EditorListener.
     * This listener is responsible for saving the changes in
     * the EObject.
     * @param editor CellEditor
     */
    protected void attachEditorListener(final CellEditor editor)
    {
        editor.addListener(new ICellEditorListener() {
            private boolean _isFocussed = true;
            /**
             * Cancel editting.
             */
            public void cancelEditor()
            {
                //Give focus to parent.
               if (_isFocussed) {
                   _isFocussed = false;
                   editor.getControl().getParent().forceFocus();
                   _isFocussed = true;
               }
            }
            /**
             * Not Used
             * @param o Is Old Valid
             * @param n Is New Valid
             */
            public void editorValueChanged(boolean o, boolean n)
            {
            }
            /**
             * Sets Editor Value, It used RowCellModifier to save
             * the value in EObject.
             */
            public void applyEditorValue()
            {
                EStructuralFeature f = (EStructuralFeature)
                    editor.getControl().getData(PropertyViewer.FEATURE_KEY);
                if (cellModifier.canModify(_input, f.getName())) {
                	cellModifier.modify(_input, f.getName(), editor.getValue());
                }
                applyDependency(_input, f);
            }

            /**
             * Updates the contents of dependent fields.
             * 
             * @param eObject EObject for the current updatation 
             * @param f Structural Feature for which we need to do
             * dependancy updatation.
             */
            private void applyDependency(EObject eObject, EStructuralFeature f) {
                String str = EcoreUtils.getAnnotationVal(f, null, "Dependency");

                if(str != null) {
                	StringTokenizer entryTokenizer = new StringTokenizer(str, "|");

                	while(entryTokenizer.hasMoreTokens()) {
                		String entry = entryTokenizer.nextToken();
                		StringTokenizer detailTokenizer = new StringTokenizer(entry, ";=");
                		String entryValue = detailTokenizer.nextToken();
                		
                		if (editor.getValue() != null
								&& (editor.getValue().toString())
										.equalsIgnoreCase(entryValue)) {
                			while(detailTokenizer.hasMoreTokens()) {
                				String featureName = detailTokenizer.nextToken();
                				String value = detailTokenizer.nextToken();

                				updateControlStatus(eObject, featureName, value);
                			}
                		}
                	}
                }
			}

            /**
             * Updates the status of dependent controls.
             * @param eObject EObject for the entry
             * @param featureName Feature to be updated.
             * @param value Value of the feature being updated.
             */
            private void updateControlStatus(EObject eObject, String featureName, String value) {
                String disableType = null;
				if(featureName.contains(":")) {
					int index = featureName.indexOf(":");
					disableType = featureName.substring(index+1, featureName.length());
					featureName = featureName.substring(0, index);
				}
				
				EStructuralFeature feature = null;
	            if(featureName.equalsIgnoreCase("Disable")) {
		            feature = eObject.eClass().getEStructuralFeature(value);
	            } else {
	            	feature = eObject.eClass().getEStructuralFeature(featureName);
	            }
				String label = EcoreUtils.getAnnotationVal(feature, null, "label");

                Control[] control = ((Composite) getChildren()[0]).getChildren();
				for(int i=0 ; i<control.length ; i++) {
					if(control[i] instanceof Label) {
						Label lbl = (Label) control[i];
						String str = lbl.getText();
						str = str.substring(0, str.length()-1);

						if(str.equalsIgnoreCase(label)) {
            				if(featureName.equalsIgnoreCase("Disable")) {
            					EList list = (EList)EcoreUtils.getValue(eObject, featureName);

            					if(disableType.equals("add")) {
            						list.add(value);
            						if(control[i+1] instanceof Composite) {
            							((Composite) control[i+1]).getChildren()[1].setEnabled(false);
            						}
									control[i+1].setEnabled(false);
            					} else if(disableType.equals("remove")) {
            						list.remove(value);
            						if(control[i+1] instanceof Composite) {
            							((Composite) control[i+1]).getChildren()[1].setEnabled(true);
            						}
									control[i+1].setEnabled(true);
            					}
							} else {

								if(control[i+1] instanceof Combo) {
									((Combo) control[i+1]).setText(value);
								} else if(control[i+1] instanceof Text) {
									((Text) control[i+1]).setText(value);
								}
        						EcoreUtils.setValue(eObject, featureName, value);
							}
						}
					}
				}
			}
        });
    }
    
    /**
     * Populate the Form.
     * Takes features from the list and show then as key-value.
     */
    protected void populateForm()
    {
        setExpandVertical(true);
        setExpandHorizontal(true);
        GridLayout containerLayout = new GridLayout();
        setLayout(containerLayout);
        
        Composite formComposite =
            new Composite(this, SWT.NONE);
        formComposite.setLayoutData(new GridData(GridData.FILL_BOTH));
        setContent(formComposite);

        List features = getFeatures();
        GridLayout formLayout = new GridLayout();
        formLayout.numColumns = 4;
        formComposite.setLayout(formLayout);
        CellEditorFactory cf = CellEditorFactory.FORM_INSTANCE;
        for (int i = 0; i < features.size(); i++) {
            final EStructuralFeature feature = (EStructuralFeature) features.get(i);
            if (!EcoreUtils.isHidden(feature)) {
                Label namelbl = new Label(formComposite, SWT.NONE);
                GridData labelData = new GridData(GridData.BEGINNING);
                labelData.horizontalSpan = 2;
                namelbl.setLayoutData(labelData);
                namelbl.setText(EcoreUtils.getLabel(feature)+":");

                int style = SWT.SINGLE | SWT.LEFT | SWT.BORDER;
                final CellEditor editor = cf.getEditor(feature, formComposite, this, style);
                editor.activate();
                final Control vControl = editor.getControl();
                _featureCellEditorMap.put(feature, editor);
                vControl.setVisible(true);
                vControl.setData(PropertyViewer.FEATURE_KEY, feature);
                activateCellEditor(editor);
                vControl.setBackground(ColorConstants.white);
                String dirStr = EcoreUtils.getAnnotationVal(
                        feature, null, "isDirectory");
                //if the field is a directory location put the Browse button 
                // along with the text field
                if (dirStr != null) {
                    boolean isDir = Boolean.parseBoolean(dirStr);
                    if (isDir) {
                    ((Text) vControl).setEditable(false);
                    GridData controlData = new GridData(GridData.FILL_HORIZONTAL);
                    controlData.horizontalSpan = 1;
                    vControl.setLayoutData(controlData);
//                  Add Browse Button.
                    Button button = new Button(formComposite, SWT.PUSH);
                    button.setText("Browse...");
                    button.addSelectionListener(new SelectionListener() {
                        /**
                         * Open Directory Dialog.
                         * @param e Event
                         */
                        public void widgetSelected(SelectionEvent e)
                        {
                            DirectoryDialog dialog =
                                new DirectoryDialog(getShell(), SWT.NONE);
                            dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue(feature.eClass().getName()));
                            String fileName = dialog.open();
                            UtilsPlugin.saveDialogSettings(feature.eClass().getName(), fileName);
                            if (fileName != null) {
                                ((Text) vControl).setText(fileName);
                                cellModifier.modify(_input, feature.getName(), editor.getValue());
                            }
                        }
                        /**
                         * Does Nothig.
                         * @param e Event
                         */
                        public void widgetDefaultSelected(SelectionEvent e)
                        {
                        }
                    });
                    }
                } else {
                    GridData controlData = new GridData(GridData.FILL_HORIZONTAL);
                    controlData.horizontalSpan = 2;
                    vControl.setLayoutData(controlData);
                }
            }
        }
        setMinSize(formComposite.computeSize(SWT.DEFAULT, SWT.DEFAULT));
    }

    /**
     * @return Map
     */
    public Map getFeatureCellEditorMap() {
		return _featureCellEditorMap;
	}
}
