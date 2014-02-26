#include "clTransaction.hxx"
#include "clMgtProv.hxx"

#include <stdio.h>

void testTransaction()
{
    ClInt32T index;
    ClCharT inVal[] = "Value";
    ClTransaction t;

    index = t.getSize();
    if (index != 0)
    {
        printf("FAIL: Invalid ClTransaction initialization [index=%d].\n", index);
        return;
    }

    printf("PASS: ClTransaction initialization.\n");

    t.add((void *)inVal, strlen((char *)inVal) + 1);

    index = t.getSize();
    if (index != 1)
    {
        printf("FAIL: Invalid ClTransaction adding [index=%d].\n", index);
        return;
    }

    printf("PASS: ClTransaction adding.\n");

    ClCharT *outVal = (char *) t.get(index - 1);
    if ((!outVal) || (strcmp(inVal, outVal)))
    {
        printf("FAIL: Invalid getting [in value = %s] [out value = %s].\n", inVal, outVal);
        return;
    }

    printf("PASS: ClTransaction getting.\n");

    t.clean();

    index = t.getSize();
    if (index != 0)
    {
        printf("FAIL: Invalid ClTransaction cleaning [index=%d].\n", index);
        return;
    }

    printf("PASS: ClTransaction cleaning.\n");
}

void testMgtTransaction()
{
    ClTransaction t;
    ClInt32T index;
    ClCharT failVal[] = "<failtest>Value</failtest>";
    ClCharT passVal[] = "<test>Value</test>";

    ClMgtProv<std::string> testVal("test");
    testVal.Value = "Init";

    if (testVal.validate(failVal, strlen(failVal), t))
    {
        testVal.set(t);
    }
    else
    {
        testVal.abort(t);
    }

    index = t.getSize();
    if (index != 0)
    {
        printf("FAIL: Test 01 - MGT with ClTransaction - abort.\n");
        return;
    }

    if (testVal.validate(passVal, strlen(passVal), t))
    {
        testVal.set(t);
    }
    else
    {
        testVal.abort(t);
    }

    index = t.getSize();
    if (index != 1)
    {
        printf("FAIL: Test 02 - MGT with ClTransaction - set.\n");
        return;
    }

    if (testVal.Value.compare("Value") != 0 )
    {
        printf("FAIL: Test 03 - MGT with ClTransaction - check value.\n");
        return;
    }

    t.clean();

    index = t.getSize();
    if (index != 0)
    {
        printf("FAIL: Invalid ClTransaction cleaning [index=%d].\n", index);
        return;
    }

    printf("PASS: ClTransaction & MGT object.\n");
}


int main(int argc, char* argv[])
{
    testTransaction();

    testMgtTransaction();

    return 0;
}
