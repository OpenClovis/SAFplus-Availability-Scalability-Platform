/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
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
#include <clTestApi.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
//#include <clCommon.hxx>
#include "clNodeStats.hxx"
#include "testOsal.hxx"


using namespace SAFplus;
using namespace SAFplusI;


TestNodeStats::TestNodeStats()
{
}

TestNodeStats::~TestNodeStats()
{
}

void TestNodeStats::testLoadAvg()
{
    
    NodeStatistics nStat;
    double ldAvg1;
    double ldAvg2;
    int32_t iloop = 0;

    ldAvg1 = nStat.loadAvg;
    while(1)
    {
        iloop++;
        if( iloop == 99999999)
        {
            NodeStatistics nStat2;
            ldAvg2 = nStat.loadAvg;
            break;
        }
    }

    //TODO: Better way is to have a seperate thread with spin loop,
    // and the main thread should wait for 1 min and then 
    // check whenther the load avg is increasing


    //ldAvg2 value will be same or graeter. because in the above 
    // does not consider the CPU utilization during one min interval
    clTest(("Load average per minute"), (ldAvg2 >= ldAvg1), ("Load average does not look valid"));

    return;
        
}

void TestNodeStats::testNodeStats()
{
    uint64_t procCount;
    NodeStatistics nStat; 

    //clTestCaseStart(("test NodeStats"));
    //testing for numProcesses
    procCount = 0;
    procCount = nStat.numProcesses;
    logInfo("NODESTAT", "TEST", "the no. of processes is %d", procCount);
    clTest(("number of process should be positive"), procCount > 0, ("Incorrect process count"));

    //testing timeSpent in user mode
    uint64_t tUserMode = 0;
    tUserMode = nStat.timeSpentInUserMode;
    clTest(("Time spent in user mode"), tUserMode > 0, ("Invalid value %lu for time spent in user mode", tUserMode));

    // testing time spent in system mode
    uint64_t tSysMode = 0;
    tSysMode = nStat.timeSpentInSystemMode;
    clTest(("time spent in system mode"), tSysMode > 0, ("Invalid value %lu for time spent in system mode", tSysMode));

    //testing the time waiting for IO to complete
    uint64_t tIoWait = 0;
    tIoWait = nStat.timeIoWait;
    clTest(("Time waiting for IO to complete"), tIoWait > 0, ("Invalid value %lu for IO waiting time", tIoWait));

    //testing time spent for servicing interrupts
    uint64_t tInterrupt = 0;
    tInterrupt = nStat.timeServicingInterrupts;
    clTest(("Time for servicing interrupts"), tInterrupt > 0, ("Invalid value %u for time taken to service interrupts", tInterrupt));


    //testing time taken for serviceing softIRQs
    uint64_t tSoftIrqs = 0;
    tSoftIrqs = nStat.timeServicingSoftIrqs;
    clTest(("Time for servicing soft IRQs"), tSoftIrqs > 0, ("Invalid value %u for time taken to service interrupts", tInterrupt));
    //clTestCaseEnd((" "));
    return;
}

void TestNodeStats::testReadUpTime()
{
    NodeStatistics nStat;
    double upTime = 0;

    upTime = nStat.sysUpTime;

    // The uptime should be definitely more than 0
    clTest(("Sytem up time"), upTime > 0, ("Invalid value %lf for system uptime", upTime));

    return;
}

void TestNodeStats::testDiskStats()
{
}

#if 0
int main()
{
    logInitialize();
    logEchoToFd = 1;  // echo logs to stdout for debugging  
    utilsInitialize();

    clTestGroupInitialize(("NodeStats"));

    TestNodeStats testObj;
    testObj.testNodeStats();

    clTestGroupFinalize();
}
#endif
