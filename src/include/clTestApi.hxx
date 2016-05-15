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
#ifndef clTestApi
#define clTestApi

#include <stdio.h>
#include <clLogApi.hxx>
namespace SAFplus
{

//? <section name="Testing">

/*? <html>
<p>
SAFplus provides test macros that are integrated with the SAFplus Test Automation Environment (TAE) product.
However, since these macros produce an easily-parsed output file, they can also be used with other test frameworks.
</p><p>
Two flavors of testing are possible -- unit testing and application testing.  
Unit testing is very important but is a familiar concept so will not be discussed further.
Application testing is a bit different; in this technique, the programmer 
instruments the real application with tests, and then can conditionally
compile the tests "in" or "out".  
</p><p>
For example, your test harness might run your application against a set of carefully chosen
input data.  By controlling the input data, specific application behavior can be tested.

For example, the application could check for:
<ul>
 <li>Memory leaks</li>
 <li>Successful execution of functions</li>
 <li>Proper handling of invalid data</li>
</ul>
</p>
</html> */


//? Issue test related logs at this level (default is INFO) 
extern SAFplus::LogSeverity testLogLevel;
  
//extern int testOn;
extern int testPrintIndent;

enum 
{
  TestCaseMaxNameLen = 512,
  TestMaxNameLen     = 512
};

typedef enum /*? A bit field that turns on/off types of printout */
{
  TEST_PRINT_ALL = 0xffffff,  //? Print all test results
  TEST_PRINT_TEST_OK = 1, //? Print successful results
  TEST_PRINT_TEST_FAILED = 2,  //? Print failed results
} TestVerbosity;

  //? Set this global variable to control how much test detail is output
extern TestVerbosity testVerbosity;
};

namespace SAFplusI
{
    
/* Not part of the public API */

typedef struct 
{
  unsigned int passed;
  unsigned int failed;
  unsigned int malfunction;
  unsigned int malfPops;
  char         name[SAFplus::TestCaseMaxNameLen];
} ClTestCaseData;

/* Not part of the public API */
extern void clPushTestCase(const ClTestCaseData* tcd);
extern void clPopTestCase(ClTestCaseData* tcd);
extern int  clTestGroupFinalizeImpl();
extern void clTestGroupInitializeImpl();
extern void clTestPrintImpl(const char* file, int line, const char* fn, const char* c);
extern void clTestImpl(const char* file, int line, const char* function, const char * id, const char * error, int ok);

extern ClTestCaseData clCurTc;
};



/* All strings are printf formatted and in () */
#define this_error_indicates_missing_parens_around_string(...) __VA_ARGS__

#define clTestStripParens(...) __VA_ARGS__

//? <const name="CL_NO_TEST">Define the CL_NO_TEST during compilation (i.e. -DCL_NO_TEST) to eliminate all test macros from the compiled code.  This allows applications to conditionally compile versions that test themselves and "release" versions</const>
#ifndef CL_NO_TEST

#ifndef     clTestGroupInitialize
/*? Start up the Test infrastructure.
This function call starts up the test infrastructure in your program, and
tells the Test Automation Environment that this Service Group has started.
You should only call it once during your program's initialization.

<arg name="name">Identifies the test Service Group.  Must be formatted in printf style i.e. ("name").  
If you have more then one program in your SG, then give them different names with the same prefix. </arg>
<return>nothing</return>

<html>
<h3>Example:</h3>
<pre>
  clTestGroupInitialize(("My Unit Test"))
</pre>
<h3>See also:</h3>
<ref>clTestGroupFinalize()</ref>
</html>
*/
#define clTestGroupInitialize(name) do { clTestPrint(name); SAFplusI::clTestGroupInitializeImpl(); } while(0)
#endif


#ifndef     clTestGroupFinalize    
/*? Stop the Test infrastructure
<return>The number of test cases that failed.</return>

This function stops the test infrastructure.  It is critically important to call this function so that the
automation environment knows to stop your Service Group.

<ref>clTestGroupInitialize()</ref>
*/
#define clTestGroupFinalize() SAFplusI::clTestGroupFinalizeImpl()
#endif

