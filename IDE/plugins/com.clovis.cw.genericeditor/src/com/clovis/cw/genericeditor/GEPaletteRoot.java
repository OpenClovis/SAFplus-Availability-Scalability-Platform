/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GEPaletteRoot.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.gef.palette.PaletteDrawer;
import org.eclipse.gef.palette.PaletteEntry;
import org.eclipse.gef.palette.PaletteRoot;
import org.eclipse.gef.palette.ToolEntry;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.engine.ClassInfo;
import com.clovis.cw.genericeditor.engine.Parser;
import com.clovis.cw.genericeditor.model.GEEdgeElementFactory;
import com.clovis.cw.genericeditor.model.GENodeElementFactory;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.genericeditor.tools.GEConnectionCreationToolEntry;
import com.clovis.cw.genericeditor.tools.GECreationToolEntry;
import com.clovis.cw.genericeditor.tools.GESelectionToolEntry;

/**
 * @author pushparaj
 *
 * This class will populate pallete entries.
 */
public class GEPaletteRoot extends PaletteRoot
{
    private GenericEditor _editor;

    // meta-class info for nodes
    private Map _nodesInfo;

    // meta-class info for edges
    private Map _edgesInfo;

    /*private PaletteDrawer _nodeDrawer = new PaletteDrawer("Nodes",
			ImageDescriptor.createFromFile(NodeModel.class, "icons/comp16.gif"));

	private PaletteDrawer _edgeGroup = new PaletteDrawer("Edges",
			ImageDescriptor.createFromFile(NodeModel.class, "icons/con16.gif"));*/
    
    private Map _paletteDrawerGroups = new HashMap();
    
