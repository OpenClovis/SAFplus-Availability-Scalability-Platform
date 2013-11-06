/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
#ifndef _CL_TEST_H_
#define _CL_TEST_H_

#include <stdio.h>
#include "clDebugApi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file
 * \brief OpenClovis Test Infrastructure
 * \ingroup test_apis
 *
 */

/**
 * \addtogroup test_apis
 * \{
 */

/* See clTest.c for documentation on these global variables */
extern int clTestLogLevel;
extern int clTestOn;
extern int clTestPrintIndent;

enum 
{
  ClTestCaseMaxNameLen = 512,
  ClTestMaxNameLen     = 512
};

typedef enum /* A bit field that turns on/off types of printout */
{
    CL_TEST_PRINT_ALL = 0xffffff,
    CL_TEST_PRINT_TEST_OK = 1,
    CL_TEST_PRINT_TEST_FAILED = 2,
} ClTestVerbosity;

extern ClTestVerbosity clTestVerbosity;

/* Not part of the public API */

typedef struct 
{
  unsigned int passed;
  unsigned int failed;
  unsigned int malfunction;
  unsigned int malfPops;
  char         name[ClTestCaseMaxNameLen];
} ClTestCaseData;

/* Not part of the public API */
extern void clPushTestCase(const ClTestCaseData* tcd);
extern void clPopTestCase(ClTestCaseData* tcd);
extern int  clTestGroupFinalizeImpl();
extern void clTestGroupInitializeImpl();
extern void clTestPrintImpl(const char* file, int line, const char* fn, const char* c);
extern void clTestImpl(const char* file, int line, const char* function, const char * id, const char * error, int ok);

extern ClTestCaseData clCurTc;


/* All strings are printf formatted and in () */
#define this_error_indicates_missing_parens_around_string(...) __VA_ARGS__

#define clTestStripParens(...) __VA_ARGS__


#ifndef CL_NO_TEST
/**
 ************************************
 *  \brief Start up the Test infrastructure
 *
 *
 *  \param name Identifies the test Service Group.  Must be formatted in printf
 *  style i.e. ("name").  If you have more then one program in your SG, then
 *  give them different names with the same prefix.
 *
 *  \return nothing
 *
 *  \par Description:
 *   This function call starts up the test infrastructure in your program, and
 *  tells the Test Automation Environment that this Service Group has started.
 *  You should only call it once during your program's initialization.
 *
 *  \par Example:
 *  clTestGroupInitialize(("My Unit Test"))
 *
 *  \sa clTestGroupFinalize()
 *
 */
#define clTestGroupInitialize(name) do { clTestPrint(name); clTestGroupInitializeImpl(); } while(0)

/**
 ************************************
 *  \brief Stop the Test infrastructure
 *
 *
 *  \return The number of test cases that failed.
 *
 *  \par Description:
 *   This function stops the test infrastructure.  It is critically important to call this function so that the
 *  automation environment knows to stop your Service Group.
 *
 *  \sa clTestGroupInitialize()
 *
 */
#define clTestGroupFinalize() clTestGroupFinalizeImpl()

/**
 ************************************
 *  \brief Indicate that the Test Group cannot execute, if an expression evaluates to False.
 *
 *  \param reason        A parenthesized printf-style string describing why the test group cannot be run.
 *  \param predicate     If this expression evaluates to true, it is a malfunction, otherwise it is not.
 *  \param execOnFailure Run this code if there is a malfunction.
 *
 *  \return 
 * 
 *  \retval 
 *  \retval 
 *
 *  \par Description:
 *    This function is used to indicate that a set of tests have neither succeeded or failed -- some condition required
 *  to execute the test was not met.  This function "Finalizes" the TestGroup, so after calling this function, you must
 *  NOT use any clTest APIs
 *
 *
 */
#define clTestGroupMalfunction(reason, predicate, execOnFailure) \
do { \
    if (!(predicate)) { \
        clTestPrint(reason); \
        clTestPrint(("Test Malfunction.")); \
        (void) clTestGroupFinalize(); \
        execOnFailure; \
    } \
} while(0)

/**
 ************************************
 *  \brief Run a test case
 *
 *
 *  \param name A parenthesized printf-style string describing the test case
 *  \param test The test case code.  Most likely a function call.
 *
 *  \par Description:
 *    A test case is a grouping of individual tests into a logical unit.  Test cases can be nested, this function can
 *  be called inside the 'test'.  This function is syntatic sugar for: "clTestCaseStart(name); test; clTestCaseEnd();"
 *  It is included because it is good practice to isolate your test cases into their own functions.  This macro makes
 *  it easy to run a test case that is implemented as a function.
 *
 *  \sa clTestCaseStart(), clTestCaseEnd()
 *
 */
