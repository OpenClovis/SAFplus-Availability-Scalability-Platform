/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IFolderLayout;

import org.eclipse.ui.IPerspectiveFactory;
import org.eclipse.ui.console.IConsoleConstants;
/**
 * @author pushparaj
 *
 * ClovisWorks perspective.
 */
public class CWPerspectiveFactory implements IPerspectiveFactory
{
    /**
     * Creates the initial layout for this Perspective.
     * @param layout Page Layout.
     */
    public void createInitialLayout(IPageLayout layout)
    {
        layout.addNewWizardShortcut("com.clovis.cw.ui.actionSets");
        String editorArea = layout.getEditorArea();

        IFolderLayout bottom =
            layout.createFolder("bottom", IPageLayout.BOTTOM, .7f, editorArea);
        bottom.addView(IPageLayout.ID_TASK_LIST);
        bottom.addView("com.clovis.cw.editor.ca.views.mibtreeView");
        bottom.addView("org.eclipse.pde.runtime.LogView");
        bottom.addView("com.clovis.cw.workspace.problemsView");
        bottom.addView("com.clovis.cw.workspace.modelTemplateView");
        bottom.addView(IConsoleConstants.ID_CONSOLE_VIEW);
                
        IFolderLayout topLeft =
            layout.createFolder("Left", IPageLayout.LEFT, 0.15f, editorArea);
        topLeft.addView("com.clovis.cw.workspace.clovisWorkspaceView");

        IFolderLayout topRight =
            layout.createFolder("Right", IPageLayout.RIGHT, 0.75f, editorArea);
        topRight.addView(IPageLayout.ID_OUTLINE);
    }
}

