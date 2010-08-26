package com.clovis.cw.editor.ca.dialog;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;

/**
 * Preference page for the first item in the AMF Configuration tree. Dislays
 * instructions on how to use the AMF Configuration dialog.
 * 
 * @author matt
 */
public class AMFInstructionsPage extends GenericPreferencePage {
	
	/**
	 * Constructor
	 * @param label Title for the page.
	 */
	public AMFInstructionsPage(String label) {
		  super(label);
		  noDefaultAndApplyButton();
	}
	  
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createContents(Composite parent)
	{
		/* Create container for dialog controls */		
		Composite container = new Composite(parent, SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 1;
        container.setLayout(glayout);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));

        /******************************************************/
    	/* Create Instruction Label                           */
    	/******************************************************/
        GridData gridData = new GridData(GridData.HORIZONTAL_ALIGN_FILL);
        gridData.horizontalSpan = 1;
        Label instructions = new Label(container, SWT.WRAP);
        instructions.setText("The AMF Configuration dialog allows you to create instances of"
        		+ " the object types already created in the resource and component editors. The tree"
        		+ " in the left-hand pane can be expanded to create and work with these instances."
        		+ " The 'Node Instance List' and 'Service Group List' tree nodes present wizards"
        		+ " that can be used to quickly create a full instance tree.\n\n"
        		+ " To use the wizards begin by creating the appropriate"
        		+ " number of Node Instances from the 'Node Instance List' page. Follow this by"
        		+ " creating Service Group Instances from the 'Service Group List' page. While creating"
        		+ " the Service Group Instances they can be associated with the already existing"
        		+ " Node Instances which will automatically fill out the entire instance tree.");
        instructions.setLayoutData(gridData);

    	return container;
	}

}