#define clTestCase(name, test) \
do { \
    clTestCaseStart(name); \
    clCurTc.malfPops = 0; \
    test; \
    clTestCaseEnd((" ")); \
} while (0)


/**
 ************************************
 *  \brief Start a Test Case
 *
 *
 *  \param testname A parenthesized printf-style string describing the test case
 *
 *  \par Description:
 *    A test case is a grouping of individual tests into a logical unit.  Test cases can be nested by calling this 
 *  function multiple times with no intervening call to ClTestCaseEnd()
 *
 *  \sa clTestCase(), clTestCaseEnd()
 *
 */
#define clTestCaseStart(testname) \
do { \
    clTestPrintIndent+=2; \
    clPushTestCase(&clCurTc); \
    clCurTc.passed=0; \
    clCurTc.failed=0; \
    clCurTc.malfunction=0; \
    clCurTc.malfPops = 1; \
    snprintf(clCurTc.name, ClTestCaseMaxNameLen, \
        this_error_indicates_missing_parens_around_string testname); \
    clTestPrint(("Test case started [%s]:\n", clCurTc.name)); \
} while(0)

/**
 ************************************
 *  \brief Returns the number of errored (failed) tests run so far in this test case.
 *
 *  \return The number of errored (failed) tests run so far in this test case.
 *
 *  \sa clTestCase(), clTestCaseStart(), clTestCaseNumErrors(), clTestCaseNumMalfunctions(), clTestCaseNumPasses()
 *
 */
#define clTestCaseNumErrors() clCurTc.failed

/**
 ************************************
 *  \brief Return the number of malfunctioned tests in this test case (so far).
 *
 *  \return The number of malfunctioned tests in this test case (so far).
 * 
 *  \sa clTestCase(), clTestCaseStart(), clTestCaseNumErrors(), clTestCaseNumMalfunctions(), clTestCaseNumPasses()
 *
 */
#define clTestCaseNumMalfunctions() clCurTc.malfunction

/**
 ************************************
 *  \brief Return the number of successful (passed) test run so far in this test case.
 *
 *  \return The number of successful (passed) test run so far in this test case.
 * 
 *  \sa clTestCase(), clTestCaseStart(), clTestCaseNumErrors(), clTestCaseNumMalfunctions(), clTestCaseNumPasses()
 *
 */
#define clTestCaseNumPasses() clCurTc.passed

/**
 ************************************
 *  \brief Stop this test case.
 *
 *  \param synopsis A parenthesized printf-style string that you would like printed and stored with the results of this
 *  test case run.  Do not print counts of the number of passed, failed, or malfunctioned tests; The function already
 *  does that.
 *
 *  \par Description:
 *   A test case is a grouping of individual tests into a logical unit.  This function ends this logical unit and prints
 *  the results.
 *
 *  \sa clTestCase(), clTestCaseStart()
 *
 */
#define clTestCaseEnd(synopsis) \
do { \
    clTestPrint(("Test case completed [%s]. " \
                 "Subcases: [%d] passed, [%d] failed, [%d] malfunction.\n" \
                 "Synopsis:\n", \
                 clCurTc.name, clCurTc.passed, \
                 clCurTc.failed, clCurTc.malfunction)); \
    clTestPrint(synopsis); \
    clTestPrintIndent-=2; \
    int malf=clCurTc.malfunction; \
    int fail = clCurTc.failed; \
    clPopTestCase(&clCurTc); \
    if (fail) \
        clCurTc.failed++; \
    else if (malf) \
        clCurTc.malfunction++; \
    else \
        clCurTc.passed++; \
} while(0)

/**
 ************************************
 *  \brief  Indicate that this test case cannot be completed, if the predicate fails.
 *
 *  \param reason        A parenthesized printf-style string describing why it cannot be completed.
 *  \param predicate     If this expression evaluates to true, it is a malfunction, otherwise it is not.
 *  \param execOnFailure Run this code if there is a malfunction.
 *
 *  \par Description:
 *    This function is used to indicate that a set of tests have neither succeeded or failed -- some condition required
 *  to execute the test was not met.  This function "Ends" the TestCase, so after calling this function you should not
 *  call clTestCaseEnd()
 *
 *  \sa clTestCase(), clTestCaseStart(), clTestCaseEnd()
 *
 */
