/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/EditorPropertiesUtil.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.util.EcoreEList;

import org.exolab.castor.xml.MarshalException;
import org.exolab.castor.xml.Marshaller;
import org.exolab.castor.xml.ValidationException;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.genericeditor.gen.uiproperties.Bend;
import com.clovis.cw.genericeditor.gen.uiproperties.Edge;
import com.clovis.cw.genericeditor.gen.uiproperties.EditorInfo;
import com.clovis.cw.genericeditor.gen.uiproperties.Location;
import com.clovis.cw.genericeditor.gen.uiproperties.Node;
import com.clovis.cw.genericeditor.gen.uiproperties.Size;
import com.clovis.cw.genericeditor.model.ConnectionBendpoint;
import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 * Reads ui property file and provide datas from it
 */
public class EditorPropertiesUtil
{
    private File                _propFile;
    private EList               _eObjects;
    private ArrayList           _nodesObjects;
    private EditorInfo          _editorInfo;
    private ArrayList            _nodeNamesList;
    private GEUtils             _utilsObj;
    private EditorModel         _editorModel = new EditorModel();

    /**
     * constructs the reader.
     * @param file UI Property File
     * @param eObjects List of EObjects
     * @param utils GEUtils
     * @param nodelist List of Nodes.
     */
    public EditorPropertiesUtil(File file,
            EList eObjects, GEUtils utils, ArrayList nodelist)
    {
        this._propFile = file;
        this._eObjects = eObjects;
        this._utilsObj = utils;
        _nodesObjects  = new ArrayList();
        _nodeNamesList = nodelist;
    }

