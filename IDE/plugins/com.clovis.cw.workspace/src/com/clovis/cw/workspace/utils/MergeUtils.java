/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/utils/MergeUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/03/28 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.utils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;

/**
 * Utils clas for Merge
 * @author pushparaj
 *
 */
public class MergeUtils {
	/**
	 * Returns Map which contains modify,add and delete file names List
	 * @param firstFolder First Folder for merge
	 * @param secondFolder Second Folder for merge
	 * @return Map
	 */
	public static Map getModifiedFiles(IFolder firstFolder, IFolder secondFolder) {
		Map map = new HashMap();
		List modifiedList = new ArrayList();
		List addedList = new ArrayList();
		List deletedList = new ArrayList();
		if (firstFolder.exists() && secondFolder.exists()) {
			Runtime runtime = Runtime.getRuntime();
			try {
				Process proc = runtime.exec("diff --brief -r -b --exclude=*.xml " + firstFolder.getLocation().toOSString() + " "
						+ secondFolder.getLocation().toOSString());
				InputStream input = proc.getInputStream();
				InputStream	error = proc.getErrorStream();
				DiffReader reader = new DiffReader(input, addedList, deletedList, modifiedList,
						firstFolder, secondFolder);
				reader.readAndParse();
				new ErrorStreamReader(error);
				proc.waitFor();
				input.close();
				error.close();
			} catch (IOException e) {
				e.printStackTrace();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		map.put("add", addedList);
		map.put("delete", deletedList);
		map.put("modify", modifiedList);
		return map;
	}
	/**
	 * Returns FileName
	 * @param str String which is read from Stream
	 * @return File Name
	 */
	private static String getFileName(String str) {
		String fileName = null;
		StringTokenizer tokenizer = new StringTokenizer(str, " ");
		while(tokenizer.hasMoreElements()) {
			fileName = tokenizer.nextToken();
		}
		return fileName;
	}
	/**
	 * Merge files from 1stFolder to 2ndFolder 
	 * @param files Files list which are needs to be merged 
	 * @param namesMap map for old and new file names
	 * @param firstFolder path for 1stFolder
	 * @param secondFolder path for 2ndFolder
	 * @param baseFolder path for baseFolder
	 */
	/*public static boolean mergeFiles(String mergeScriptFile, String files[], Map namesMap, IFolder firstFolder, IFolder secondFolder,
			IFolder baseFolder) {
		for (int i = 0; i < files.length; i++) {
			String file = files[i];
			File firstFile = null;
			if(namesMap.get(file) == null) {
				firstFile = firstFolder.getLocation().append(file).toFile();
			} else {
				firstFile = firstFolder.getLocation().append((String)namesMap.get(file)).toFile();
			}
			File secondFile = secondFolder.getLocation().append(file)
					.toFile();
			File resultFile = firstFolder.getLocation().append(file).toFile();
			if(!resultFile.getParentFile().exists()) {
				resultFile.getParentFile().mkdirs();
			}
			File baseFile = null;
			if(namesMap.get(file) == null) {
				baseFile = baseFolder.getLocation().append(file)
				.toFile();
			} else {
				baseFile = baseFolder.getLocation().append((String)namesMap.get(file))
					.toFile();
			}
			if (firstFile.exists() && secondFile.exists()
					&& baseFile.exists()) {
				if(!mergeFile(mergeScriptFile, firstFile.getAbsolutePath(), secondFile.getAbsolutePath(), resultFile.getAbsolutePath(), baseFile.getAbsolutePath())) {
					return false;
				}
				if(!resultFile.exists()) {
					IFile newFile = firstFolder.getFile(file);
					try {
						newFile.create(new FileInputStream(secondFile), true, null);
					} catch (FileNotFoundException e) {
						e.printStackTrace();
					} catch (CoreException e) {
						e.printStackTrace();
					}
				}
			}
		}
		return true;
	}*/
	public static void mergeFiles(Object files[], Map namesMap,
			IFolder firstFolder, IFolder secondFolder) {
		for (int i = 0; i < files.length; i++) {
			String file = (String) files[i];
			File firstFile = null;
			if (namesMap.get(file) == null) {
				firstFile = firstFolder.getLocation().append(file).toFile();
			} else {
				firstFile = firstFolder.getLocation().append(
						(String) namesMap.get(file)).toFile();
			}
			File secondFile = secondFolder.getLocation().append(file).toFile();
			File resultFile = firstFolder.getLocation().append(file).toFile();
			if (!resultFile.getParentFile().exists()) {
				resultFile.getParentFile().mkdirs();
			}
			if (firstFile.exists() && secondFile.exists()) {
				mergeFile(firstFile, secondFile, resultFile);
				if (!resultFile.exists()) {
					IFile newFile = firstFolder.getFile(file);
					try {
						newFile.create(new FileInputStream(secondFile), true,
								null);
					} catch (FileNotFoundException e) {
						e.printStackTrace();
					} catch (CoreException e) {
						e.printStackTrace();
					}
				}
			}
		}
	}
	/**
	 * Merge file
	 * @param firstFile
	 * @param secondFile
	 * @param baseFile
	 */
	/*public static boolean mergeFile(String mergeScriptFile, String firstFile, String secondFile, String resultFile,
			String baseFile) {
		try {
			Process proc = Runtime.getRuntime().exec(mergeScriptFile + " " + firstFile + " " + secondFile + " " + baseFile + " " + resultFile);
			proc.waitFor();
		} catch (IOException e) {
			System.err.println("Invalid merge script");
			return false;
		} catch (InterruptedException e) {
			System.err.println("Invalid merge script");
			return false;
		}
		return true;
	}*/
	
	/**
	 * Merge Application code from old file to new file
	 */
	public static boolean mergeFile(File firstFile, File secondFile, File resultFile) {
//		mergeApplicationCode(firstFile, secondFile, resultFile);
		performCodeMerge(firstFile, secondFile, resultFile);
		return true;
	}
	/**
	 * Creates Reader and Writer and update the result file contents.
	 * @param oldFile Old File
	 * @param newFile New File
	 */
	public static void mergeApplicationCode(File oldFile, File newFile, File resultFile) {
		StringBuffer resultFileContents = new StringBuffer(""); 
		try {
			RandomAccessFile reader1 = new RandomAccessFile(oldFile, "r");
			RandomAccessFile reader2 = new RandomAccessFile(newFile, "r");
			boolean isValid = true;
			String readLine = null;
			while ((readLine = reader1.readLine()) != null) {
				if (readLine.contains(MergeConstants.BEGIN_TAG)) {
					StringBuffer userCode = readCodeWithInTheBlock(reader1);
					if(!replacegeneratedCodeWithUserCode(reader2, userCode, resultFileContents)){
						isValid = false;
					}
				}
			}
			/** This part of code needs to be cleaned. For creation and deletion should
			 * use eclipse apis**/
			if(!isValid) {
				/*resultFileContents = new StringBuffer("");
				reader2.seek(0);
				appendEndOfFile(reader2,resultFileContents);*/
				if(getMergeConfirmation(oldFile.getAbsolutePath())) {
					moveFile(oldFile, oldFile.getAbsolutePath() + "_bak");
				}
				moveFile(newFile, oldFile.getAbsolutePath());
				reader1.close();
				reader2.close();
				return;
			} else {
				isValid = appendEndOfFile(reader2,resultFileContents);
				if(!isValid) {
					/*resultFileContents = new StringBuffer("");
					reader2.seek(0);
					appendEndOfFile(reader2,resultFileContents);*/
					if(getMergeConfirmation(oldFile.getAbsolutePath())) {
						moveFile(oldFile, oldFile.getAbsolutePath() + "_bak");
					}
					moveFile(newFile, oldFile.getAbsolutePath());
					reader1.close();
					reader2.close();
					return;
				} 
			}
			reader1.close();
			reader2.close();
			
			resultFile.delete();
			resultFile = new File(resultFile.getAbsolutePath());
			resultFile.createNewFile();
			
			RandomAccessFile writer = new RandomAccessFile(resultFile, "rw");
			writer.writeBytes(resultFileContents.toString());
			writer.close();
			/**------------------------------------**/
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Performs code merge on the given two files.
	 * 
	 * @param oldFile
	 * @param newFile
	 * @param resultFile
	 */
	public static void performCodeMerge(File oldFile, File newFile,
			File resultFile) {
		List<Long> tagOffsetList = new ArrayList<Long>();
		Map<String, Long> tagOffsetMap = new HashMap<String, Long>();
		String readLine = null;

		try {
			RandomAccessFile oldFileReader = new RandomAccessFile(oldFile, "r");
			while ((readLine = oldFileReader.readLine()) != null) {

				if (readLine.contains(MergeConstants.BEGIN_TAG)) {
					if (readLine.contains("$")) {
						tagOffsetMap.put(readLine.substring(readLine
								.indexOf("$") + 1, readLine.lastIndexOf("$")),
								oldFileReader.getFilePointer());

					} else {
						tagOffsetList.add(oldFileReader.getFilePointer());
					}
				}
			}

			RandomAccessFile newFileReader = new RandomAccessFile(newFile, "r");
			StringBuffer mergedContent = new StringBuffer("");
			int counter = 0;
			String tag;
			long offset;

			while ((readLine = newFileReader.readLine()) != null) {

				mergedContent.append(readLine + "\n");
				if (readLine.contains(MergeConstants.BEGIN_TAG)) {

					if (readLine.contains("$")) {
						tag = readLine.substring(readLine.indexOf("$") + 1,
								readLine.lastIndexOf("$"));
						if (tagOffsetMap.get(tag) != null) {
							offset = tagOffsetMap.get(tag);
						} else {
							continue;
						}

					} else {
						try {
							offset = tagOffsetList.get(counter++);
						} catch (ArrayIndexOutOfBoundsException e) {
							counter++;
							break;
						}
					}

					oldFileReader.seek(offset);
					mergedContent
							.append(getApplicationBlockCode(oldFileReader));
					skipApplicationBlockCode(newFileReader);
				}
			}

			oldFileReader.close();
			newFileReader.close();

			if (tagOffsetList.size() != counter) {
				backupFile(oldFile);
				moveFile(newFile, oldFile.getAbsolutePath());
				return;
			}

			resultFile.delete();
			resultFile = new File(resultFile.getAbsolutePath());
			resultFile.createNewFile();

			RandomAccessFile resultFileWriter = new RandomAccessFile(
					resultFile, "rw");
			resultFileWriter.writeBytes(mergedContent.toString());
			resultFileWriter.close();

		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Returns the code from the current Application Block.
	 * 
	 * @param reader
	 * @return
	 * @throws IOException
	 */
	public static StringBuffer getApplicationBlockCode(RandomAccessFile reader)
			throws IOException {
		StringBuffer applicationCode = new StringBuffer("");
		String readLine = null;

		while ((readLine = reader.readLine()) != null) {
			if (readLine.contains(MergeConstants.END_TAG)) {
				return applicationCode;
			}
			applicationCode.append(readLine + "\n");
		}

		return applicationCode;
	}

	/**
	 * Skips code from the current Application Block.
	 * 
	 * @param reader
	 * @throws IOException
	 */
	public static void skipApplicationBlockCode(RandomAccessFile reader)
			throws IOException {
		long offset = reader.getFilePointer();
		String readLine = null;

		while ((readLine = reader.readLine()) != null) {
			if (readLine.contains(MergeConstants.END_TAG)) {
				reader.seek(offset);
				return;
			}
			offset = reader.getFilePointer();
		}
	}

	/**
	 * Backs up the given file based on user's input.
	 * 
	 * @param file
	 * @throws IOException
	 */
	public static void backupFile(File file) throws IOException {
		if (MessageDialog
				.openQuestion(
						Display.getDefault().getActiveShell(),
						"Conflicts in " + file.getAbsolutePath()
								+ " file templates",
						"Mismatch in templates which are used for generated code and old code. So new file will overide old file. Do you need backup?")) {

			moveFile(file, file.getAbsolutePath() + "_bak");
		}
	}

	/**
	 * Reads the code with in Clovis Tag
	 * @param reader Reader
	 * @return code within the tag
	 * @throws IOException
	 */
	public static StringBuffer readCodeWithInTheBlock(RandomAccessFile reader) throws IOException {
		StringBuffer userCode = new StringBuffer("");
		String readLine = null;
		while((readLine = reader.readLine()) != null) {
			if(readLine.contains(MergeConstants.END_TAG)) 
				return userCode;
			userCode.append(readLine+"\n");
		}
		return userCode;
	}
	/**
	 * Skip the code within the clovis tag
	 * @param reader Reader
	 * @throws IOException
	 */
	public static void skipCodeWithInTheBlock(RandomAccessFile reader) throws IOException {
		String readLine = null;
		long offset = reader.getFilePointer();
		while((readLine = reader.readLine()) != null) {
			if(readLine.contains(MergeConstants.END_TAG)) {
				reader.seek(offset);
				return;
			}
			offset = reader.getFilePointer();	
		}
	}
	/**
	 * Relaces new code(newly generated code) with user written code
	 * @param reader Reader
	 * @param userCode User Code
	 * @param resultFileContents Result File Contents
	 * @throws IOException
	 */
	public static boolean replacegeneratedCodeWithUserCode(RandomAccessFile reader,
			StringBuffer userCode, StringBuffer resultFileContents) throws IOException {
		String readLine = null;
		while ((readLine = reader.readLine()) != null) {
			resultFileContents.append(readLine + "\n");
			if (readLine.contains(MergeConstants.BEGIN_TAG)) {
				skipCodeWithInTheBlock(reader);
				resultFileContents.append(userCode);
				resultFileContents.append(reader.readLine() + "\n");
				return true;
			}
		}
		return false;
	}
	/**
	 * Update the remaining part of generated file
	 * @param reader Reader
	 * @param resultFileContents
	 * @throws IOException
	 */
	public static boolean appendEndOfFile(RandomAccessFile reader, StringBuffer resultFileContents) throws IOException {
		String readLine = null;
		boolean isValidFile = true;
		while ((readLine = reader.readLine()) != null) {
			if(readLine.contains(MergeConstants.BEGIN_TAG)) {
				isValidFile = false;
			}
			resultFileContents.append(readLine + "\n");
		}
		return isValidFile;
	}
	/**
	 * Add newly created Files
	 * @param resources Files list which needs to be added
	 * @param sourceFolder Source Folder
	 * @param targetFolder Target Folder
	 */
	public static void addResources(List resources, IFolder sourceFolder,
			IFolder targetFolder) {
		for (int i = 0; i < resources.size(); i++) {
			String path = (String) resources.get(i);
			IResource resource = sourceFolder.findMember(path);
			if(resource.getType() == IResource.FOLDER) {
				IFolder folder = targetFolder.getFolder(path);
				try {
					folder.create(true, true, null);
				} catch (CoreException e) {
					e.printStackTrace();
				}
			} else if(resource.getType() == IResource.FILE) {
				IFile file = targetFolder.getFile(path);
				try {
					file.create(new FileInputStream(resource.getLocation().toOSString()), true, null);
				} catch (FileNotFoundException e) {
					e.printStackTrace();
				} catch (CoreException e) {
					e.printStackTrace();
				}
			}
		}
	}
	/**
	 * Remove Files which are removed in model
	 * @param resources List of files which needs to be removed
	 * @param targetFolder Target Folder
	 */
	public static void removeResources(List resources, IFolder targetFolder, IFolder last) {
		for (int i = 0; i < resources.size(); i++) {
			String path = (String) resources.get(i);
			IResource targetResource = targetFolder.findMember(path);
			IResource lastResource = last.findMember(path);
			if(targetResource != null && targetResource.exists()) {
				try {
					if (lastResource != null) {
						if (targetResource instanceof IFolder) {
							List<String> members = new ArrayList<String>();
							IResource memArray[] = ((IFolder) targetResource)
									.members();
							for (int j = 0; j < memArray.length; j++) {
								members.add(memArray[j].getName());
							}
							removeResources(members,
									(IFolder) targetResource,
									(IFolder) lastResource);
							IResource res[] = ((IFolder)targetResource).members();
							if(((IFolder)targetResource).members().length == 0) {
								targetResource.delete(true, null);
							}
						} else {
							targetResource.delete(true, null);
						}
					} else {
						
					}
				} catch (CoreException e) {
					e.printStackTrace();
				}
			}
		}
	}
	/**
	 * Override files from source to target 
	 * @param modifiedFiles files which are already updated
	 * @param sourceFolder source folder
	 * @param targetFolder target folder.
	 */
	public static void overrideFiles(String path, List modifiedFiles,
			IFolder sourceFolder, IFolder targetFolder) {
		try {
			IResource resources[] = sourceFolder.members();
			for (int i = 0; i < resources.length; i++) {
				IResource resource = resources[i];
				String resName = resource.getName();
				if (resource instanceof IFolder) {
					IFile file = targetFolder.getFile(resName);
					if (file.exists())
					{
						file.delete(true, false, null);
					}
					
					IFolder target = targetFolder.getFolder(resName);
					if (!target.exists()) {
						target.create(true, true, null);
					}
					overrideFiles(path + resName + File.separator,
							modifiedFiles, (IFolder) resource, target);
				} else if (resource instanceof IFile) {
					if(!modifiedFiles.contains(path + resName)) {
						IFile file = targetFolder.getFile(resName);
						if(file.exists()) {
							file.delete(true, false, null);
						}
						file.create(new FileInputStream(resource.getLocation().toOSString()), true, null);
					}
				}
			}
		} catch (CoreException e) {
			e.printStackTrace();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
	}
	/**
	 * Overrides files from source to target
	 * @param files files needs to be override
	 * @param sourceFolder source location
	 * @param targetFolder target location
	 */
	public static void overrideFiles(String files[], IFolder sourceFolder,
			IFolder targetFolder) {
		for (int i = 0; i < files.length; i++) {
			IFile sourceFile = sourceFolder.getFile(files[i]);
			IFile targetFile = targetFolder.getFile(files[i]);
			try {
				if (targetFile.exists()) {
					targetFile.delete(true, false, null);
				} else if(!targetFile.getParent().exists()) {
					((IFolder)targetFile.getParent()).create(true, true, null);
				}
				targetFile.create(new FileInputStream(sourceFile.getLocation()
						.toOSString()), true, null);
				} catch (CoreException e) {
				e.printStackTrace();
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			}
		}
	}
	/**
	 * Adds name changed files to changed list
	 * @param changedList list contains user modified files
	 * @param addedList list contains new files
	 * @param removedList list contains old files
	 * @param namesMap map for old with new file name
	 * @param srcFolder
	 * @param nextGenFolder
	 * @param lastGenFolder
	 */
	public static void addNameChangedFiles(List changedList, List addedList,
			List removedList, Map namesMap, IFolder srcFolder,
			IFolder nextGenFolder, IFolder lastGenFolder) {
		List tmp = new ArrayList();
		for (int i = 0; i < addedList.size(); i++) {
			String fileName = (String) addedList.get(i);
			if (namesMap.containsKey(fileName)) {
				String oldName = (String) namesMap.get(fileName);
				if (!oldName.equals(fileName)) {
					try {
						String oldFile = srcFolder.getFile(oldName)
						.getLocation().toOSString();
						String newFile = nextGenFolder.getFile(fileName)
						.getLocation().toOSString();
						boolean status = isFileMatched(oldFile, newFile);
						if(!status) {
							IFile lastFile = lastGenFolder.getFile(oldName);
							if(!lastFile.exists() || !isFileMatched(oldFile, lastFile.getLocation().toOSString())) {
								tmp.add(fileName);
								changedList.add(fileName);
								//removedList.remove(oldName);
							} else {}
						} else {}
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			} else {
				// System.out.println(fileName + " ERROR IN MERGE");
			}
		}
		addedList.removeAll(tmp);
	}
	/**
	 * Filter user modified files from changed file list
	 * @param changesList changed file list
	 * @param namesMap map for old and new files
	 * @param srcFolder source location
	 * @param last code generation location
	 * @return List which contains user modified files
	 */
	public static List filterUserModifiedFiles(List changesList, Map namesMap,
			IFolder srcFolder, IFolder lastGenFolder) {
		List list = new ArrayList();
		for (int i = 0; i < changesList.size(); i++) {
			String fileName = (String) changesList.get(i);
			if (namesMap.containsKey(fileName)) {
				try {
					IFile srcFile = srcFolder.getFile(fileName);
					IFile lastFile = lastGenFolder.getFile(fileName);
					if (!lastFile.exists() || !isFileMatched(srcFile.getLocation().toOSString(), lastFile.getLocation().toOSString()))
						list.add(fileName);
				} catch (IOException e) {
					e.printStackTrace();
				}
			} 
		}
		return list;
	}
	/**
	 * List all the files list which are modified by user.
	 * @param changesList
	 * @param srcFolder
	 * @return
	 */
	public static List filterUserModifiedFiles(List <String> changesList, IFolder srcFolder) {
		List <String>list = new ArrayList <String>();
		for (int i = 0; i < changesList.size(); i++) {
			String fileName =  changesList.get(i);
			if(fileName.endsWith(".c")) {
				list.add(fileName);
			}
		}
		return list;
	}
	/**
	 * Check the files match
	 * @param oldFile Old File
	 * @param newFile New File
	 * @return match status
	 * @throws IOException
	 */
	public static boolean isFileMatched(String oldFile, String newFile) throws IOException {
		boolean matched = true;
		Runtime runtime = Runtime.getRuntime();
		Process proc = runtime.exec("diff " + oldFile + " "
				+ newFile);
		InputStream input = proc.getInputStream();
		int read;
		try {
			while((read = input.read()) != -1) {
				char c = (char) read;
				if(c == '>' || c == '<') {
					matched = false;
					break;
				} 
			}
			proc.waitFor();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		input.close();
		return matched;
	}
	/**
	 * Move files from source to target locations
	 * @param sourceFolder source folder
	 * @param targetFolder target folder
	 */
	public static void moveFiles(IFolder sourceFolder, IFolder targetFolder) {
		try {
			IResource resources[] = sourceFolder.members();
			for (int i = 0; i < resources.length; i++) {
				IResource resource = resources[i];
				if(resource instanceof IFile) {
					IFile file = targetFolder.getFile(resource.getName());
					file.create(new FileInputStream(resource.getLocation()
							.toOSString()), true, null);
				} else if(resource instanceof IFolder) {
					IFolder folder = targetFolder.getFolder(resource.getName());
					folder.create(true, true, null);
					moveFiles((IFolder) resource, folder);
				}
			}
			sourceFolder.delete(true, null);
		} catch (CoreException e) {
			e.printStackTrace();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
	}
	
	static class ErrorStreamReader {
		InputStream error;
		public ErrorStreamReader(InputStream error) {
			this.error = error;
			read();
		}
		private void read() {
			try {
				int c;
				while((c = error.read()) != -1) {
					System.err.print((char)c);
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		
	}
	/**
	 * Returns user's merge confirmation status
	 * @return boolean
	 */
	public static boolean getMergeConfirmation(String file) {
		return MessageDialog
		.openQuestion(
				Display.getDefault().getActiveShell(),
				"Conflicts in " + file + " file templates",
				"Mismatch in templates which are used for generated code and old code. So new file will overide old file. Do you need backup?");
	}
	/**
	 * Move srcFile to destLocation
	 * @param file source file
	 * @throws IOException 
	 */
	public static void moveFile(File srcFile, String destLocation) throws IOException {
		File targetFile = new File(destLocation);
		targetFile.deleteOnExit();
		targetFile.createNewFile();
		srcFile.renameTo(targetFile);
	}

	/**
	 * Filters files which should not be merged.
	 * 
	 * @param project
	 * @param changedList
	 * @return
	 */
	public static ArrayList<String> filterMergeImmuneFiles(IProject project, List changedList)
	{
		File mergeImmuneListFile = new File(project.getLocation().append(".mergeImmuneFile").toOSString());
		ArrayList<String> mergeImmuneList = new ArrayList<String>();

		if(mergeImmuneListFile.exists()) {
			try {
				Iterator<String> changedIterator;
				String file, path;

				BufferedReader reader = new BufferedReader(new FileReader(mergeImmuneListFile));
				while ((file = reader.readLine()) != null) {
					changedIterator = changedList.iterator();

					while(changedIterator.hasNext()) {
						path = changedIterator.next();
						if(path.endsWith(file)) {
							changedIterator.remove();
							mergeImmuneList.add(path);
						}
					}
				}

			} catch (FileNotFoundException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

		return mergeImmuneList;
	}
}
