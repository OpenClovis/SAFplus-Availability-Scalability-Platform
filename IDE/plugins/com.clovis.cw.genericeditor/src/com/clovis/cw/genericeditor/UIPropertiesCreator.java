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

import java.util.List;

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;

import com.clovis.cw.genericeditor.model.ContainerNodeModel;
import com.clovis.cw.genericeditor.model.NodeModel;

/**
 * @author pushparaj
 *
 * This class will create location for nodes   
 */
public class UIPropertiesCreator {
    
    /**
     * returns last node property(location and size)
     * @return rect Rectangle which contains size and location
     */
    public static Rectangle getLastNodeProperty(ContainerNodeModel parentModel)
    {   
        List childrens = parentModel.getChildren();
        Rectangle rect = new Rectangle(new Point(10, 10), new Dimension(50, 50));
        int y = 0;
        for (int i = 0; i < childrens.size(); i++) {
            y = rect.getLocation().y;
            NodeModel node = (NodeModel) childrens.get(i);
            Point p = node.getLocation();
            if (p.y >= y) {
                rect = new Rectangle(p, node.getSize());
            }
        }
        return rect;
    }
    
    /**
     * Creates and returns location for node.
     * @return loc location for new node
     */
    public static Point createNodeLocation(ContainerNodeModel parentModel)
    {
        Rectangle rect = getLastNodeProperty(parentModel);
        Point loc = new Point(10, 10);
        int x = rect.getLocation().x;
        int y = rect.getLocation().y;
        if ((x + y) > 400) {
            loc.x = 10;
            loc.y = y + rect.getSize().height + 50;
        } else {
            loc.x = x + rect.getSize().width + 50;
            loc.y = y;
        }
        return loc;
    }
}