/*? Indicate that the Test Group cannot execute, if an expression evaluates to False.

<arg name="reason">A parenthesized printf-style string describing why the test group cannot be run.</arg>
<arg name="predicate">If this expression evaluates to true, it is a malfunction, otherwise it is not.</arg>
<arg name="execOnFailure">Run this code if there is a malfunction.</arg>

<return>nothing</return>

This function is used to indicate that a set of tests have neither succeeded or failed -- some condition required
to execute the test was not met.  This function "Finalizes" the TestGroup, so after calling this function, you must
NOT use any clTest APIs
 */
#define clTestGroupMalfunction(reason, predicate, execOnFailure) \
do { \
    if (predicate) { \
        clTestPrint(reason); \
        clTestPrint(("Test Malfunction.")); \
        (void) clTestGroupFinalize(); \
        execOnFailure; \
    } \
} while(0)

/*? Run a test case.
<arg name="name">A parenthesized printf-style string describing the test case</arg>
<arg name="test">The test case code.  Most likely a function call.</arg>

A test case is a grouping of individual tests into a logical unit.  Test cases can be nested, this function can
be called inside the 'test'.  This function is syntatic sugar for: "clTestCaseStart(name); test; clTestCaseEnd();"
It is included because it is good practice to isolate your test cases into their own functions.  This macro makes
it easy to run a test case that is implemented as a function.
<html>
<h3>See also:</h3>
<ref>clTestCaseStart()</ref>, <ref>clTestCaseEnd()</ref>
</html>
*/
#define clTestCase(name, test) \
do { \
    clTestCaseStart(name); \
    SAFplusI::clCurTc.malfPops = 0; \
    test; \
    clTestCaseEnd(("\n")); \
} while (0)


/*? Start a Test Case
<arg name="testname">A parenthesized printf-style string describing the test case</arg>
<html>
<p>
A test case is a grouping of individual tests into a logical unit.  Test cases can be nested by calling this 
function multiple times with no intervening call to ClTestCaseEnd()
</p>
<h3>See also:</h3>
<ref>clTestCase()</ref>, <ref>clTestCaseEnd()</ref>
</html>
*/
#define clTestCaseStart(testname) \
do { \
    SAFplus::testPrintIndent+=2; \
    clPushTestCase(&SAFplusI::clCurTc); \
    SAFplusI::clCurTc.passed=0; \
    SAFplusI::clCurTc.failed=0; \
    SAFplusI::clCurTc.malfunction=0; \
    SAFplusI::clCurTc.malfPops = 1; \
    snprintf(SAFplusI::clCurTc.name, TestCaseMaxNameLen, \
        this_error_indicates_missing_parens_around_string testname); \
    clTestPrint(("Test case started [%s]:\n", SAFplusI::clCurTc.name)); \
} while(0)

//? Returns the number of errored (failed) tests run so far in this test case.
#define clTestCaseNumErrors() SAFplusI::clCurTc.failed

//? Return the number of malfunctioned tests in this test case (so far).
#define clTestCaseNumMalfunctions() SAFplusI::clCurTc.malfunction

//? Return the number of successful (passed) test run so far in this test case.
#define clTestCaseNumPasses() SAFplusI::clCurTc.passed

/*? Stop this test case.

<arg name="synopsis">A parenthesized printf-style string that you would like printed and stored with the results of this
test case run.  Do not print counts of the number of passed, failed, or malfunctioned tests; The function already
does that.
</arg>
<html>
A test case is a grouping of individual tests into a logical unit.  This function ends this logical unit and prints the results.
<br/>
<h3>See also:</h3>
<ref>clTestCase()</ref>, <ref>clTestCaseStart()</ref>
</html>
*/
#define clTestCaseEnd(synopsis) do { \
    clTestPrint(("Test case completed [%s]. " \
                 "Subcases: [%d] passed, [%d] failed, [%d] malfunction.\n" \
                 "Synopsis:\n", \
                 SAFplusI::clCurTc.name, SAFplusI::clCurTc.passed, \
                 SAFplusI::clCurTc.failed, SAFplusI::clCurTc.malfunction)); \
    clTestPrint(synopsis);                          \
    SAFplus::testPrintIndent-=2; \
    int malf=SAFplusI::clCurTc.malfunction; \
    int fail = SAFplusI::clCurTc.failed; \
    clPopTestCase(&SAFplusI::clCurTc); \
    if (fail) \
        SAFplusI::clCurTc.failed++; \
    else if (malf) \
        SAFplusI::clCurTc.malfunction++; \
    else \
        SAFplusI::clCurTc.passed++; \
} while(0)