    /**
     * constructor
     * @param editor Editor
     * @param pack EPackage
     * @param objs meta-objects list
     * @param option option to populate palette
     */
    public GEPaletteRoot(GenericEditor editor, EPackage pack,
            EList objs, int option, Map toolEntries)
    {
    	_editor = editor;
        populatePalette(pack, objs, option, toolEntries);
    }
    /**
     * populate palette entries. and create Element Factories
     * for each palette entry.
     * @param pack EPackage
     * @param metaobjs meta-objects list
     * @param option option to populate palette
     */
    private void populatePalette(EPackage pack, EList metaobjs, int option, Map toolEntries)
    {
        Parser parser = _editor._parser;
        _nodesInfo = parser.getNodeModels();
        _edgesInfo = parser.getEdgeModels();
        List paletteEntries = new ArrayList(3);
        GESelectionToolEntry selectionTool = new GESelectionToolEntry("Select [Q]", _editor);
        paletteEntries.add(selectionTool);
        toolEntries.put("Q", selectionTool);
        /*_nodeDrawer.setUserModificationPermission(
                PaletteEntry.PERMISSION_FULL_MODIFICATION);*/
        if (option == 1) {
            populateMetaClasses(pack, toolEntries);
        } else if (option == 2) {
            populateMetaObjects(metaobjs, toolEntries);
        } else {
            populateMetaClasses(pack, toolEntries);
            populateMetaObjects(metaobjs, toolEntries);
        }
        populateEdges(pack, toolEntries);
        /*paletteEntries.add(_nodeDrawer);
        paletteEntries.add(_edgeGroup);*/
        Object groups[] = _paletteDrawerGroups.values().toArray();
        for (int i = 0; i < groups.length; i++) {
        	paletteEntries.add(groups[i]);
        }
        setChildren(paletteEntries);
    }
    /**
     * This will populate meta-classes info
     * in palette
     * @param pack EPackage
     */
    private void populateMetaClasses(EPackage pack, Map toolEntries)
    {
        Map elementFactoriesMap = _editor._elementFactoriesMap;
        if (_nodesInfo != null) {
            Object[] templates = _nodesInfo.values().toArray();
            for (int i = 0; i < templates.length; i++) {
                ClassInfo classInfo = (ClassInfo) templates[i];
                EClass eClass = (EClass) pack.getEClassifier(classInfo
                        .getName());
                EObject eObj = EcoreUtils.createEObject(eClass, true);
                GENodeElementFactory elementFactory = new GENodeElementFactory(
                        _editor, eObj);
                elementFactoriesMap.put(eObj, elementFactory);
                //template creation for nodes.
                GECreationToolEntry combined = new GECreationToolEntry(
						classInfo.getTemplate() + " [" 
								+ classInfo.getKeyStroke()+"]", classInfo
								.getDescription(), eObj, elementFactory,
						ImageDescriptor.createFromFile(getClass(), classInfo
								.getSmallIcon()), ImageDescriptor
								.createFromFile(getClass(), classInfo
										.getLargeIcon()), classInfo.getKeyStroke(), _editor);
                // entries1.add(combined);
                toolEntries.put(classInfo.getKeyStroke(), combined);
                getPaletteDrawerGroup(classInfo.getGroupName()).add(combined);
            }
        }
        //_nodeDrawer.addAll(entries1);
    }
    /**
     * This will populate meta-objects info
     * in palette
     * @param metaObjs meta-objects list
     */
    private void populateMetaObjects(EList metaObjs, Map toolEntries)
    {
        Map elementFactoriesMap = _editor._elementFactoriesMap;
        for (int i = 0; i < metaObjs.size(); i++) {
            EObject eObj = (EObject) metaObjs.get(i);
            String name = eObj.eClass().getName();
            
            if (_nodesInfo.containsKey(name)) {
                ClassInfo classInfo = (ClassInfo) _nodesInfo.get(name);
                GENodeElementFactory elementFactory = new GENodeElementFactory(
                        _editor, eObj);
                elementFactoriesMap.put(eObj, elementFactory);
                //template creation for nodes.
                GECreationToolEntry combined =
                    new GECreationToolEntry(classInfo.getTemplate() + " [" 
							+ classInfo.getKeyStroke()+"]",
                    		classInfo.getDescription(), eObj, elementFactory, ImageDescriptor
                            .createFromFile(
                            		_editor.getClass(), classInfo
                                    .getSmallIcon()), ImageDescriptor.
                                    createFromFile(_editor.getClass(),
                                            classInfo.getLargeIcon()), classInfo.getKeyStroke(), _editor);
                //entries1.add(combined);
                toolEntries.put(classInfo.getKeyStroke(), combined);
                getPaletteDrawerGroup(classInfo.getGroupName()).add(combined);
            }
        }
        //_nodeDrawer.addAll(entries1);
    }
    /**
     * Populates Edge's Meta-Class Info in the palette
     * @param pack EPackage
     */
    private void populateEdges(EPackage pack, Map toolEntries)
    {
        Map elementFactoriesMap = _editor._elementFactoriesMap;
        if (_edgesInfo != null) {
            Object[] templates = _edgesInfo.values().toArray();
            for (int i = 0; i < templates.length; i++) {
                ClassInfo classInfo = (ClassInfo) templates[i];
                EClass eClass = (EClass) pack.getEClassifier(classInfo
                        .getName());
                EObject eObj = EcoreUtils.createEObject(eClass, true);
                GEEdgeElementFactory elementFactory = new GEEdgeElementFactory(
                        _editor, eObj);
                elementFactoriesMap.put(eObj, elementFactory);
                // templates creation for edges.
                ToolEntry tool = new GEConnectionCreationToolEntry(classInfo
                        .getTemplate() + " [" 
						+ classInfo.getKeyStroke()+"]", classInfo.getDescription(),
                        elementFactory, ImageDescriptor.createFromFile(
                                _editor.getClass(), classInfo.getSmallIcon()),
                        ImageDescriptor.createFromFile(_editor.getClass(), classInfo
                                .getLargeIcon()), classInfo.getKeyStroke(), _editor);
                //entries2.add(tool);
                toolEntries.put(classInfo.getKeyStroke(), tool);
                getPaletteDrawerGroup(classInfo.getGroupName()).add(tool);
            }
            //_edgeGroup.addAll(entries2);
        }
    }
    /**
     * Return PaletteDrawer
     * @param groupName
     * @return PaletteDrawer
     */
    private PaletteDrawer getPaletteDrawerGroup(String groupName) {
    	PaletteDrawer drawer = (PaletteDrawer) _paletteDrawerGroups.get(groupName);
    	if (drawer == null) {
    		drawer = new PaletteDrawer(groupName,
    	            ImageDescriptor.createFromFile(NodeModel.class,
    	                    "icons/comp16.gif"));
    		drawer.setUserModificationPermission(
                    PaletteEntry.PERMISSION_FULL_MODIFICATION);
    		_paletteDrawerGroups.put(groupName, drawer);
    	}
    	return drawer;
    }
}
