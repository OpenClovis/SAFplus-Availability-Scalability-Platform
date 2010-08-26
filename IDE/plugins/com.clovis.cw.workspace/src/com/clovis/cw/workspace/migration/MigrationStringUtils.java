/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * This is a utility class for various string based migration operations.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationStringUtils {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Replaces all the regex[] with correspondig replacement[] in all the
	 * file[].
	 * 
	 * @param file
	 * @param regex
	 * @param replacement
	 */
	public static void replaceAll(String file[], String regex[],
			String replacement[]) {

		for (int i = 0; i < file.length; i++) {
			replaceAll(file[i], regex, replacement);
		}
	}

	/**
	 * Replaces all the regex[] with correspondig replacement[] in the given
	 * file.
	 * 
	 * @param fileName
	 * @param regex
	 * @param replacement
	 */
	public static void replaceAll(String fileName, String regex[],
			String replacement[]) {

		String tempFile = fileName.substring(0, fileName
				.lastIndexOf(File.separator))
				+ File.separator + "temp.tmp";

		try {
			BufferedReader br = new BufferedReader(new FileReader(fileName));
			BufferedWriter bw = new BufferedWriter(new FileWriter(tempFile));

			String str = null;
			while ((str = br.readLine()) != null) {
				for (int i = 0; i < regex.length; i++) {
					str = str.replaceAll(regex[i], replacement[i]);
				}
				bw.write(str);
				bw.newLine();
			}

			bw.close();
			new File(tempFile).renameTo(new File(fileName));

		} catch (FileNotFoundException e) {
			LOG.warn("Migration : File " + fileName + " not found.", e);

		} catch (IOException e) {
			LOG.error("Migration : Error migrating " + fileName + ".", e);
		}
	}

	/**
	 * Checks whether the file is having the given string.
	 * 
	 * @param fileName
	 * @param checkStr
	 * @return
	 */
	public static boolean fileContainsString(String fileName, String checkStr) {
		try {
			BufferedReader br = new BufferedReader(new FileReader(fileName));
			String str = null;

			while ((str = br.readLine()) != null) {
				if (str.contains(checkStr)) {
					return true;
				}
			}

			return false;

		} catch (FileNotFoundException e) {
			LOG.warn("Migration : File " + fileName + " not found.", e);

		} catch (IOException e) {
			LOG.error("Migration : Error migrating " + fileName + ".", e);
		}
		return false;
	}
}
