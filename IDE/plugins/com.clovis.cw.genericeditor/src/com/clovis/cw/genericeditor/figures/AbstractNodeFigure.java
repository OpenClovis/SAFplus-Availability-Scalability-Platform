/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.genericeditor.figures;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.ConnectionAnchor;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.RoundedRectangle;
import org.eclipse.draw2d.TitleBarBorder;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;

import com.clovis.cw.genericeditor.model.Base;

/**
 *
 * @author pushparaj
 *
 * Figure class with Anchor Points
 */
public abstract class AbstractNodeFigure extends RoundedRectangle
{

    protected Hashtable _connectionAnchors       = new Hashtable(7);

    protected Vector    _inputConnectionAnchors  = new Vector(2, 2);

    protected Vector    _outputConnectionAnchors = new Vector(2, 2);
    
    private IFigure     _conFigure;
    /**
     * Creates anchor points in Figure
     * @param figure Figure
     */
    protected void createConnectionAnchors(IFigure figure)
    {
        _conFigure = figure;
        FixedConnectionAnchor in, out;
        for (int i = 0; i < 52; i++) {
            in = new FixedConnectionAnchor(figure);
            out = new FixedConnectionAnchor(figure);
            setOutputConnectionAnchor(i, out);
            setInputConnectionAnchor(i, in);
            _outputConnectionAnchors.addElement(out);
            _inputConnectionAnchors.addElement(in);
        }
    }
    /**
     * Gets Input Connection Anchor
     * @param i index
     * @return Connection Anchor
     */
    protected FixedConnectionAnchor getInputConnectionAnchor(int i)
    {
        return (FixedConnectionAnchor) _connectionAnchors
                .get(Base.TERMINALS_IN[i]);
    }

