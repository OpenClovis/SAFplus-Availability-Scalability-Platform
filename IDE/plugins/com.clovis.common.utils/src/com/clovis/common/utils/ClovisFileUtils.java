/*******************************************************************************
 * ModuleName  : Fileutils.com
 * $File: $
 * $Author: matt $
 * $Date: 2007/12/07 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * @author matt
 *
 * A place holder for general file utility functions.
 */
public class ClovisFileUtils
{
    
    /**
     * Copies a directory from one location to another. The method recurses
     *  through the directories files and subdirectories moving them as well.
     *  Passing a value of true for the deleteOnCopy parameter will delete
     *  each file and directory after copying. This is equivalent to a move
     *  operation. 
     * @param sourceDir - The directory to copy from.
     * @param destDir - The directory to copy to.
     * @param deleteOnCopy - Whether or not to delete teh directory after
     *                       copying.
     * @throws IOException
     */
	public static void copyDirectory(File sourceDir, File destDir, boolean deleteOnCopy) throws IOException
    {
    	// make the desination directory if it doesn't exist
		if (!destDir.exists()) destDir.mkdir();

		// recurse through all children of the directory
		File[] children = sourceDir.listFiles();
    	for (File sourceChild : children)
    	{
    		String name = sourceChild.getName();

    		File destChild = new File(destDir, name);

    		if (sourceChild.isDirectory())
    		{
    			copyDirectory(sourceChild, destChild, deleteOnCopy);
    		} else {
    			copyFile(sourceChild, destChild, deleteOnCopy);
    		}

    	}
        
    	// delete the directory if specified
    	if (deleteOnCopy) sourceDir.delete();
    }

   	/**
   	 * Copies a file from one location to another. If true is passed for the
   	 *  deleteOnCopy parameter then the source file is deleted after the copy
   	 *  is complete. This is equivalent to moving the file.
   	 * @param src
   	 * @param dst
   	 * @param deleteOnCopy
   	 * @throws IOException
   	 */
	public static void copyFile(File src, File dst, boolean deleteOnCopy) throws IOException
    {
   		// create the desintation file if it doesn't exist
		if(!dst.exists()) dst.createNewFile();

   		// open input and output streams
		InputStream in = new FileInputStream(src);
		OutputStream out = new FileOutputStream(dst);
    
        // transfer bytes from in to out
        byte[] buf = new byte[1024];
        int len;
        while ((len = in.read(buf)) > 0) {
            out.write(buf, 0, len);
        }
        in.close();
        out.close();
        
        // delete the file if specified
        if (deleteOnCopy) src.delete();
    }

	/**
	 * Delete a directory and all of its files and subdirectories
	 * @param directory
	 */
	public static void deleteDirectory(File directory)
	{
		// recurse through all children of the directory
		File[] children = directory.listFiles();
    	for (File child : children)
    	{
    		if (child.isDirectory())
    		{
    			deleteDirectory(child);
    		} else {
    			child.delete();
    		}
    	}

    	// now delete the directory itself
    	directory.delete();
	}
    
    /**
     * Creates a relative symbolic link to a file or directory from another path. Both
     *  of the paths passed in must be absolute (begin with /). The sourceLocation
     *  must exist and the linkFile must not.
     * @param sourceLocation
     * @param linkFile
     * @return true if the link was created successfully, false otherwise
     */
    public static boolean createRelativeSourceLink(String sourceLocation, String linkFile)
    {
    	boolean retVal = true;
    	
    	//make sure both are absolute paths
    	if (!sourceLocation.startsWith("/") || !linkFile.startsWith("/"))
    	{
    		return false;
    	}
    	
    	//trim any trailing slash
    	if (sourceLocation.endsWith("/")) sourceLocation = sourceLocation.substring(0, sourceLocation.length()-1);
    	if (linkFile.endsWith("/")) linkFile = linkFile.substring(0, linkFile.length()-1);
    	
    	// make sure the source and link are not the same
    	if (sourceLocation.equals(linkFile))
   		{
    		return false;
   		}
    	
    	// make sure that the source location exists and the link file doesn't
    	File source = new File(sourceLocation);
    	File link = new File(linkFile);
    	if (!source.exists() || link.exists())
   		{
    		return false;
   		}
    	
    	//break the paths into segments
    	String[] sourcePath = sourceLocation.split("/");
    	String[] linkPath = linkFile.split("/");
    	
    	// compare two sets of path segments and stop when they differ
    	int pathLength = sourcePath.length;
    	if (linkPath.length < pathLength) pathLength = linkPath.length;
    	int i=0;
    	while (sourcePath[i].equals(linkPath[i]))
    	{
    		i++;
    	}

    	// build the link command based on the path segments
    	StringBuffer linkCommand = new StringBuffer("ln -s ");
    	for (int j=i+1; j<linkPath.length; j++)
    	{
    		linkCommand.append("../");
    	}
    	
    	for (int j=i; j<sourcePath.length; j++)
    	{
    		if (j!=i) linkCommand.append("/");
    		linkCommand.append(sourcePath[j]);
    	}
    	
    	linkCommand.append(" ").append(linkFile);
    	
		// now execute the link command
    	try
		{
	    	Process proc = Runtime.getRuntime().exec(linkCommand.toString());
			int procReturn = proc.waitFor();
			if (procReturn != 0) retVal = false;
		} catch (IOException ioe) {
			return false;
		} catch (InterruptedException ie) {
			return false;
		}

    	return retVal;
    }

	/**
	 * Creates archive for the given source in the target location.
	 * 
	 * @param archievePath
	 * @param targetPath
	 * @return
	 */
	public static String createArchive(String archivePath, String targetPath) {

		File archiveSource = new File(archivePath);
		if (!archiveSource.exists()) {
			return null;
		}

		String command = "tar -zcf " + targetPath + " "
				+ archiveSource.getName();
		try {
			Runtime.getRuntime().exec(command, null,
					archiveSource.getParentFile());
			return new File(targetPath).getName();

		} catch (IOException e) {
			return null;
		}
	}

	/**
	 * Extracts the given archive to the given extract location.
	 * 
	 * @param archivePath
	 * @param extractPath
	 * @return
	 */
	public static String extractArchive(String archivePath, String extractPath) {

		File archiveSource = new File(archivePath);
		if (!archiveSource.exists()) {
			return null;
		}

		String command = "tar -zxf " + archiveSource.getName() + " -C "
				+ extractPath;
		try {
			Runtime.getRuntime().exec(command, null,
					archiveSource.getParentFile());
			return archiveSource.getName();

		} catch (IOException e) {
			return null;
		}
	}
}