/*? Indicate that the Test Group cannot execute, if an expression evaluates to False.

<arg name="reason">A parenthesized printf-style string describing why the test group cannot be run.</arg>
<arg name="predicate">If this expression evaluates to true, it is a malfunction.</arg>
<arg name="execOnFailure">Run this code if there is a malfunction.</arg>

<return>nothing</return>
<html>
This function is used to indicate that a set of tests have neither succeeded or failed -- some condition required
to execute the test was not met.  This function "Ends" the TestCase, so after calling this function you should not
call clTestCaseEnd(), but typically you would "return" or "break" in the 3rd parameter.
<br/>
<h3>See also:</h3>
<ref>clTestCase()</ref>, <ref>clTestCaseStart()</ref>, <ref>clTestCaseEnd()</ref>
</html>
*/
#define clTestCaseMalfunction(reason, predicate, execOnFailure) do { \
    if (predicate) { \
        clTestPrint(("Test case malfunction [%s]. " \
                     "Subtests: [%d] Passed, [%d] Failed.\n" \
                     "Malfunction reason:\n", \
                     SAFplusI::clCurTc.name, SAFplusI::clCurTc.passed, SAFplusI::clCurTc.failed)); \
        clTestPrint(reason); \
        if (SAFplusI::clCurTc.malfPops) clPopTestCase(&SAFplusI::clCurTc); \
        SAFplusI::clCurTc.malfunction++; \
        execOnFailure; \
    }  \
} while(0)

/*? Run an individual test.

<arg name="string">       A parenthesized printf-style string that identifies this test.</arg>
<arg name="predicate">    An expression that evaluates to True if the test succeeded, False if it failed.</arg>
<arg name="errorPrintf">  A parenthesized printf-style string that you would like printed if the predicate fails.  There is no need to print "failed!", etc since this function will do that for you.</arg>
<html>
This call printfs the first argument (but it may not be displayed depending on your verbosity settings), 
runs a test contained in the "predicate" parameter, and "printfs" the last argument if the predicate evaluates to false.
<h3>Example</h3>
<pre>
  clTest(("Foo basic"), Foo(ptr)==CL_OK,(" "));
</pre>
This example shows how printf-style strings can be used in the "string" and "errorPrintf" parameters:
<verbatim><pre>
  char* fnName = "Foo";
  char* arg = NULL;
  clTest(("Function %s with %d arg", fnName,arg), ((rc = Foo(arg))==ERROR_NULL_POINTER),("rc = %d", rc));
</pre></verbatim>
<h3>See also:</h3>
<ref>clTestExecOnFailure()</ref>
</html>
 */
#define clTest(string, predicate, errorPrintf)\
do{ \
  char __id[SAFplus::TestMaxNameLen];		\
    char __error[SAFplus::TestMaxNameLen]=""; \
    int __ok = predicate; \
    snprintf(__id, SAFplus::TestMaxNameLen, \
             this_error_indicates_missing_parens_around_string string); \
    if (!__ok) \
        snprintf(__error, SAFplus::TestMaxNameLen, \
            this_error_indicates_missing_parens_around_string errorPrintf); \
    SAFplusI::clTestImpl(__FILE__, __LINE__, __FUNCTION__, __id, __error, __ok); \
} while(0)

// do { clTestPrint(string); { int result = (predicate); if (result) { SAFplusI::clCurTc.passed++; clTestPrint((": Ok\n")); } else { SAFplusI::clCurTc.failed++; clTestPrint((": %s:%d: Test (" #predicate ") failed.  ", __FILE__, __LINE__)); clTestPrint(errorPrintf); clTestPrint(("\n")); } } } while(0)

/*? Run an individual test, and run some special code if the predicate fails.

<arg name="string">        A parenthesized printf-style string that identifies this test.</arg>
<arg name="predicate">     An expression that evaluates to True if the test succeeded, False if it failed.</arg>
<arg name="errorPrintf">   A parenthesized printf-style string that you would like printed if the predicate fails.  There is no need to print "failed!", etc since this function will do that for you.</arg>
<arg name="execOnFailure"> Run this code if the predicate is false.</arg>
<html>
This function is equivalent to clTest, except that you can pass some code to run if there is a failure.
<pre>
  clTestExecOnFailure(("Foo basic"), Foo(ptr)==CL_OK,(" "),{ emailMe(); return; });
</pre>
<h3>See also:</h3>
<ref>clTest()</ref>
</html>
*/
#define clTestExecOnFailure(string, predicate, errorPrintf, execOnFailure) \
do { \
    clTestPrint(string); \
    { \
        int result = (predicate); \
        if (result) { \
            SAFplusI::clCurTc.passed++; \
            clTestPrint((": Ok\n")); \
        } else { \
            SAFplusI::clCurTc.failed++; \
            clTestPrint((": %s:%d: Test (" #predicate ") failed.  ", \
                         __FILE__, __LINE__)); \
            clTestPrint(errorPrintf); \
            clTestPrint(("\n")); \
            execOnFailure; \
        } \
    } \
} while(0)

