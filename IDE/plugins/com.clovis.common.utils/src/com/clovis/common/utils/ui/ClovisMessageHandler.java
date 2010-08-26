package com.clovis.common.utils.ui;

import java.io.IOException;

import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.UtilsPlugin;

/**
 * 
 * @author shubhada
 *
 * Generic Error class which takes care of displaying
 * the Error both in UI mode and commandline mode 
 */
public class ClovisMessageHandler
{
	/**
	 * Method to display the popup error
	 * 
	 * @param shell - Shell
	 * @param title - dialog title
	 * @param msg - message to be displayed
	 */
	public static void displayPopupError(Shell shell, String title, String msg)
	{
		
		if (!UtilsPlugin.isCmdToolRunning()) {
			MessageDialog.openError(shell, title, msg);
		} else {
			System.err.println("\nERROR: " + msg);
		}
	}
	/**
	 * Method to display the popup warning
	 * 
	 * @param shell - Shell
	 * @param title - dialog title
	 * @param msg - message to be displayed
	 */
	public static void displayPopupWarning(Shell shell, String title, String msg)
	{
		
		if (!UtilsPlugin.isCmdToolRunning()) {
			MessageDialog.openWarning(shell, title, msg);
		} else {
			System.out.println("\nWARNING: " + msg);
		}
	}
	/**
	 * Method to display the popup Information
	 * 
	 * @param shell - Shell
	 * @param title - dialog title
	 * @param msg - message to be displayed
	 */
	public static void displayPopupInformation(Shell shell, String title, String msg)
	{
		
		if (!UtilsPlugin.isCmdToolRunning()) {
			MessageDialog.openInformation(shell, title, msg);
		} else {
			System.out.println("\nINFORMATION: " + msg);
		}
	}
	/**
	 * Method to display the popup Question
	 * 
	 * @param shell - Shell
	 * @param title - dialog title
	 * @param msg - message to be displayed
	 * @throws IOException 
	 */
	public static boolean displayPopupQuestion(Shell shell, String title, String msg) throws IOException
	{
		boolean retVal = false;
		if (!UtilsPlugin.isCmdToolRunning()) {
			retVal = MessageDialog.openQuestion(shell, title, msg);
		} else {
			System.out.println("\nQUESTION: " + msg + " (y/n)");
			char ans = (char) System.in.read();
			if (ans == 'y') {
				retVal = true;
			} 
		}
		return retVal;
	}
	/**
	 * Displays the error message
	 * 
	 * @param dialog - Dialog in which msg to be displayed
	 * @param msg - Message to be displayed
	 */
	public static void displayDialogErrorMessage(TitleAreaDialog dialog, String msg)
	{
		if (!UtilsPlugin.isCmdToolRunning()) {
			dialog.setMessage(msg, IMessageProvider.ERROR);
		} else {
			System.err.println("\nERROR: " + msg);
		}
	}
	
	/**
	 * Displays the warning message
	 * 
	 * @param dialog - Dialog in which msg to be displayed
	 * @param msg - Message to be displayed
	 */
	public static void displayDialogWarningMessage(TitleAreaDialog dialog, String msg)
	{
		if (!UtilsPlugin.isCmdToolRunning()) {
			dialog.setMessage(msg, IMessageProvider.WARNING);
		} else {
			System.err.println("\nWARNING: " + msg);
		}
	}
	/**
	 * Displays the information message
	 * 
	 * @param dialog - Dialog in which msg to be displayed
	 * @param msg - Message to be displayed
	 */
	public static void displayDialogInformationMessage(TitleAreaDialog dialog, String msg)
	{
		if (!UtilsPlugin.isCmdToolRunning()) {
			dialog.setMessage(msg, IMessageProvider.INFORMATION);
		} else {
			System.err.println("\nINFORMATION: " + msg);
		}
	}
	/**
	 * Displays the error message
	 * 
	 * @param dialog - Dialog in which msg to be displayed
	 * @param msg - Message to be displayed
	 */
	public static void displayDialogErrorMessage(PreferenceDialog dialog, String msg)
	{
		if (!UtilsPlugin.isCmdToolRunning()) {
			dialog.setMessage(msg, IMessageProvider.ERROR);
		} else {
			System.err.println("\nERROR: " + msg);
		}
	}
	
	/**
	 * Displays the warning message
	 * 
	 * @param dialog - Dialog in which msg to be displayed
	 * @param msg - Message to be displayed
	 */
	public static void displayDialogWarningMessage(PreferenceDialog dialog, String msg)
	{
		if (!UtilsPlugin.isCmdToolRunning()) {
			dialog.setMessage(msg, IMessageProvider.WARNING);
		} else {
			System.err.println("\nWARNING: " + msg);
		}
	}
	/**
	 * Displays the information message
	 * 
	 * @param dialog - Dialog in which msg to be displayed
	 * @param msg - Message to be displayed
	 */
	public static void displayDialogInformationMessage(PreferenceDialog dialog, String msg)
	{
		if (!UtilsPlugin.isCmdToolRunning()) {
			dialog.setMessage(msg, IMessageProvider.INFORMATION);
		} else {
			System.err.println("\nINFORMATION: " + msg);
		}
	}
	
}
