/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/AttributeList.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.widgets.List;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.list.ListView;

/**
 * @author ravik
 * List Viewer Class
 */
public class AttributeList extends ListView
{


    /**
     * Constructor
     * @param parent List
     */
    public AttributeList(List parent)
    {
        super(parent);
        setLabelProvider(new ListLabelProvider());
    }
    /**
     *
     * @author shubhada
     *
     * LabelProvider for this viewer.
     */
    class ListLabelProvider extends LabelProvider
    {
        /**
         * @param obj Object
         * @return label for the object
         */
        public String getText(Object obj)
        {
            EStructuralFeature feature = ((EObject) obj).eClass()
                        .getEStructuralFeature("Name");
            String oid = (String) EcoreUtils.getValue
            ((EObject) obj, "OID");
            if (feature != null && oid != null) {
                String label = (String) ((EObject) obj).eGet(feature);
                label = label + "(" + oid + ")";
                return label;
            }
            return "";
        }
    }

}
