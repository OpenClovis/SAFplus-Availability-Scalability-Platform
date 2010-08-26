package com.clovis.logtool.ui;

import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.StatusLineManager;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.window.ApplicationWindow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabFolder2Adapter;
import org.eclipse.swt.custom.CTabFolderEvent;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;

import com.clovis.logtool.message.MessageFormatter;
import com.clovis.logtool.ui.action.AboutAction;
import com.clovis.logtool.ui.action.CloseAction;
import com.clovis.logtool.ui.action.ExitAction;
import com.clovis.logtool.ui.action.FilterAction;
import com.clovis.logtool.ui.action.OpenAction;
import com.clovis.logtool.ui.action.SavedFiltersAction;
import com.clovis.logtool.ui.action.SettingsAction;
import com.clovis.logtool.ui.filter.FilterPanel;

/**
 * Main application window for the log tool.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogDisplay extends ApplicationWindow {

	/**
	 * Message Formatter Instance.
	 */
	private MessageFormatter _messageFormatter;

	/**
	 * Filter Panel Instance.
	 */
	private FilterPanel _filterPanel;

	/**
	 * Record Panel Folder Instance.
	 */
	private CTabFolder _recordPanelFolder;

	/**
	 * Navigation Panel Instance.
	 */
	private NavigationPanel _navigationPanel;

	/**
	 * Open Action for the log tool application.
	 */
	private OpenAction _openAction;

	/**
	 * Close Action for the log tool application.
	 */
	private CloseAction _closeAction;

	/**
	 * Close All Action for the log tool application.
	 */
	private CloseAction _closeAllAction;

	/**
	 * Exit Action for the log tool application.
	 */
	private ExitAction _exitAction;

	/**
	 * Filter Action for the log tool application.
	 */
	private FilterAction _filterAction;

	/**
	 * Filter Action for the log tool application.
	 */
	private SavedFiltersAction _savedFiltersAction;

	/**
	 * Settings Action for the log tool application.
	 */
	private SettingsAction _settingsAction;

	/**
	 * About Action for the log tool application.
	 */
	private AboutAction _aboutAction;

	/**
	 * Static reference for this class.
	 */
	private static LogDisplay _instance;

	/**
	 * Constructs the application window for the log tool.
	 */
	private LogDisplay() {
		super(null);
		_instance = this;
		createActions();

		addMenuBar();
		addToolBar(SWT.FLAT | SWT.WRAP);
		addStatusLine();

		try {
			Class clazz = Class
					.forName("com.clovis.logtool.message.LogMessageFormatter");
			_messageFormatter = (MessageFormatter) clazz.newInstance();
		} catch (InstantiationException e) {
			System.out.println("Class could not be loaded " + e.getMessage());
			System.exit(-1);
		} catch (IllegalAccessException e) {
			System.out.println("Class could not be loaded " + e.getMessage());
			System.exit(-1);
		} catch (ClassNotFoundException e) {
			System.out.println("Class could not be loaded " + e.getMessage());
			System.exit(-1);
		}
	}

	/**
	 * Creates the actions for this log tool application.
	 */
	private void createActions() {
		_openAction = new OpenAction();
		_closeAction = new CloseAction(false);
		_closeAllAction = new CloseAction(true);
		_exitAction = new ExitAction();

		_filterAction = new FilterAction();
		_savedFiltersAction = new SavedFiltersAction();
		_settingsAction = new SettingsAction();

		_aboutAction = new AboutAction();
	}

	/**
	 * Starts the log tool.
	 * 
	 * @param args
	 *            the command-line arguments to the log tool
	 */
	public static void main(String args[]) {
		getInstance().run();
	}

	/**
	 * Runs the application window for the log tool.
	 */
	private void run() {
		setBlockOnOpen(true);
		open();
		Display.getCurrent().dispose();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell) {
		super.configureShell(shell);

		Rectangle bounds = Display.getCurrent().getClientArea();
		shell.setBounds(0, 0, bounds.width, bounds.height);

		shell.setText("Log Tool");
		shell.setImage(new Image(shell.getDisplay(), "icons/logViewer.png"));

		setStatus("Log Tool - Open Clovis");
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#createContents(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createContents(Composite parent) {
		Composite composite = new Group(parent, SWT.SHADOW_IN);

		GridLayout compositeLayout = new GridLayout(1, false);
		compositeLayout.marginWidth = 0;
		compositeLayout.marginHeight = 0;
		composite.setLayout(compositeLayout);

		GridData compositeData = new GridData(SWT.FILL, 0, true, false);
		composite.setLayoutData(compositeData);

		_filterPanel = new FilterPanel(composite);
		_filterPanel.setLayoutData(new GridData(SWT.FILL, 0, true, false));

		_recordPanelFolder = new CTabFolder(composite, SWT.BORDER);
		_recordPanelFolder.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		_recordPanelFolder.setSimple(false);
		_recordPanelFolder.setMRUVisible(true);
/*		_recordPanelFolder.setMaximizeVisible(true);
		_recordPanelFolder.setMinimizeVisible(true);
*/
		_recordPanelFolder.addCTabFolder2Listener(new CTabFolder2Adapter() {
			public void close(CTabFolderEvent e) {
				((CTabItem) e.item).getControl().dispose();
			}
		});

		_navigationPanel = new NavigationPanel(composite);
		_navigationPanel.setLayoutData(new GridData(SWT.END, 0, false, false));

		return composite;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.ApplicationWindow#createMenuManager()
	 */
	protected MenuManager createMenuManager() {
		MenuManager menuBar = new MenuManager();
		Separator separator = new Separator();

		MenuManager fileMenu = new MenuManager("&File");
		fileMenu.add(_openAction);
		fileMenu.add(separator);
		fileMenu.add(_closeAction);
		fileMenu.add(_closeAllAction);
		fileMenu.add(separator);
		fileMenu.add(_exitAction);
		menuBar.add(fileMenu);

		MenuManager toolsMenu = new MenuManager("&Tools");
		toolsMenu.add(_filterAction);
		toolsMenu.add(_savedFiltersAction);
		toolsMenu.add(separator);
		toolsMenu.add(_settingsAction);
		menuBar.add(toolsMenu);

		MenuManager helpMenu = new MenuManager("&Help");
		helpMenu.add(_aboutAction);
		menuBar.add(helpMenu);

		return menuBar;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.ApplicationWindow#createStatusLineManager()
	 */
	protected StatusLineManager createStatusLineManager() {
		StatusLineManager statusLine = new StatusLineManager();
		return statusLine;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.ApplicationWindow#createToolBarManager(int)
	 */
	protected ToolBarManager createToolBarManager(int style) {
		ToolBarManager toolBar = new ToolBarManager(SWT.FLAT);
		Separator separator = new Separator();

		toolBar.add(_openAction);
		toolBar.add(_closeAction);
		toolBar.add(separator);

		toolBar.add(_filterAction);
		toolBar.add(_savedFiltersAction);
		toolBar.add(separator);

		toolBar.add(_exitAction);
		return toolBar;
	}

	/**
	 * Returns the Filter Panel.
	 * 
	 * @return the Filter Panel
	 */
	public FilterPanel getFilterPanel() {
		return _filterPanel;
	}

	/**
	 * Returns the Navigation Panel.
	 * 
	 * @return the Navigation Panel
	 */
	public NavigationPanel getNavigationPanel() {
		return _navigationPanel;
	}

	/**
	 * Returns the Message Formatter.
	 * 
	 * @return the Message Formatter
	 */
	public MessageFormatter getMessageFormatter() {
		return _messageFormatter;
	}

	/**
	 * Returns the static instance of this class.
	 * 
	 * @return the Log Display Instance
	 */
	public static LogDisplay getInstance() {
		return _instance != null ? _instance : new LogDisplay();
	}

	/**
	 * Returns the Record Panel Tab Folder.
	 * 
	 * @return the Record Panel Tab Folder
	 */
	public CTabFolder getRecordPanelFolder() {
		return _recordPanelFolder;
	}
}
