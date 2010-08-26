/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor.model;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;


/**
 *
 * @author pushparaj
 *
 * Base class for all Nodes
 */
public abstract class Base extends IElement
{

    public static String ID_SIZE        = "size";

    public static String ID_LOCATION     = "location";

    public static final String TERMINALS_OUT[] = new String[]{"1", "2", "3", "4",
            "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
            "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27",
            "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38",
            "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
            "50", "51", "52"};

    public static final String TERMINALS_IN[]  = new String[]{"A", "B", "C", "D",
            "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q",
            "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "a", "b", "c", "d",
            "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q",
            "r", "s", "t", "u", "v", "w", "x", "y", "z"};

    protected Hashtable  _inputsHash      = new Hashtable(7);

    protected Vector     _outputsHash     = new Vector(4, 4);

    protected Point      _locationPoint   = new Point(0, 0);

    protected Dimension  _sizeDim         = new Dimension(110, 130);
       
    private Base         _parentModel;
    
    private boolean      _isCollapsedParent;
    
    private Point		 _oldLocation	  = null;	
    
    private Vector _lsConnectionsList, _rsConnectionsList, _ltConnectionsList,
			_rtConnectionsList, _csConnectionsList, _ctConnectionsList;
    
    /**
     * constructor
     *
     */
    public Base()
    {
        _sizeDim.width = 130;
        _sizeDim.height = 65;
        _locationPoint.x = 20;
        _locationPoint.y = 20;
        _lsConnectionsList = new Vector();
        _rsConnectionsList = new Vector();
        _ltConnectionsList = new Vector();
        _rtConnectionsList = new Vector();
        _csConnectionsList = new Vector();
        _ctConnectionsList = new Vector();
    }

    /**
     * returns connections list for node.
     * @return vector
     */
    public Vector getConnections()
    {
        Vector v = (Vector) _outputsHash.clone();
        Enumeration ins = _inputsHash.elements();
        while (ins.hasMoreElements()) {
            v.addElement(ins.nextElement());
        }
        return v;
    }

    /**
     * returns location.
     * @return locationPoint
     */
    public Point getLocation()
    {
        return _locationPoint;
    }
    /**
     * Returns Previous location point
     * @return Old Location
     */
    public Point getOldLocation()
    {
    	return _oldLocation;
    }
    /**
     * returns size.
     * @return sizeDim
     */
    public Dimension getSize()
    {
    	return _sizeDim;
    }

    /**
     * returns source connections for node.
     * @return Vector
     */
    public Vector getSourceConnections()
    {
        return (Vector) _outputsHash.clone();
    }

    /**
     * returns target connections for node.
     * @return Vetcor
     */
    public Vector getTargetConnections()
    {
        Enumeration enums = _inputsHash.elements();
        Vector v = new Vector(_inputsHash.size());
        while (enums.hasMoreElements()) {
            v.addElement(enums.nextElement());
        }
        return v;
    }

