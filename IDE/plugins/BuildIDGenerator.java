/*******************************************************************************
 * ModuleName  : BuildIDGenerator
 * $File: //depot/dev/main/Andromeda/IDE/plugins/BuildIDGenerator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Properties;

/**
 * This class is responsible for creating the build-id and project version for
 * the release.
 * 
 * @author Suraj Rajyaguru
 */
public class BuildIDGenerator {

	/**
	 * Main Method for the class.
	 */
	public static void main(String args[]) {

		String versionFilePath = args[0] + File.separator
				+ "com.clovis.cw.data" + File.separator + "version.properties";
		String mappingsFilePath = args[0] + File.separator
				+ "com.clovis.cw.data" + File.separator + "about.mappings";

		Properties versionProperties = loadVersionDetails(versionFilePath);
		createMappingsFile(mappingsFilePath, versionProperties);
	}

	/**
	 * Loads the version details for the release and returns it.
	 */
	private static Properties loadVersionDetails(String filePath) {
		Properties versionProperties = new Properties();

		try {
			versionProperties.load(new FileInputStream(new File(filePath)));
		} catch (Exception e) {
			System.out.println("Error Loading Product version details.\n"
					+ e.getMessage());
		}

		return versionProperties;
	}

	/**
	 * Creates the about.mappings file.
	 */
	private static void createMappingsFile(String filePath,
			Properties versionProperties) {

		try {
			File file = new File(filePath);
			if (file.exists()) {
				file.delete();
			}
			file.createNewFile();

			String comments = "# about.mappings \n"
					+ "# contains fill-ins for about.properties \n"
					+ "# java.io.Properties file (ISO 8859-1 with '\\' escapes) \n"
					+ "# This file does not need to be translated. \n";

			Properties mappingsProperties = new Properties();
			mappingsProperties.put("0", generateBuildID());
			mappingsProperties.put("1", versionProperties
					.getProperty("release.version"));
			mappingsProperties.put("2", versionProperties
					.getProperty("update.version"));

			mappingsProperties.store(new FileOutputStream(file), comments);

		} catch (Exception e) {
			System.out.println("Error creating about.mappings file.\n"
					+ e.getMessage());
		}
	}

	/**
	 * Generates the build-id for the release.
	 */
	private static String generateBuildID() {
		SimpleDateFormat sdf = new SimpleDateFormat("ddMMyy-HHmm");
		return "C" + sdf.format(Calendar.getInstance().getTime());
	}
}
