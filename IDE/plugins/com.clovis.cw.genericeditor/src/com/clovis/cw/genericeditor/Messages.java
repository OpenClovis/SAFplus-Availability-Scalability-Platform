/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.util.MissingResourceException;

/**
 * @author pushparaj
 *
 * Constants for Generic Editor
 */
public interface Messages
{
    public static final String BASEDIAGRAM_LABELTEXT
    = Helper.getString("BaseDiagram.LabelText");
    public static final String CONNECTION_LABELTEXT
    = Helper.getString("Connection.LabelText");
    
    public static final String CWKEY
    = Helper.getString("EObject.Key");
    
    // PropertyDescriptor
    public static final String PROPERTDESCRIPTOR_BASEDIAGRAM_CONNECTIONROUTER
    = Helper.getString("PropertyDescriptor.BaseDiagram.ConnectionRouter");
    public static final String PROPERTDESCRIPTOR_BASEDIAGRAM_MANUAL
    = Helper.getString("PropertyDescriptor.BaseDiagram.Manual");
    public static final String PROPERTDESCRIPTOR_BASEDIAGRAM_MANHATTAN
    = Helper.getString("PropertyDescriptor.BaseDiagram.Manhattan");
    
    // Context Menus
    public static final String COPYACTION_LABEL
    = Helper.getString("ContextMenu.CopyAction.Label");
    public static final String CUTACTION_LABEL
    = Helper.getString("ContextMenu.CutAction.Label");
    public static final String PASTEACTION_LABEL
    = Helper.getString("ContextMenu.PasteAction.Label");
    
    // Commands
    public static final String CONNECTIONCOMMAND_LABEL
    = Helper.getString("ConnectionCommand.Label");
    public static final String CONNECTIONCOMMAND_DESCRIPTION
    = Helper.getString("ConnectionCommand.Description");
    public static final String ADDCOMMAND_LABEL
    = Helper.getString("AddCommand.Label");
    public static final String ADDCOMMAND_DESCRIPTION
    = Helper.getString("AddCommand.Description");
    public static final String CREATECOMMAND_LABEL
    = Helper.getString("CreateCommand.Label");
    public static final String CREATECOMMAND_DESCRIPTION
    = Helper.getString("CreateCommand.Description");
    public static final String DELETECOMMAND_LABEL
    = Helper.getString("DeleteCommand.Label");
    public static final String DELETECOMMAND_DESCRIPTION
    = Helper.getString("DeleteCommand.Description");
    public static final String ORPHANCHILDCOMMAND_LABEL
    = Helper.getString("OrphanChildCommand.Label");
    public static final String ORPHANCHILDCOMMAND_DESCRIPTION
    = Helper.getString("OrphanChildCommand.Description");
    
    // Connection Properties
    public static final String CONNECTIONPROPERTY_VALUE
    = Helper.getString("Connection.Property.Value");
    public static final String CONNECTIONPROPERTY_BENDPOINT
    = Helper.getString("Connection.Property.Bendpoint");
    
    // Policies
    public static final String XYPOLICY_ADDCOMMAND_LABEL
    = Helper.getString("GEXYLayoutEditPolicy.AddCommand.Label");
    public static final String XYPOLICY_CREATECOMMAND_LABEL
    = Helper.getString("GEXYLayoutEditPolicy.CreateCommand.Label");
    public static final String ELEMENTPOLICY_ORPHANCOMMAND_LABEL
    = Helper.getString("GEElementEditPolicy.OrphanCommand.Label");
    public static final String CONTAINERPOLICY_ORPHANCOMMAND_LABEL
    = Helper.getString("GEContainerEditPolicy.OrphanCommand.Label");
    public static final String XYPOLICY_ADDCOMMAND_DEBUG_LABEL
    = Helper.getString("GEXYLayoutEditPolicy.AddCommand.Debug.Label");
    public static final String XYPOLICY_SETCONSTRAINTCOMMAND_DEBUG_LABEL
    = Helper.getString("GEXYLayoutEditPolicy.SetConstraintCommand.Debug.Label");
    
    // Error Messages
    public static final String CLASSLOADER_UNABLE_LOAD
    = Helper.getString("ClassLoader.Unable.Load");
    public static final String CLASSLOADER_UNABLE_INSTANTIATE
    = Helper.getString("ClassLoader.Unable.Instantiate");
    public static final String CLASSLOADER_UNABLE_ACCESS
    = Helper.getString("ClassLoader.Unable.Access");
    public static final String FILE_LABEL
    = Helper.getString("File.Label");
    public static final String FILE_NOTEXIST
    = Helper.getString("File.NotExist");
    public static final String FILE_INVALID
    = Helper.getString("File.Invalid");
    public static final String MARSHALL_INVALIDDATA
    = Helper.getString("Marshall.InValidData");
    public static final String MARSHALL_ERROR
    = Helper.getString("Matshall.Error");
    /**
     *
     * @author pushparaj
     *
     * Helper class to read resource bundle
     */
    static class Helper
    {
        /**
         *
         * @param key resource key
         * @return value
         */
        public static String getString(String key)
        {
            try {
                return GenericeditorPlugin.getResourceString(key);
            } catch (MissingResourceException e) {
                GenericeditorPlugin.LOG.warn
                ("Unable to get the value for " + key
                        + " from resource bundle");
                return key;
            }
        }
    }
}
