/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor.engine;

/**
 * @author pushparaj
 *
 * Class contains Meta Class information
 */
public class ClassInfo
{
    private String _name;

    private String _template;

    private String _editPartName;

    private String _smallIcon;

    private String _largeIcon;

    private String _description;
    
    private String _groupName;
    
    private String _keyStroke;

    /**
     * constructor
     *
     * @param name meta class name
     * @param template template
     * @param editPartName EditPart class name
     * @param smallIcon small icon
     * @param largeIcon large icon
     * @param description about meta class
     */
    public ClassInfo(String name, String template, String editPartName,
            String smallIcon, String largeIcon, String description, String groupName, String keyStroke)
    {
        _name = name;
        _template = template;
        _editPartName = editPartName;
        _smallIcon = smallIcon;
        _largeIcon = largeIcon;
        _description = description;
        _groupName = groupName;
        _keyStroke = keyStroke;
    }

    /**
     *
     * @return meta-class name
     */
    public String getName()
    {
        return _name;
    }

    /**
     *
     * @return meta-class template name
     */
    public String getTemplate()
    {
        return _template;
    }

    /**
     *
     * @return small icon
     */
    public String getSmallIcon()
    {
        return _smallIcon;
    }

    /**
     *
     * @return large icon
     */
    public String getLargeIcon()
    {
        return _largeIcon;
    }

    /**
     *
     * @return EditPart class name
     */
    public String getEditPartName()
    {
        return _editPartName;
    }

    /**
     *
     * @return description
     */
    public String getDescription()
    {
        return _description;
    }
    /**
     * return group name 
     */
    public String getGroupName()
    {
    	return _groupName;
    }
    /**
     * @return KeyStroke
     */
    public String getKeyStroke()
    {
    	return _keyStroke;
    }
}
