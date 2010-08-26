/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/test/ToolbarProviderTest.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu.test;

import java.io.FileReader;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import org.eclipse.jface.dialogs.TitleAreaDialog;

import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.MenuBuilder;
/**
 * Dialog Class
 * @author nadeem
 */
class ToolbarTestDialog extends TitleAreaDialog
{
    private MenuBuilder _menuBuilder;
    /**
     * Test Main method.
     * @param shell   Parent Shell.
     * @param builder MenuBuilder
     */
    public ToolbarTestDialog(Shell shell, MenuBuilder builder)
    {
        super(shell);
        _menuBuilder = builder;
    }
    /**
     * Creation callback.
     * @param parent Parent Composite.
     * @return Control.
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite baseComposite = new Composite(parent, SWT.NONE);
        _menuBuilder.getToolbar(parent, 0);
        return baseComposite;
    }
}
/**
 * @author shubhada
 * Test Class.
 */
public class ToolbarProviderTest
{
    /**
     * Test Method.
     * @param  args      Program Argument
     * @throws Exception Error
     */
    public static void main(String[] args)
        throws Exception
    {
        if (args.length != 1) {
            System.err.println("Usgae: <menu_xml_file>");
        }
        System.out.println("--------- ToolTest: Starting  main() ----------");
        Display display = new Display();
        Shell shell     = new Shell(display);
        shell.setLayout(new FillLayout());

        Environment env = new Environment() {
            public Object getValue(Object key)       { return null; }
            public Environment getParentEnv()        { return null; }
            public EnvironmentNotifier getNotifier() { return null; }
            public void setValue(Object key, Object value) { }
        };
        MenuBuilder builder = new MenuBuilder(new FileReader(args[0]), env);
        ToolbarTestDialog testWindow = new ToolbarTestDialog(shell, builder);
        System.out.println("--------- ToolTest: Opening  Dialog -----------");
        testWindow.open();
    }
}
