/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/Python.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils;

import java.io.InputStream;
/**
 * @author nadeem
 *
 * This class helps running python scripts from JVM.
 */
public class Python
{
    private static final String PYTHON_CMD = "python";
    /**
     * Wait for process to exit.
     * @param process Process
     * @return exit status of the process
     * @throws Exception On process issues.
     */
    private static int waitToExit(Process process)
        throws Exception
    {
        int exitStatus   = 0;
        byte[] byteArray = new byte[1024];
        InputStream errStream = process.getErrorStream();
        InputStream outStream = process.getInputStream();
        while (true) {
            if (errStream.available() != 0) {
                errStream.read(byteArray);
                System.err.println(new String(byteArray));
            }
            if (outStream.available() != 0) {
                outStream.read(byteArray);
                System.out.println(new String(byteArray));
            }
            try {
                exitStatus = process.exitValue();
                 break;
            } catch (IllegalThreadStateException  e) {
                //Process Still Running. Wait for some
                //time and process the stream.
                try {
                    Thread.sleep(100);
                } catch (InterruptedException iexeption) {
                      iexeption.printStackTrace();
                }
            }
        }
        return exitStatus;
    }
    /**
     * Test program.
     * @param args script argument
     * @throws Exception On process issues.
     */
    public static void main(String[] args)
        throws Exception
    {
        int status = run(args);
        System.out.println("Script Exit Value: " + status);
    }
    /**
     * Run python script with arguments.
     * @param args script argument
     * @return exit value of script.
     * @throws Exception On process issues.
     */
    public static int run(String[] args)
        throws Exception
    {
        String[] cmdArgsArray     = new String[args.length + 1];
        System.arraycopy(args, 0, cmdArgsArray, 1, args.length);
        cmdArgsArray[0] = PYTHON_CMD;
        return waitToExit(Runtime.getRuntime().exec(cmdArgsArray));
    }
}