// Will call TestCaseEnd, but you have to "return" or break, whatever
#define clTestCaseMalfunction(reason, predicate, execOnFailure) \
do { \
    if (!(predicate)) { \
        clTestPrint(("Test case malfunction [%s]. " \
                     "Subtests: [%d] Passed, [%d] Failed.\n" \
                     "Malfunction reason:\n", \
                     clCurTc.name, clCurTc.passed, clCurTc.failed)); \
        clTestPrint(reason); \
        if (clCurTc.malfPops) clPopTestCase(&clCurTc); \
        clCurTc.malfunction++; \
        execOnFailure; \
    }  \
} while(0)

/**
 ************************************
 *  \brief Run an individual test.
 *
 *  \param string       A parenthesized printf-style string that identifies this test
 *  \param predicate    An expression that evaluates to True if the test succeeded, False if it failed.
 *  \param errorPrintf  A parenthesized printf-style string that you would like printed if the predicate fails.
 *                      There is no need to print "failed!", etc since this function will do that for you.
 *
 *  \par Description:
 *    This call runs a test contained in the "predicate" parameter.
 *
 *  \par Examples:
 *    clTest(("Foo basic"), Foo(ptr)==CL_OK,(" "))
 *    clTest(("Foo with NULL arg"), ((rc = Foo(NULL))==CL_ERR_NULL_POINTER),("rc = %d", rc))
 *
 *  \sa clTestExecOnFailure()
 *
 */
#define clTest(string, predicate, errorPrintf)\
do{ \
    char __id[ClTestMaxNameLen]; \
    char __error[ClTestMaxNameLen]=""; \
    int __ok = predicate; \
    snprintf(__id, ClTestMaxNameLen, \
             this_error_indicates_missing_parens_around_string string); \
    if (!__ok) \
        snprintf(__error, ClTestMaxNameLen, \
            this_error_indicates_missing_parens_around_string errorPrintf); \
    clTestImpl(__FILE__, __LINE__, __FUNCTION__, __id, __error, __ok); \
} while(0)

// do { clTestPrint(string); { int result = (predicate); if (result) { clCurTc.passed++; clTestPrint((": Ok\n")); } else { clCurTc.failed++; clTestPrint((": %s:%d: Test (" #predicate ") failed.  ", __FILE__, __LINE__)); clTestPrint(errorPrintf); clTestPrint(("\n")); } } } while(0)

/**
 ************************************
 *  \brief Run an individual test, and run some special code if it fails
 *
 *  \param string        A parenthesized printf-style string that identifies this test
 *  \param predicate     An expression that evaluates to True if the test succeeded, False if it failed.
 *  \param errorPrintf   A parenthesized printf-style string that you would like printed if the predicate fails.
 *                       There is no need to print "failed!", etc since this function will do that for you.
 *  \param execOnFailure Run this code if the predicate is false.
 *
 *  \par Description:
 *    This function is equivalent to clTest, except that you can pass some code to run if there is a failure.
 *
 *  \par Examples:
 *    clTestExecOnFailure(("Foo basic"), Foo(ptr)==CL_OK,(" "),{ emailMe(); return; })
 *    clTest(("Foo with NULL arg"), ((rc = Foo(NULL))==CL_ERR_NULL_POINTER),("rc = %d", rc))
 *
 *  \sa clTest()
 *
 */
#define clTestExecOnFailure(string, predicate, errorPrintf, execOnFailure) \
do { \
    clTestPrint(string); \
    { \
        int result = (predicate); \
        if (result) { \
            clCurTc.passed++; \
            clTestPrint((": Ok\n")); \
        } else { \
            clCurTc.failed++; \
            clTestPrint((": %s:%d: Test (" #predicate ") failed.  ", \
                         __FILE__, __LINE__)); \
            clTestPrint(errorPrintf); \
            clTestPrint(("\n")); \
            execOnFailure; \
        } \
    } \
} while(0)

/**
 ************************************
 *  \brief Indicate that a test failed
 *
 *  \param __string        A parenthesized printf-style string that identifies this test
 *
 *  \par Description:
 *   Sometimes deciding whether a test passed or failed requires some complex logic.  Other times you have to write an
 *  if() statement anyway.  This call simply indicates that the test identified by the 'string' failed.  Do not put
 *  words like "failed", "didn't work" etc. in the string.  The library will do that for you.
 *
 *  \par Examples:
 *  rc = Foo()
 *  if (rc != CL_OK) clTestFailed(("Foo"));
 *
 *  \sa clTestSuccess()
 *
 */
#define clTestFailed(__string) clTestFailedAt(__FILE__, __LINE__,__string)

