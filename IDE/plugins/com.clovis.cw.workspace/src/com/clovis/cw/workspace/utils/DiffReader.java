/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/utils/DiffReader.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.utils;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.StringTokenizer;

import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;

/**
 * Class to read 'diff' output.
 * @author pushparaj
 *
 */
public class DiffReader {
	private InputStream _input;
	private List _addedList, _deletedList, _modifiedList;
	private IFolder _srcFolder, _nextFolder;
	public DiffReader(InputStream input, List addedList,
			List deletedList, List modifiedList, IFolder srcFolder, IFolder nextFolder) {
		_input = input;
		_addedList = addedList;
		_deletedList = deletedList;
		_modifiedList = modifiedList;
		_srcFolder = srcFolder;
		_nextFolder = nextFolder;
	}
	/**
	 * reads diff 'output' 
	 *
	 */
	public void readAndParse() {
		try {
			int read;
			StringBuffer buffer = new StringBuffer("");
			while((read = _input.read()) != -1) {
				char c = (char) read;
				if(c == '\n') {
					parseAndUpdateLists(buffer.toString());
					buffer = new StringBuffer("");
				} else {
					buffer.append(c);
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	/**
	 * parse 'diff' output
	 * @param output string needs to be parsed
	 */
	private void parseAndUpdateLists(String output) {
		if(output.startsWith("Only in")) {
			String result = output.replaceFirst("Only in ", "");
			if(result.startsWith(_srcFolder.getLocation().toOSString())) {
				updateDeletedList(result);
			} else if(result.startsWith(_nextFolder.getLocation().toOSString())) {
				updateAddedList(result);
			}
		} else if(output.startsWith("Files") && output.endsWith("differ")) {
			String result = output.replaceFirst("Files ", "");
			updateModifiedList(result);
		}
	}
	/**
	 * parse and add new file to List
	 * @param output string which contains new file name. 
	 */
	private void updateAddedList(String output) {
		StringTokenizer tokenizer = new StringTokenizer(output, ":");
		String folder = tokenizer.nextToken();
		String name = tokenizer.nextToken().trim();
		String fileName = folder.replaceFirst(_nextFolder.getLocation().toOSString(), "") + File.separator + name;
		IResource resource = _nextFolder.findMember(fileName);
		if(resource.getType() == IResource.FILE) {
			_addedList.add(fileName);
		} else if(resource.getType() == IResource.FOLDER) {
			includeFolderInList(_nextFolder, fileName, _addedList);
		}
	}
	/**
	 * parse and add old(removed) file to List
	 * @param output string which contains old file name. 
	 */
	private void updateDeletedList(String output) {
		StringTokenizer tokenizer = new StringTokenizer(output, ":");
		String folder = tokenizer.nextToken();
		String name = tokenizer.nextToken().trim();
		String fileName = folder.replaceFirst(_srcFolder.getLocation().toOSString(), "") + File.separator + name;
		IResource resource = _srcFolder.findMember(fileName);
		if(resource.getType() == IResource.FILE) {
			_deletedList.add(fileName);
		} else if(resource.getType() == IResource.FOLDER) {
			_deletedList.add(fileName);
			includeFolderInList(_srcFolder, fileName, _deletedList);
		}		
	}
	/**
	 * parse and add modified file to List
	 * @param output string which contains modified file name. 
	 */
	private void updateModifiedList(String output) {
		String result = new StringTokenizer(output, " ").nextToken();
		String file = result.replaceFirst(_srcFolder.getLocation().toOSString(), "");
		_modifiedList.add(file);
	}
	/**
	 * add files to added or removed list
	 * @param distFolder
	 * @param fileName
	 * @param list
	 */
	private void includeFolderInList(IFolder distFolder, String fileName, List list) {
		IFolder folder = distFolder.getFolder(fileName);
		try {
			IResource resources[] = folder.members();
			for (int i = 0; i < resources.length; i++) {
				IResource res = resources[i];
				if(res.getType() == IResource.FILE) {
					list.add(fileName + File.separator + res.getName());
				} else if(res.getType() == IResource.FOLDER) {
					includeFolderInList(distFolder, fileName + File.separator + res.getName(), list);
				}
			}
		} catch (CoreException e) {
			e.printStackTrace();
		}
	}
}