    /**
     * Returns the EditorModel.
     * @return editor model.
     */
    public EditorModel getEditorModel()
    {
        try {
            FileReader reader = new FileReader(_propFile);
            _editorInfo = (EditorInfo) EditorInfo.unmarshalEditorInfo(reader);
            _editorModel.setZoom(_editorInfo.getZoom());
            _editorModel.setConnectionRouter((
                        new Integer(_editorInfo.getConnection())));
        } catch (FileNotFoundException e) {            
            _editorInfo = new EditorInfo();
        } catch (ValidationException e) {
            GenericeditorPlugin.LOG.warn(Messages.FILE_LABEL
                    + _propFile.getName() + Messages.FILE_INVALID, e);
        } catch (Exception e) {
            GenericeditorPlugin.LOG.error(e);
        }
        readEObjects();
        return _editorModel;
    }
    /**
     * 
     *
     */
    public void readEObjects()
    {
    	EObject rootObject = (EObject)_eObjects.get(0);
    	List refList = rootObject.eClass().getEAllReferences();
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            EList list = (EList) rootObject.eGet(ref);
            String refType = ((EcoreEList) list).getEStructuralFeature().getEType().getName();  
            if(!_editorModel.getEReferencesEListMap().containsKey(refType)) {
            		_editorModel.getEReferencesEListMap().put(refType, list);
            }
        }
        for (int j = 0; j < refList.size(); j++) {
            EReference ref = (EReference) refList.get(j);
            EList list = (EList) rootObject.eGet(ref);
            for (int i = 0; i < list.size(); i++) {
    			EObject obj = (EObject) list.get(i);
    			String value = EcoreUtils.getAnnotationVal(obj.eClass(), null,
    					"isContainer");
    			if (value != null && value.equals("true")) {
    				if (_nodeNamesList.contains(obj.eClass().getName())) {
    					createContainerNode(obj);
    				}
    			}
    		}
        }
        for (int j = 0; j < refList.size(); j++) {
            EReference ref = (EReference) refList.get(j);
            EList list = (EList) rootObject.eGet(ref);
            for (int i = 0; i < list.size(); i++) {
    			EObject obj = (EObject) list.get(i);
    			String value = EcoreUtils.getAnnotationVal(obj.eClass(), null,
    					"isContainer");
    			if (value == null || !(value.equals("true"))) {
    				if (_nodeNamesList.contains(obj.eClass().getName())) {
    					createNode(obj);
    				}
    			}
    		}
        }
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            EList list = (EList) rootObject.eGet(ref);
            /*String refType = ((EcoreEList) obj).getEStructuralFeature().getEType().getName();  
            if(!_editorModel.getEReferencesEListMap().containsKey(refType)) {
            		_editorModel.getEReferencesEListMap().put(refType, obj);
            }*/
           	readEdgeEObjects(list);
        }
    }
    /**
	 * 
	 * @param list
	 */
    private void readEdgeEObjects(EList list)
    {
    	for (int i = 0; i < list.size(); i++) {
    		EObject obj = (EObject) list.get(i);
           if (!_nodeNamesList.contains(obj.eClass().getName())) {
               createEdge(obj);
            }
    	}
    }
    /**
     * 
     * @param obj
     */
    private void createContainerNode(EObject obj) {
		if (EcoreUtils.getName(obj) != null) {
			String key = EcoreUtils.getName(obj);
			NodeModel node = new ContainerNodeModel(obj.eClass().getName());
			node.setEObject(obj);
			if (obj == null || getNodeModel(_utilsObj.getParent(obj)) == null) {
				_editorModel.addChild(node);
			} else {
				((ContainerNodeModel) getNodeModel(_utilsObj.getParent(obj)))
						.addChild(node);
			}
			setUIProperties(node, key);
			_nodesObjects.add(node);
			_editorModel.putNodeModel(obj, node);
		}
	}
    /**
	 * 
	 * @param obj
	 */
    private void createNode(EObject obj)
    {
    	if (EcoreUtils.getName(obj) != null) {
			String key = EcoreUtils.getName(obj);
			NodeModel node = new NodeModel(obj.eClass().getName());
			node.setEObject(obj);
			if (obj == null || getNodeModel(_utilsObj.getParent(obj)) == null) {
				_editorModel.addChild(node);
			} else {
				((ContainerNodeModel) getNodeModel(_utilsObj.getParent(obj)))
						.addChild(node);
			}
			setUIProperties(node, key);
			_nodesObjects.add(node);
			_editorModel.putNodeModel(obj, node);
		}
    }
    /**
	 * Sets UI properties for NodeModel
	 * 
	 * @param model
	 *            NodeModel
	 * @param name
	 *            NodeModel name
	 */
    private void setUIProperties(NodeModel model, String name)
    {
    	Node nodes[] = _editorInfo.getNodeInfo();
        for (int i = 0; i < nodes.length; i++) {
            Node node = nodes[i];
            if (name.equals(node.getName())) {
            	model.setCollapsedParent(node.getCollapsedparent());
            	model.setCollapsedElement(node.getCollapsedelement());
            	model.setLocation(new Point(node.getLocation().getX(),
                                 node.getLocation().getY()));
            	model.setSize(new Dimension(node.getSize().getWidth(),
                                     node.getSize().getHeight()));
            	return;
            }
        }
        model.setCollapsedElement(false);
        model.setCollapsedParent(false);
        if(model instanceof ContainerNodeModel)
        	model.setSize(new Dimension(260, 130));
        else
        	model.setSize(new Dimension(130, 65));
        model.setLocation(UIPropertiesCreator.createNodeLocation((ContainerNodeModel) model.getParent()));
    }
    /**
     * 
     * @param obj
     */
    private void createEdge(EObject obj)
    {
    	 EObject src = _utilsObj.getSource(obj);
    	 EObject trg = _utilsObj.getTarget(obj);
    	 EdgeModel edge = new EdgeModel(obj.eClass().getName());
         edge.setEObject(obj);
         if (src != null && trg != null && EcoreUtils.getName(src) != null && EcoreUtils.getName(trg) != null) {
			setUIProperties(edge, EcoreUtils.getName(src), EcoreUtils.getName(trg));
			NodeModel source = getNodeModel(src);
			edge.setSource(source);
			NodeModel target = getNodeModel(trg);
			edge.setTarget(target);
			edge.attachSource();
			edge.attachTarget();
			_editorModel.putEdgeModel(obj, edge);
		}
    }
    /**
	 * Sets UI Properties for EdgeModel
	 * 
	 * @param model
	 *            EdgeModel
	 * @param source
	 * @param target
	 */
    private void setUIProperties(EdgeModel model, String source,
			String target) {
		Node nodes[] = _editorInfo.getNodeInfo();
		for (int i = 0; i < nodes.length; i++) {
			Node node = nodes[i];
			if (node.getName().equals(source)) {
				Edge edges[] = node.getEdgeinfo();
				for (int j = 0; j < edges.length; j++) {
					Edge edge = edges[j];
					if (target.equals(edge.getTarget())) {
						model.setSourceTerminal(edge.getSourceterminal());
						model.setTargetTerminal(edge.getTargetterminal());
						model.setCollapsedElement(edge.getCollapsedelement());
						Bend bends[] = edge.getBentpoints();
		                for (int k = 0; k < bends.length; k++) {
		                    Bend bend = bends[k];
		                    ConnectionBendpoint bendpoint = new ConnectionBendpoint();
		                    bendpoint.setRelativeDimensions(new Dimension(bend
		                            .getFirstRelative().getWidth(), bend
		                            .getFirstRelative().getHeight()), new Dimension(
		                            bend.getSecondRelative().getWidth(), bend
		                                    .getSecondRelative().getHeight()));
		                    bendpoint.setWeight(bend.getWeight());
		                    model.insertBendpoint(k, bendpoint);
		                }
		                return;
					}
				}
				return;
			}
		}
		model.setSourceTerminal("31");
		model.setTargetTerminal("E");
		model.setCollapsedElement(false);
	}
    /**
     * returns the wrapper class for EObject.
     * @param eObj EObject
     * @return NodeModel
     */
    private NodeModel getNodeModel(EObject eObj)
    {
        NodeModel node = null;
        if (eObj == null) {
            return node;
        }
        String key1 = (String) eObj.eGet(
                eObj.eClass().getEStructuralFeature(Messages.CWKEY));
        for (int i = 0; i < _nodesObjects.size(); i++) {
            node = (NodeModel) _nodesObjects.get(i);
            EObject obj = (EObject) node.getEObject();
            String key2 = (String) obj.eGet(obj.eClass().getEStructuralFeature(
            		Messages.CWKEY));
            if (key1.equals(key2)) {
                return node;
            }
        }
        //System.out.println(node);
        return node;
    }
    /**
     * saves the editor properties into xml file.
     */
    public void save()
    {
        List nodes = _editorModel.getChildren();
        try {
            EditorInfo editorInfo = new EditorInfo();
            editorInfo.setZoom(_editorModel.getZoom());
            editorInfo.setConnection(
                    _editorModel.getConnectionRouter().intValue());
            saveNodes(nodes, editorInfo);
            writeProperties(editorInfo);
        } catch (IOException e) {
            GenericeditorPlugin.LOG.error(e);
        } catch (ValidationException e) {
            GenericeditorPlugin.LOG.warn(Messages.MARSHALL_INVALIDDATA, e);
        } catch (MarshalException e) {
            GenericeditorPlugin.LOG.error(Messages.MARSHALL_ERROR, e);
        } catch (Exception e) {
            GenericeditorPlugin.LOG.error(e);
        }
    }
    /**
     * add nodes into EditorInfo and EList.
     * @param nodes List of Nodes
     * @param editorInfo Editor Info Instance
     */
    private void saveNodes(List nodes, EditorInfo editorInfo)
    {
        for (int i = 0; i < nodes.size(); i++) {
            NodeModel node = (NodeModel) nodes.get(i);
            EObject nodeobj = node.getEObject();
            String nodename = EcoreUtils.getName(nodeobj);
            Node nodeInfo = new Node();
            Size size = new Size();

            size.setHeight(node.getSize().height);
            size.setWidth(node.getSize().width);
            Location location = new Location();
            location.setX(node.getLocation().x);
            location.setY(node.getLocation().y);
            nodeInfo.setName(nodename);
            nodeInfo.setLocation(location);
            nodeInfo.setSize(size);
            nodeInfo.setCollapsedelement(node.isCollapsedElement());
            nodeInfo.setCollapsedparent(node.isCollapsedParent());
            saveEdges(node, nodeInfo);
            editorInfo.addNodeInfo(nodeInfo);
            if(node instanceof ContainerNodeModel)
            	saveNodes(((ContainerNodeModel)node).getChildren(), editorInfo);
        }
    }
    /**
     * add edges into EditorInfo and EList.
     * @param nodes      List of Nodes
     * @param editorInfo EditorInfo Instance
     */
    private void saveEdges(NodeModel node, Node nodeInfo) {
		List connections = node.getSourceConnections();
		for (int j = 0; j < connections.size(); j++) {
			EdgeModel edge = (EdgeModel) connections.get(j);
			String target = (String) EcoreUtils.getName(edge.getTarget().getEObject());
			Edge edgeInfo = new Edge();
			List bendPointList = edge.getBendpoints();
			for (int bp = 0; bp < bendPointList.size(); bp++) {
				Bend bend = new Bend();
				Size firstRelative = new Size();
				Size secondRelative = new Size();
				ConnectionBendpoint connectionBendpoint = (ConnectionBendpoint) bendPointList
						.get(bp);
				firstRelative.setHeight(connectionBendpoint
						.getFirstRelativeDimension().height);
				firstRelative.setWidth(connectionBendpoint
						.getFirstRelativeDimension().width);
				secondRelative.setHeight(connectionBendpoint
						.getSecondRelativeDimension().height);
				secondRelative.setWidth(connectionBendpoint
						.getSecondRelativeDimension().width);
				bend.setFirstRelative(firstRelative);
				bend.setSecondRelative(secondRelative);
				bend.setWeight(connectionBendpoint.getWeight());
				edgeInfo.addBentpoints(bend);
			}
			edgeInfo.setSourceterminal(edge.getSourceTerminal());
			edgeInfo.setTargetterminal(edge.getTargetTerminal());
			edgeInfo.setCollapsedelement(edge.isCollapsedElement());
			edgeInfo.setTarget(target);
			nodeInfo.addEdgeinfo(edgeInfo);
			//_eObjects.add(edgeobj);
		}
	}
    /**
     * marshall editor properties.
     * @param editorInfo EditorInfo Instance
     * @throws Exception If marshalling throws
     */
    private void writeProperties(EditorInfo editorInfo)
        throws Exception
    {
        Marshaller marshaller = new Marshaller(new FileWriter(_propFile));
        marshaller.setMarshalAsDocument(true);
        marshaller.marshal(editorInfo);
    }
}
