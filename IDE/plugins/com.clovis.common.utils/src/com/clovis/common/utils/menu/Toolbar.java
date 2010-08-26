/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/Toolbar.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.menu;


import java.util.HashMap;
import java.util.Iterator;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import com.clovis.common.utils.menu.gen.Menu;
import com.clovis.common.utils.menu.gen.MenuBar;
import com.clovis.common.utils.menu.gen.types.OrientationType;
/**
 * @author shubhada
 *
 * Toolbar Provider for the dialog.
 */
public class Toolbar extends Composite implements IEnvironmentListener
{
	
	private HashMap     _actionButtonMap = new HashMap();
	/**
	 * Constructor.
	 * @param builder Menubuilder
	 */
	public Toolbar(Composite parent, MenuBuilder builder, int index)
	{
		super(parent, SWT.NONE);
		MenuBar menubar   = builder.getMenuBars().getMenuBar(index);
		int orientation = menubar.getOrientation().getType();
		int rowType = (orientation == OrientationType.HORIZONTAL_TYPE) ?
					SWT.HORIZONTAL : SWT.VERTICAL;
		RowLayout rowLayout = new RowLayout(rowType);
		rowLayout.fill = true;
		setLayout(rowLayout);
		for (int i = 0; i < menubar.getMenu().length; i++) {
			addButton(builder.getEnvironment(), menubar.getMenu(i));
		}
		EnvironmentNotifier notifier = builder.getEnvironment().getNotifier();
		if (notifier != null) {
			notifier.addListener(this);
		}
	}
	/**
	 * Change event from listener.
	 * @param obj Object Changed
	 */
	public void valueChanged(Object obj) 
	{
		Iterator iterator = _actionButtonMap.keySet().iterator();
		while (iterator.hasNext()) {
			MenuAction action = (MenuAction) iterator.next();
			Button button = (Button) _actionButtonMap.get(action);
			button.setEnabled(action.isEnabled());
		}	
	}	
	/**
	 * Add Button to the Toolbar
	 * @param menu
	 */
	private void addButton(Environment env, Menu menu)
	{
		final MenuAction action = new MenuAction(env, menu);
		if (action.isVisible()) {
			Button button = new Button(this, SWT.PUSH);
			button.setEnabled(action.isEnabled());
			button.setText(menu.getLabel());
			//button.setImage(menu.getIcon());
			button.addSelectionListener(new SelectionAdapter() {       	
				public void widgetSelected(SelectionEvent e) { action.run(); }
			});
			_actionButtonMap.put(action, button);
		}
	}

}
