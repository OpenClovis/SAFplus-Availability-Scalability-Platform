/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/table/TableDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.table;

import java.math.BigInteger;
import java.util.List;

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.ui.PlatformUI;

import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.IStructuredSelection;

/**
 * Provides a Dialog using TableUI Class.
 * @author Ashish
 */
public class TableDialog   extends TitleAreaDialog 
{
	private Table    table       = null;
	private TableUI  tableViewer = null;
		
	private EClass eClass        = null;
	private List eObjects        = null;
	private Button addButton     = null;
	private Button deleteButton  = null;
	private static final String ADD_BUTTON    = "Add...";
	private static final String DELETE_BUTTON = "Delete";
	/**
	 * Constructor.
	 */
	public TableDialog(Shell parent, EClass eClass, List eObjects) 
    {
		super(parent);
		this.eClass = eClass;
		this.eObjects = eObjects;
	}
	/**
     * Callback method to created Dialog.
     */
	protected Control createDialogArea( Composite aParent) 
	{
		Composite baseComposite = new Composite( aParent, SWT.NONE);
		GridLayout gridLayout = new GridLayout();
		gridLayout.numColumns = 7;
		baseComposite.setLayout( gridLayout );
		GridData d1 = new GridData( GridData.FILL_BOTH );
		d1.horizontalSpan = 7;
		baseComposite.setLayoutData(d1);
		int style = SWT.SINGLE | SWT.BORDER | SWT.H_SCROLL
				| SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
		
		table = new Table ( baseComposite, style );
		tableViewer = new TableUI(table, this.eClass, null);
		
		GridData gridData1 = new GridData();
		gridData1.horizontalSpan = 6;
		gridData1.horizontalAlignment = GridData.FILL;
		gridData1.grabExcessHorizontalSpace = true;
		gridData1.grabExcessVerticalSpace = true;
		gridData1.verticalAlignment = GridData.FILL;

		table.setLayoutData( gridData1 );
		table.setLinesVisible( true );
		table.setHeaderVisible( true );
		
		Composite buttonComposite = new Composite(baseComposite, SWT.NONE);
		buttonComposite.setLayout(new GridLayout());

		addButton = new Button(buttonComposite, SWT.PUSH);
		addButton.setText(ADD_BUTTON);
		addButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

		ButtonHandler buttonHandler = new ButtonHandler();
		addButton.addSelectionListener(buttonHandler);

		deleteButton = new Button(buttonComposite, SWT.PUSH);
		deleteButton.setText(DELETE_BUTTON);
		deleteButton.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		deleteButton.addSelectionListener(buttonHandler);

		tableViewer.setInput( eObjects );
		this.setMessage( "Some Message" );
		this.setTitle( "Clovis Table" );
		aParent.setSize( 600, 400 );
		EAnnotation ann = eClass.getEAnnotation("CWAnnotation");
		if (ann != null) {
			if (ann.getDetails().get("Help") != null) {
				final String contextid = (String) ann.getDetails().get("Help");
				baseComposite.addHelpListener(new HelpListener() {

					public void helpRequested(HelpEvent e) {
						PlatformUI.getWorkbench().getHelpSystem().displayHelp(
								contextid);
					}
				});

			}
		}
		return baseComposite;
	}
	/**
	 * Button Handler.
	 * @author Ashish
	 */
	class ButtonHandler extends SelectionAdapter 
	{
        /**
         * Button pressed callback.
         */
		public void widgetSelected( SelectionEvent evt) 
		{
			if (evt.getSource().equals(addButton)) {
				handleAddRowAction();
			} else if (evt.getSource().equals(deleteButton)) {
				handleDeleteRowAction();
			}
		}
        /**
         * Get Value for an Attribute.
         * @param attr Attribute
         */
		private Object getValue(EAttribute attr)
		{
			Object value = attr.getDefaultValue();
			if (value == null) {
                return null;
            }
            String typeName = attr.getEAttributeType().getName();
            Object defValue = attr.getDefaultValue();
            if (typeName.equals("String")) {
                return defValue != null ? defValue : new String("");
            } else if (typeName.equals("Integer")) {
                return defValue != null ? defValue : new BigInteger("0");
            } else if (attr.getEType() instanceof EEnum) {
                return defValue != null ? defValue :
                ((EEnum) attr.getEType()).getEEnumLiteral(0);
            }
			return value;
		}
        /**
         * Add row in the List.
         */
		private void handleAddRowAction() 
        {	
			EObject eObject = 
                eClass.getEPackage().getEFactoryInstance().create(eClass);
			List listOfAttributes = eClass.getEAllAttributes();
			int count = listOfAttributes.size();
			for ( int i=0; i < count; i++ ) {
				eObject.eSet((EStructuralFeature)listOfAttributes.get(i), 
					getValue ((EAttribute)listOfAttributes.get(i)));
			}
			eObjects.add(eObject);
			tableViewer.refresh();
		}
        /**
         * Delete Row.
         */
		private void handleDeleteRowAction() {
			int index = tableViewer.getTable().getSelectionIndex();
			if ( index != -1 ) {
			    EObject eObject = (EObject) ((IStructuredSelection) 
                        tableViewer.getSelection()).getFirstElement();
                if (eObject != null) {
                    eObjects.remove(eObject);
                    tableViewer.refresh();
                    table.setSelection((index == 0) ? 0 : (index - 1));
                }
            }
		}
	}
}