    /**
     * @see org.eclipse.gef.handles.HandleBounds#getHandleBounds()
     */
    public Rectangle getHandleBounds()
    {
        return getBounds().getCropped(new Insets(2, 0, 2, 0));
    }
    /**
     * Returns Output Connection Anchor
     * @param i index
     * @return Connection Anchor
     */
    protected FixedConnectionAnchor getOutputConnectionAnchor(int i)
    {
        return (FixedConnectionAnchor) _connectionAnchors
                .get(Base.TERMINALS_OUT[i]);
    }
    /**
     * @see org.eclipse.draw2d.IFigure#getPreferredSize(int, int)
     */
    public Dimension getPreferredSize(int w, int h)
    {
        Dimension prefSize = super.getPreferredSize(w, h);
        Dimension defaultSize = new Dimension(100, 100);
        prefSize.union(defaultSize);
        return prefSize;
    }
    /**
     * Creates Input Connection anchors
     * @param size size of the Figure
     */
    protected void layoutInConnectionAnchors(Dimension size)
    {
        for (int i = 0; i < 9; i++) {
        	getInputConnectionAnchor(i).offsetH = (size.width / 10)
                                                      * (9 - i);
        }
        for (int i = 0; i < 17; i++) {
            getInputConnectionAnchor(i + 9).offsetV = (size.height / 18)
                                                      * (i + 1);
        }
        for (int i = 0; i < 9; i++) {
            getInputConnectionAnchor(i + 26).offsetH = (size.width / 10)
                                                       * (i + 1);
            getInputConnectionAnchor(i + 26).offsetV = (size.height);
        }
        for (int i = 0; i < 17; i++) {
            getInputConnectionAnchor(i + 35).offsetV = (size.height / 18)
                                                       * (17 - i);
            getInputConnectionAnchor(i + 35).offsetH = size.width;
        }
    }
    /**
     * Creates Output Connection Anchors
     * @param size size of Figure
     */
    protected void layoutOutConnectionAnchors(Dimension size)
    {
        for (int i = 0; i < 9; i++) {
            getOutputConnectionAnchor(i).offsetH = (size.width / 10)
                                                   * (9 - i);
        }
        for (int i = 0; i < 17; i++) {
            getOutputConnectionAnchor(i + 9).offsetV = (size.height / 18)
                                                       * (i + 1);
        }
        for (int i = 0; i < 9; i++) {
            getOutputConnectionAnchor(i + 26).offsetH = (size.width / 10)
                                                        * (i + 1);
            getOutputConnectionAnchor(i + 26).offsetV = (size.height);
        }
        for (int i = 0; i < 17; i++) {
            getOutputConnectionAnchor(i + 35).offsetV = (size.height / 18)
                                                        * (17 - i);
            getOutputConnectionAnchor(i + 35).offsetH = size.width;
        }
    }
    /**
     * Creates In and Out Connection Anchors
     *
     */
    protected void layoutConnectionAnchors()
    {
        Dimension size = _conFigure.getSize();
        layoutInConnectionAnchors(size);
        layoutOutConnectionAnchors(size);
    }
    /**
     *
     * @param i index
     * @param c Connection Anchor
     */
    public void setInputConnectionAnchor(int i, ConnectionAnchor c)
    {
        _connectionAnchors.put(Base.TERMINALS_IN[i], c);
    }
    /**
     *
     * @param i index
     * @param c Connection Anchor
     */
    public void setOutputConnectionAnchor(int i, ConnectionAnchor c)
    {
        _connectionAnchors.put(Base.TERMINALS_OUT[i], c);
    }
    /**
     * @see org.eclipse.draw2d.IFigure#validate()
     */
    public void validate()
    {
        if (isValid()) {
            return;
        }
        layoutConnectionAnchors();
        super.validate();
    }
    /**
     * @see org.eclipse.draw2d.Figure#useLocalCoordinates()
     */
    protected boolean useLocalCoordinates()
    {
        return true;
    }
    /**
     * Returns ConnectionAnchor for p
     * @param p Point
     * @return ConnectionAnchor
     */
    public ConnectionAnchor connectionAnchorAt(Point p)
    {
        ConnectionAnchor closest = null;
        long min = Long.MAX_VALUE;

        Enumeration e = getSourceConnectionAnchors().elements();
        while (e.hasMoreElements()) {
            ConnectionAnchor c = (ConnectionAnchor) e.nextElement();
            Point p2 = c.getLocation(null);
            long d = p.getDistance2(p2);
            if (d < min) {
                min = d;
                closest = c;
            }
        }
        e = getTargetConnectionAnchors().elements();
        while (e.hasMoreElements()) {
            ConnectionAnchor c = (ConnectionAnchor) e.nextElement();
            Point p2 = c.getLocation(null);
            long d = p.getDistance2(p2);
            if (d < min) {
                min = d;
                closest = c;
            }
        }
        return closest;
    }
    /**
     * Returns ConnectionAnchor for terminal
     * @param terminal connection position
     * @return ConnectionAnchor for terminal
     */
    public ConnectionAnchor getConnectionAnchor(String terminal)
    {
        return (ConnectionAnchor) _connectionAnchors.get(terminal);
    }
    /**
     * Returns the name for ConnectionAnchor
     * @param c ConnectionAnchor
     * @return name
     */
    public String getConnectionAnchorName(ConnectionAnchor c)
    {
        Enumeration eenum = _connectionAnchors.keys();
        String key;
        while (eenum.hasMoreElements()) {
            key = (String) eenum.nextElement();
            if (_connectionAnchors.get(key).equals(c)) {
                return key;
            }
        }
        return null;
    }
    /**
     * Returns Source ConnectionAnchor for p
     * @param p Point
     * @return ConnectionAnchor
     */
    public ConnectionAnchor getSourceConnectionAnchorAt(Point p)
    {
        ConnectionAnchor closest = null;
        long min = Long.MAX_VALUE;

        Enumeration e = getSourceConnectionAnchors().elements();
        while (e.hasMoreElements()) {
            ConnectionAnchor c = (ConnectionAnchor) e.nextElement();
            Point p2 = c.getLocation(null);
            long d = p.getDistance2(p2);
            if (d < min) {
                min = d;
                closest = c;
            }
        }
        return closest;
    }
    /**
     * Returns Out ConnectionAnchors List
     * @return List of Connection Anchors
     */
    public Vector getSourceConnectionAnchors()
    {
        return _outputConnectionAnchors;
    }
    /**
     * Returns Target ConnectionAnchor for p
     * @param p Point
     * @return ConnectionAnchor
     */
    public ConnectionAnchor getTargetConnectionAnchorAt(Point p)
    {
        ConnectionAnchor closest = null;
        long min = Long.MAX_VALUE;
        Enumeration e = getTargetConnectionAnchors().elements();
        while (e.hasMoreElements()) {
            ConnectionAnchor c = (ConnectionAnchor) e.nextElement();
            Point p2 = c.getLocation(null);
            long d = p.getDistance2(p2);
            if (d < min) {
                min = d;
                closest = c;
            }
        }
        return closest;
    }
    /**
     * Returns Target ConnectionAnchors
     * @return List of ConnectionAnchors
     */
    public Vector getTargetConnectionAnchors()
    {
        return _inputConnectionAnchors;
    }
    /**
     * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
     */
    public void paintFigure(Graphics graphics) {
    	super.paintFigure(graphics);
    	if(getModel().isCollapsedElement())
    		setVisible(false);
    	if(getModel().isCollapsedParent()) {
    		//setForegroundColor(ColorConstants.red);
    		setBorder(new NodeBorder());
    	} else {
    		//setForegroundColor(/*ColorConstants.lightGray*/null);
    		setBorder(null);
    	}
    }
    public abstract Base getModel();
    
    class NodeBorder extends TitleBarBorder {
    	public NodeBorder() {
    		super("collapsed");
      	}
    	/**
		 * @see org.eclipse.draw2d.Border#paint(org.eclipse.draw2d.IFigure,
		 *      org.eclipse.draw2d.Graphics, org.eclipse.draw2d.geometry.Insets)
		 */
		public void paint(IFigure figure, Graphics g, Insets insets) {
			setBackgroundColor(ColorConstants.lightGray);
			setTextAlignment(PositionConstants.CENTER);
			setTextColor(ColorConstants.green);
			super.paint(figure, g, insets);
		}

	}
}