/**
 ************************************
 *  \brief Just like clTestFailed, but allows the caller to specify the file and line
 *  \par Description:
 *   clTestFailed is the preferred API.  Use this API when you want to wrap
 *   this in your own helper function.  In this way the file and line can
 *   reflect the file/line of your test, not that of the helper function.
 *  \sa clTestFailed()
 */
#define clTestFailedAt(__file, __line, __string) \
do { \
    clCurTc.failed++; \
    clTestPrint((": %s:%d: Test failed.  ", __file, __line)); \
    clTestPrint(__string); \
    clTestPrint(("\n")); \
} while(0)

/**
 ************************************
 *  \brief Indicate that a test passed
 *
 *  \param __string        A parenthesized printf-style string that identifies this test
 *
 *  \par Description:
 *   Sometimes deciding whether a test passed or failed requires some complex logic.  Other times you have to write an
 *  if() statement anyway.  This call simply indicates that the test identified by the 'string' succeeded.  Do not put
 *  words like "success", "worked", "ok" etc. in the string.  The library will do that for you.
 *
 *  \par Examples:
 *  rc = Foo()
 *  if (rc != CL_OK) clTestSuccess(("Foo"));
 *
 *  \sa clTestFailed()
 *
 */


#define clTestSuccess(__string) clTestSuccessAt(__FILE__, __LINE__, __string) 

/**
 ************************************
 *  \brief Just like clTestSuccess, but allows the caller to specify the file and line
 *  \par Description:
 *   clTestSuccess is the preferred API.  Use this API when you want to wrap
 *   this in your own helper function.  In this way the file and line can
 *   reflect the file/line of your test, not that of the helper function.
 *  \sa clTestSuccess()
 */
#define clTestSuccessAt(__file, __line, __string) \
{ \
    clCurTc.passed++; \
    clTestPrintAt(__file,__line, __FUNCTION__, __string); \
    clTestPrintAt(__file,__line, __FUNCTION__, (": Ok\n")); \
} while (0)



/**
 ************************************
 *  \brief Print something to the test console
 *
 *  \param x A parenthesized printf-style string that should be printed
 *
 *  \par Description:
 *   This function prints to the test console.  Do not use for general purpose logging, but only for items that are
 *  specific to testing.  Also, it is much better to use the strings within the other API calls because they are
 *  associated with a particular test or test case.  Frankly, I can't think of any reason to use this function, but
 *  it is here to provide for the unanticipated.
 *
 */
#define clTestPrint(x) clTestPrintAt(__FILE__, __LINE__, __FUNCTION__, x)

/**
 ************************************
 *  \brief Just like clTestPrint, but allows the caller to specify the file and line
 *  \par Description:
 *   clTestPrint is the preferred API.  Use this API when you want to wrap
 *   this in your own helper function.  In this way the file and line can
 *   reflect the file/line of your test, not that of the helper function.
 *  \sa clTestPrint()
 */
#define clTestPrintAt(__file, __line, __function, x) \
do { \
    char __tempstr[2048]; \
    snprintf(__tempstr,2048, \
             this_error_indicates_missing_parens_around_string x); \
    clTestPrintImpl(__file, __line, __function,__tempstr); \
    clLog(clTestLogLevel,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,__tempstr); \
} while(0)

#else

/* Make all macros a noop if CL_NO_TEST is defined */
#define clTestGroupInitialize(name) do{ } while(0)
#define clTestGroupFinalize() do{ } while(0)
#define clTestGroupMalfunction(reason, predicate, execOnFailure) do{ } while(0)
#define clTestCase(name, test) do{ } while(0)
#define clTestCaseStart(testname)  do{ } while(0)
#define clTestCaseNumErrors() 0
#define clTestCaseNumMalfunctions() 0
#define clTestCaseNumPasses() 0
#define clTestCaseEnd(synopsis) do{ } while(0)
#define clTestCaseMalfunction(reason, predicate, execOnFailure) do{ } while(0)
#define clTest(string, predicate, errorPrintf) do{ } while(0)
#define clTestExecOnFailure(string, predicate, errorPrintf, execOnFailure) do{ } while(0)
#define clTestFailed(string) do{ } while(0)
#define clTestSuccess(string) do{ } while(0)
#define clTestPrint(x) do{ } while(0)

#define clTestPrintAt(__file, __line, __function, x) do{ } while(0)
#define clTestSuccessAt(__file, __line, __string) do{ } while(0)
#define clTestFailedAt(__file, __line, __string) do{ } while(0)
#endif

#ifdef __cplusplus
}
#endif

/**
 * \}
 */
#endif