/*? Indicate that a test failed.
<html>
<p>Sometimes deciding whether a test passed or failed requires some complex logic.  Other times you have to write an
if() statement anyway.  This call simply indicates that the test identified by the 'string' failed.  Do not put
words like "failed", "didn't work" etc. in the string.  The library will do that for you.
</p><p>
<strong>Don't forget that strings are enclosed in their OWN parens!!!</strong>
</p><p>
Examples:</p>
<pre>
  rc = Foo()
  if (rc != CL_OK) clTestFailed(("Foo returned %d",rc));
</pre>
</html>
<arg name="__string">A parenthesized printf-style string that identifies this test</arg>
 */
#define clTestFailed(__string) clTestFailedAt(__FILE__, __LINE__,__string)

/*? Just like clTestFailed, but allows the caller to specify the file and line.
<ref>clTestFailed()</ref> is the preferred API.  Use this API when you want to wrap
this in your own helper function.  In this way the file and line can
reflect the actual file/line of your test, not that of the helper function.
*/
#define clTestFailedAt(__file, __line, __string) \
do { \
    SAFplusI::clCurTc.failed++; \
    clTestPrint((": %s:%d: Test failed.  ", __file, __line)); \
    clTestPrint(__string); \
    clTestPrint(("\n")); \
} while(0)

/*? Indicate that a test passed

<arg name="__string">A parenthesized printf-style string that identifies this test.</arg>
<html>
Sometimes deciding whether a test passed or failed requires some complex logic.  Other times you have to write an
if() statement anyway.  This call simply indicates that the test identified by the 'string' succeeded.  Do not put
words like "success", "worked", "ok" etc. in the string.  The library will do that for you.
<h3>Examples:</h3>
<pre>
rc = Foo();
if (rc != CL_OK) clTestSuccess(("Foo"));
</pre>
</html>
 */
#define clTestSuccess(__string) clTestSuccessAt(__FILE__, __LINE__, __string) 

/*? Just like clTestSuccess, but allows the caller to specify the file and line

<ref>clTestSuccess()</ref> is the preferred API.  Use this API when you want to wrap
this in your own helper function.  In this way the file and line can
reflect the file/line of your test, not that of the helper function.
*/
#define clTestSuccessAt(__file, __line, __string) \
{ \
    SAFplusI::clCurTc.passed++; \
    clTestPrintAt(__file,__line, __FUNCTION__, __string); \
    clTestPrintAt(__file,__line, __FUNCTION__, (": Ok\n")); \
} while (0)



/*? Print something to the test log/console

<arg name="x">A parenthesized printf-style string that should be printed</arg>

This function prints to the test console.  Do not use for general purpose logging, but only for items that are
specific to testing.  Also, it is much better to use the strings within the other API calls because they are
associated with a particular test or test case.  Frankly, I can't think of any reason to use this function, but
it is here to provide for the unanticipated.
 */
#define clTestPrint(x) clTestPrintAt(__FILE__, __LINE__, __FUNCTION__, x)


#ifndef clTestPrintAt    
/*? Just like clTestPrint, but allows the caller to specify the file and line

<ref>clTestPrint</ref> is the preferred API.  Use this API when you want to wrap
this in your own helper function.  In this way the file and line can
reflect the file/line of your test, not that of the helper function.
*/
#define clTestPrintAt(__file, __line, __function, x) do { \
    char __tempstr[2048]; \
    snprintf(__tempstr,2048, \
             this_error_indicates_missing_parens_around_string x); \
    SAFplusI::clTestPrintImpl(__file, __line, __function,__tempstr); \
    logStrWrite(TEST_LOG,testLogLevel,0,"TST","___",__file, __line,__tempstr); \
} while(0)
#endif
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

//? </section>
#endif
