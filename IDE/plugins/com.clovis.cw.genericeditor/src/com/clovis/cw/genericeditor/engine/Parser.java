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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.exolab.castor.xml.MarshalException;
import org.exolab.castor.xml.ValidationException;

import com.clovis.cw.genericeditor.GenericeditorPlugin;
import com.clovis.cw.genericeditor.gen.metaclasses.EditorInfo;
import com.clovis.cw.genericeditor.gen.metaclasses.MetaClass;

/**
 * @author pushparaj
 *
 * This class will parse meta class file and populate
 * meta-classes info.
 */
public class Parser
{
    private Map  _nodesModels        = new HashMap();

    private Map  _edgesModels        = new HashMap();

    private String    _menuXmlPath   = null;

    private Map       _editPartsMap  = new HashMap();

    private ArrayList _nodeNamesList = new ArrayList();

    private ArrayList _edgeNamesList = new ArrayList();

    /**
     * constructor
     *
     * @param file meta-class file
     */
    public Parser(File file)
    {
        EditorInfo info = null;
        try {
            info = (EditorInfo) EditorInfo.unmarshalEditorInfo(new FileReader(
                    file.getAbsolutePath()));
        } catch (FileNotFoundException e) {
            GenericeditorPlugin.LOG.warn("File " + file.getName()
                                         + " not exist", e);
        } catch (ValidationException e) {
            GenericeditorPlugin.LOG.warn("File " + file.getName()
                                         + " is invalid", e);
        } catch (MarshalException e) {
            GenericeditorPlugin.LOG.error(e);
        } catch (Exception e) {
            GenericeditorPlugin.LOG.error("Unable to UnMarshal file "
                                          + file.getName(), e);
        }
        _menuXmlPath = info.getMenuFilePath();
        MetaClass[] nodes = info.getNodeInfo();
        //_nodesModels = new Object[nodes.length];
        for (int i = 0; i < nodes.length; i++) {
            MetaClass node = (MetaClass) nodes[i];
            String label = node.getLabel();
            String editClass = node.getEditClass();
            /*_nodesModels[i] = new ClassInfo(label, node.getTemplate(),
                    editClass, node.getIconSmall(), node.getIconLarge(), node
                            .getShortDesc());*/
            _nodesModels.put(label, new ClassInfo(label, node.getTemplate(),
                    editClass, node.getIconSmall(), node.getIconLarge(), node
                            .getShortDesc(), node.getGroupName(), node.getKeyStroke()));
            _editPartsMap.put(label, editClass);
            _nodeNamesList.add(label);
        }

        MetaClass[] edges = info.getEdgeInfo();
        //_edgesModels = new Object[edges.length];
        for (int i = 0; i < edges.length; i++) {
            MetaClass edge = (MetaClass) edges[i];
            String label = edge.getLabel();
            String editClass = edge.getEditClass();
            /*_edgesModels[i] = new ClassInfo(label, edge.getTemplate(),
                    editClass, edge.getIconSmall(), edge.getIconLarge(), edge
                            .getShortDesc());*/
            _edgesModels.put(label, new ClassInfo(label, edge.getTemplate(),
                    editClass, edge.getIconSmall(), edge.getIconLarge(), edge
                            .getShortDesc(), edge.getGroupName(), edge.getKeyStroke()));
            _editPartsMap.put(label, editClass);
            _edgeNamesList.add(label);
        }
    }

    /**
     * Get Menu Xml.
     *
     * @return mene xml file
     */
    public String getMenuXmlPath()
    {
        return _menuXmlPath;
    }

    /**
     * Get all Nodes.
     *
     * @return _nodesModels
     */
    public Map getNodeModels()
    {
        return _nodesModels;
    }

    /**
     * Get all Edges.
     *
     * @return _edgeModels
     */
    public Map getEdgeModels()
    {
        return _edgesModels;
    }

    /**
     * Get EditParts Map.
     *
     * @return _editPartMaps. have meta-class EditPart class mapping
     */
    public Map getEditPartsMap()
    {
        return _editPartsMap;
    }

    /**
     * Have node's names list
     *
     * @return nodes list
     */
    public ArrayList getNodeNamesList()
    {
        return _nodeNamesList;
    }

    /**
     * return edge's names list
     *
     * @return edges list
     */
    public ArrayList getEdgeNamesList()
    {
        return _edgeNamesList;
    }
}
