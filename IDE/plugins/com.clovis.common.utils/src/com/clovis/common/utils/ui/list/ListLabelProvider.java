/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/list/ListLabelProvider.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.list;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.LabelProvider;

import com.clovis.common.utils.ecore.EcoreUtils;
/**
 * LabelProvider for this viewer.
 * @author shubhada
 */
public class ListLabelProvider extends LabelProvider
{
    /**
     * @param obj Object
     * @return label for the object
     */
    public String getText(Object obj)
    {
        String text = null;
        if (obj instanceof EObject) {
            text = EcoreUtils.getName((EObject) obj);
            return (text != null)
                ? text : EcoreUtils.getLabel(((EObject) obj).eClass());
        }
        return obj.toString();
    }
}
