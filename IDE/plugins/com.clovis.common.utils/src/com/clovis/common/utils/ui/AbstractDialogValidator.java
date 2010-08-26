/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/AbstractDialogValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui;

import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import com.clovis.common.utils.ecore.AbstractValidator;
/**
 * @author shubhada
 *
 * Abstract dialog validator class
 */
public abstract class AbstractDialogValidator extends AbstractValidator
{
    private TitleAreaDialog  _tdialog    = null;
    private PreferenceDialog _pdialog    = null;
    private DialogPage       _dialogPage = null;
    /**
     *
     * @param dialog - TitleAreaDialog
     */
    public AbstractDialogValidator(TitleAreaDialog dialog)
    {
        _tdialog = dialog;
    }
    /**
     *
     * @param dialog - PreferenceDialog
     */
    public AbstractDialogValidator(PreferenceDialog dialog)
    {
        _pdialog = dialog;
    }
    /**
     *
     * @param page - Dialog Page
     */
    public AbstractDialogValidator(DialogPage page)
    {
        _dialogPage = page;
    }
    /**
     * @param message - Message to set
     */
    public void setMessage(String message)
    {
        int type = isValid() ? IMessageProvider.INFORMATION : IMessageProvider.ERROR;
        if (_tdialog != null) {
            _tdialog.setMessage(message, type);
        } else if (_pdialog != null) {
            _pdialog.setMessage(message, type);
        } else if (_dialogPage != null) {
            _dialogPage.setMessage(message, type);
        }
    }
}
