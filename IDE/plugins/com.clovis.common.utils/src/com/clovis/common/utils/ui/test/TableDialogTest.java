/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/test/TableDialogTest.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.test;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;

import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
/**
 * Tests classes used in UI Infra.
 * @author Nadeem
 */
public class TableDialogTest
{
    /**
     * Opens the dialog.
     * @param obj    Object (EObject or EList)
     * @param eClass EClass.
     */
    private static void bringUpDialog(Object obj, EClass eClass)
    {
        Display display = new Display();
        Shell shell = new Shell(display);
        shell.setLayout(new FillLayout());
        new PushButtonDialog(shell, eClass, obj).open();
    }
    /**
     * Test Main method.
     * @param args program argument.
     * @throws Exception on errors.
     */
    public static void main(String[] args)
        throws Exception
    {
        if (args.length != 2) {
            System.err.println("Usgae: <ecore_model> <xml_file>");
        }
        System.out.println("--------- UITestMain: Starting  main() ----------");
        EPackage eLibPackage = EcoreModels.get(args[0]);
        EClass eClass = (EClass) eLibPackage.getEClassifiers().get(0);
        Resource resource = EcoreModels.getResource(URI.createFileURI(args[1]));
        EList  elist = resource.getContents();

        // First argument is list for table, EObject for form.
        bringUpDialog(elist.get(0), eClass);
        System.out.println("--------- UITestMain: calling   save() ----------");
        // Save model unconditionally.
        EcoreModels.save(resource);
        System.out.println("--------- UITestMain: Finishing main() ----------");
    }
}