    /**
     * set the loacation.
     * @param p Point
     */
    public void setLocation(Point p)
    {
    	if(_oldLocation == null)
    		setOldLocation(p);
    	else
    		setOldLocation(_locationPoint);
    	if (_locationPoint.equals(p)) {
            return;
        }
    	_locationPoint = p;
        //_listeners.firePropertyChange("location", null, p);
        _listeners.firePropertyChange("parent", null, getParent());
    }
    /**
     * Set Previous Location whem the location is changed
     * @param p Point
     */
    private void setOldLocation(Point p)
    {
    	_oldLocation = p;
    }
    /**
     * set the size.
     * @param d Dimension
     */
    public void setSize(Dimension d)
    {
        if (_sizeDim.equals(d)) {
            return;
        }
        _sizeDim = d;
        _listeners.firePropertyChange("size", null, _sizeDim);
    }
    /**
     * set the parent for nodes.
     * @param model base
     */
    public void setParent(Base model)
    {
        _parentModel = model;
        _listeners.firePropertyChange("parent", null, _parentModel);
    }
    /**
     * set the selection for node.
     */
    public void setSelection()
    {
        _listeners.firePropertyChange("selection", null, null);
    }
    /**
     * Add this node in selection list
     *
     */
    public void appendSelection()
    {
    	_listeners.firePropertyChange("appendselection", null, null);
    }
    /**
     * set the focus for node.
     */
    public void setFocus()
    {
    	_listeners.firePropertyChange("focus", null, null);
    }
    /**
     * Updates UI
     *
     */
    public void refreshUI()
    {
    	_listeners.firePropertyChange("refresh", null, null);
    }
    /**
     * returns parent model.
     * @return perentModel
     */
    public Base getParent()
    {
        return _parentModel;
    }
    /**
     * Set flag for collapse and expand
     * @param collapse flag for expand and collapse
     */
    public void setCollapsedParent(boolean collapse)
    {
    	_isCollapsedParent = collapse;
    }
    /**
     * Returnns flag for exapnd and collapse
     * @return flag
     */
    public boolean isCollapsedParent()
    {
    	return _isCollapsedParent;
    }
    /**
     * Adding Source Connections according to the Target Locations
     * @param edge EdgeModel
     */
    public void addSourceConnections(EdgeModel edge)
    {
    	// This code needs to be cleaned.
    	int x = getLocation().x;
    	int end = x + getSize().width;
    	NodeModel targetNode = edge.getTarget();
    	int targetx = targetNode.getLocation().x;
    	int targetend = targetx + targetNode.getSize().width; 
    	if(targetx >= x && targetend <= end) {
    		//edge.setSourceTerminal("31");
    		if(_csConnectionsList.size() == 0) {
    			_csConnectionsList.insertElementAt(edge, 0);
    			return;
    		}
    		if(_csConnectionsList.size() == 1) {
    			EdgeModel emodel = (EdgeModel) _csConnectionsList.get(0);
    			NodeModel nmodel = emodel.getTarget();
    			if(targetx < nmodel.getLocation().x) {
    				_csConnectionsList.insertElementAt(edge,0);
    			} else {
    				_csConnectionsList.insertElementAt(edge,1);
    			}
    			return;
    		}
    		int count = _csConnectionsList.size() - 1;
    		EdgeModel emodel1 = null;
			NodeModel nmodel1 = null;
			EdgeModel emodel2 = null;
			NodeModel nmodel2 = null;
    		for (int i = 0; i < count; i++) {
    			emodel1 = (EdgeModel) _csConnectionsList.get(i);
    			nmodel1 = emodel1.getTarget();
    			emodel2 = (EdgeModel) _csConnectionsList.get(i + 1);
    			nmodel2 = emodel2.getTarget();
    			if(targetx > nmodel1.getLocation().x
						&& targetx < nmodel2.getLocation().x) {
    				_csConnectionsList.insertElementAt(edge,i + 1);
    				return;
				}
    		}
    		emodel1 = (EdgeModel) _csConnectionsList.get(0);
			nmodel1 = emodel1.getTarget();
			emodel2 = (EdgeModel) _csConnectionsList.get(count);
			nmodel2 = emodel2.getTarget();
			if (targetx <= nmodel1.getLocation().x) {
				_csConnectionsList.insertElementAt(edge,0);
				return;
			} else if(targetx >= nmodel2.getLocation().x) {
				_csConnectionsList.insertElementAt(edge, count + 1);
				return;
			} else {
				_csConnectionsList.insertElementAt(edge, count + 1);
			}
    	} else if (x < targetx) {
    		if(_rsConnectionsList.size() == 0) {
    			_rsConnectionsList.insertElementAt(edge, 0);
    			return;
    		}
    		if(_rsConnectionsList.size() == 1) {
    			EdgeModel emodel = (EdgeModel) _rsConnectionsList.get(0);
    			NodeModel nmodel = emodel.getTarget();
    			if(targetx < nmodel.getLocation().x) {
    				_rsConnectionsList.insertElementAt(edge,0);
    			} else {
    				_rsConnectionsList.insertElementAt(edge,1);
    			}
    			return;
    		}
    		int count = _rsConnectionsList.size() - 1;
    		EdgeModel emodel1 = null;
			NodeModel nmodel1 = null;
			EdgeModel emodel2 = null;
			NodeModel nmodel2 = null;
    		for (int i = 0; i < count; i++) {
    			emodel1 = (EdgeModel) _rsConnectionsList.get(i);
    			nmodel1 = emodel1.getTarget();
    			emodel2 = (EdgeModel) _rsConnectionsList.get(i + 1);
    			nmodel2 = emodel2.getTarget();
    			if(targetx > nmodel1.getLocation().x
						&& targetx < nmodel2.getLocation().x) {
    				_rsConnectionsList.insertElementAt(edge,i + 1);
    				return;
				}
    		}
    		emodel1 = (EdgeModel) _rsConnectionsList.get(0);
			nmodel1 = emodel1.getTarget();
			emodel2 = (EdgeModel) _rsConnectionsList.get(count);
			nmodel2 = emodel2.getTarget();
			if (targetx <= nmodel1.getLocation().x) {
				_rsConnectionsList.insertElementAt(edge,0);
				return;
			} else if(targetx >= nmodel2.getLocation().x) {
				_rsConnectionsList.insertElementAt(edge, count + 1);
				return;
			} else {
				_rsConnectionsList.insertElementAt(edge, count + 1);
			}
    	} else {
    		if(_lsConnectionsList.size() == 0) {
    			_lsConnectionsList.insertElementAt(edge, 0);
    			return;
    		}
    		if(_lsConnectionsList.size() == 1) {
    			EdgeModel emodel = (EdgeModel) _lsConnectionsList.get(0);
    			NodeModel nmodel = emodel.getTarget();
    			if(targetx <= nmodel.getLocation().x) {
    				_lsConnectionsList.insertElementAt(edge,1);
    			} else {
    				_lsConnectionsList.insertElementAt(edge,0);
    			}
    			return;
    		}
    		int count = _lsConnectionsList.size() - 1;
    		EdgeModel emodel1 = null;
			NodeModel nmodel1 = null;
			EdgeModel emodel2 = null;
			NodeModel nmodel2 = null;
    		for (int i =  0; i < count; i++) {
    			emodel1 = (EdgeModel) _lsConnectionsList.get(i);
    			nmodel1 = emodel1.getTarget();
    			emodel2 = (EdgeModel) _lsConnectionsList.get(i + 1);
    			nmodel2 = emodel2.getTarget();
    			if(targetx < nmodel1.getLocation().x
						&& targetx > nmodel2.getLocation().x) {
    				_lsConnectionsList.insertElementAt(edge,i + 1);
    				return;
				}
    		}
    		emodel1 = (EdgeModel) _lsConnectionsList.get(0);
			nmodel1 = emodel1.getTarget();
			emodel2 = (EdgeModel) _lsConnectionsList.get(count);
			nmodel2 = emodel2.getTarget();
			if (targetx >= nmodel1.getLocation().x) {
				_lsConnectionsList.insertElementAt(edge,0);
				return;
			} else if(targetx <= nmodel2.getLocation().x) {
				_lsConnectionsList.insertElementAt(edge, count + 1);
				return;
			} else {
				_lsConnectionsList.insertElementAt(edge, count + 1);
			}
    	}
    }
    public Vector getLeftSourceConnectionsList()
    {
    	return _lsConnectionsList;
    }
    public Vector getRightSourceConnectionsList()
    {
    	return _rsConnectionsList;
    }
    /**
     * Adding Target Connections according to the Source Locations
     * @param edge EdgeModel
     */
    public void addTargetConnections(EdgeModel edge)
    {
    	// This code needs to be cleaned.
    	int x = getLocation().x;
    	int end = x + getSize().width;
    	NodeModel targetNode = edge.getSource();
    	int targetx = targetNode.getLocation().x;
    	int targetend = targetx + targetNode.getSize().width;
    	if(targetx >= x && targetend <= end) {
    		//edge.setTargetTerminal("E");
    		if(_ctConnectionsList.size() == 0) {
    			_ctConnectionsList.insertElementAt(edge, 0);
    			return;
    		}
    		if(_ctConnectionsList.size() == 1) {
    			EdgeModel emodel = (EdgeModel) _ctConnectionsList.get(0);
    			NodeModel nmodel = emodel.getTarget();
    			if(targetx < nmodel.getLocation().x) {
    				_ctConnectionsList.insertElementAt(edge,0);
    			} else {
    				_ctConnectionsList.insertElementAt(edge,1);
    			}
    			return;
    		}
    		int count = _ctConnectionsList.size() - 1;
    		EdgeModel emodel1 = null;
			NodeModel nmodel1 = null;
			EdgeModel emodel2 = null;
			NodeModel nmodel2 = null;
    		for (int i = 0; i < count; i++) {
    			emodel1 = (EdgeModel) _ctConnectionsList.get(i);
    			nmodel1 = emodel1.getTarget();
    			emodel2 = (EdgeModel) _ctConnectionsList.get(i + 1);
    			nmodel2 = emodel2.getTarget();
    			if(targetx > nmodel1.getLocation().x
						&& targetx < nmodel2.getLocation().x) {
    				_ctConnectionsList.insertElementAt(edge,i + 1);
    				return;
				}
    		}
    		emodel1 = (EdgeModel) _ctConnectionsList.get(0);
			nmodel1 = emodel1.getTarget();
			emodel2 = (EdgeModel) _ctConnectionsList.get(count);
			nmodel2 = emodel2.getTarget();
			if (targetx <= nmodel1.getLocation().x) {
				_ctConnectionsList.insertElementAt(edge,0);
				return;
			} else if(targetx >= nmodel2.getLocation().x) {
				_ctConnectionsList.insertElementAt(edge, count + 1);
				return;
			} else {
				_ctConnectionsList.insertElementAt(edge, count + 1);
			}
    	} else if (x < targetx) {
    		if(_rtConnectionsList.size() == 0) {
    			_rtConnectionsList.insertElementAt(edge, 0);
    			return;
    		}
    		if(_rtConnectionsList.size() == 1) {
    			EdgeModel emodel = (EdgeModel) _rtConnectionsList.get(0);
    			NodeModel nmodel = emodel.getSource();
    			if(targetx < nmodel.getLocation().x) {
    				_rtConnectionsList.insertElementAt(edge,0);
    			} else {
    				_rtConnectionsList.insertElementAt(edge,1);
    			}
    			return;
    		}
    		int count = _rtConnectionsList.size() - 1;
    		EdgeModel emodel1 = null;
			NodeModel nmodel1 = null;
			EdgeModel emodel2 = null;
			NodeModel nmodel2 = null;
    		for (int i = 0; i < count; i++) {
    			emodel1 = (EdgeModel) _rtConnectionsList.get(i);
    			nmodel1 = emodel1.getSource();
    			emodel2 = (EdgeModel) _rtConnectionsList.get(i + 1);
    			nmodel2 = emodel2.getSource();
    			if(targetx > nmodel1.getLocation().x
						&& targetx < nmodel2.getLocation().x) {
    				_rtConnectionsList.insertElementAt(edge,i + 1);
    				return;
				}
    		}
    		emodel1 = (EdgeModel) _rtConnectionsList.get(0);
			nmodel1 = emodel1.getSource();
			emodel2 = (EdgeModel) _rtConnectionsList.get(count);
			nmodel2 = emodel2.getSource();
			if (targetx <= nmodel1.getLocation().x) {
				_rtConnectionsList.insertElementAt(edge,0);
				return;
			} else if(targetx >= nmodel2.getLocation().x) {
				_rtConnectionsList.insertElementAt(edge, count + 1);
				return;
			} else {
				_rtConnectionsList.insertElementAt(edge, count + 1);
			}
    	} else {
    		if(_ltConnectionsList.size() == 0) {
    			_ltConnectionsList.insertElementAt(edge, 0);
    			return;
    		}
    		if(_ltConnectionsList.size() == 1) {
    			EdgeModel emodel = (EdgeModel) _ltConnectionsList.get(0);
    			NodeModel nmodel = emodel.getSource();
    			if(targetx <= nmodel.getLocation().x) {
    				_ltConnectionsList.insertElementAt(edge,1);
    			} else {
    				_ltConnectionsList.insertElementAt(edge,0);
    			}
    			return;
    		}
    		int count = _ltConnectionsList.size() - 1;
    		EdgeModel emodel1 = null;
			NodeModel nmodel1 = null;
			EdgeModel emodel2 = null;
			NodeModel nmodel2 = null;
    		for (int i =  0; i < count; i++) {
    			emodel1 = (EdgeModel) _ltConnectionsList.get(i);
    			nmodel1 = emodel1.getSource();
    			emodel2 = (EdgeModel) _ltConnectionsList.get(i + 1);
    			nmodel2 = emodel2.getSource();
    			if(targetx < nmodel1.getLocation().x
						&& targetx > nmodel2.getLocation().x) {
    				_ltConnectionsList.insertElementAt(edge,i + 1);
    				return;
				}
    		}
    		emodel1 = (EdgeModel) _ltConnectionsList.get(0);
			nmodel1 = emodel1.getSource();
			emodel2 = (EdgeModel) _ltConnectionsList.get(count);
			nmodel2 = emodel2.getSource();
			if (targetx >= nmodel1.getLocation().x) {
				_ltConnectionsList.insertElementAt(edge,0);
				return;
			} else if(targetx <= nmodel2.getLocation().x) {
				_ltConnectionsList.insertElementAt(edge, count + 1);
				return;
			} else {
				_ltConnectionsList.insertElementAt(edge, count + 1);
			}
    	}
    }
    public Vector getLeftTargetConnectionsList()
    {
    	return _ltConnectionsList;
    }
    public Vector getRightTargetConnectionsList()
    {
    	return _rtConnectionsList;
    }
    public Vector getCentreTargetConnectionsList()
    {
    	return _ctConnectionsList;
    }
    public Vector getCentreSourceConnectionsList()
    {
    	return _csConnectionsList;
    }
}
