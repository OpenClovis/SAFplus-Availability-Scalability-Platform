/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/BlankPreferencePage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
/**
 *
 * @author shubhada
 *    Blank Preference Page
 */
public class BlankPreferencePage extends GenericPreferencePage
{
    /**
     * Constructor.
     * @param name     Name of the Page.
     */
    public BlankPreferencePage(String name)
    {
        super(name);
        setTitle(name);
        noDefaultAndApplyButton();
    }
    /**
     * @param parent Composite
     * @return Control
     */
    protected Control createContents(Composite parent)
    {
        return parent;
    }

}
