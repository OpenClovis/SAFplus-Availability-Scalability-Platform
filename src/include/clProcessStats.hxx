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

#ifndef CL_PROCESS_STATS_HXX
#define CL_PROCESS_STATS_HXX

#include <list>
#include "clCommon.hxx"

namespace SAFplus
{

class ProcStats
{
    public:

    ProcStats(int32_t pid);
    ~ProcStats();

    // Process ID.
    int32_t pid;

    // Enum which defines the different states of process
    typedef enum 
    {
        Running = 0,
        Sleeping,
        WaitingInDiskSleep,
        Zombie,
        Stopped,
        TracingStop, /*(Linux 2.6.33 onward)*/
        Paging, /*(only before Linux 2.6.0)*/
        Dead, /*(Linux 2.6.33 to 3.13 only)*/
        Wakekill, /*(Linux 2.6.33 to 3.13 only)*/
        Waking, /*(Linux 2.6.33 to 3.13 only)*/
        Parked, /*(Linux 3.9 to 3.13 only)*/
        InvalidState,
    } ProcessState;

    // The enum value showing the process state at the 
    // time when the instance of this class is created.
    ProcessState procState;

    // The number of minor faults the process has made 
    //which have not required loading a memory page from disk
    uint64_t numMinorFaults;

    // The number of major faults the process has made 
    // which have required loading a memory page from disk
    uint64_t numMajorFaults;

    // Amount of time that process has been scheduled in user mode.
    uint64_t timeSchedInUserMode;

    // Amount of time that process has been scheduled in kernel mode.
    uint64_t timeSchedInKernelMode;

    // Number of threads in this process 
    uint64_t numThreads;

    // Virtual memory size(in bytes) which the process is using
    uint64_t virtualMemSize;

    // Resident Set Size: no. of pages the process has in real memory.
    uint64_t residentSetSize;

    // Guest time of the process (ie. time spent running a virtual 
    // CPU for a guest operating system), measured in clock ticks
    uint64_t guestTime;

    // The number of shared memory pages. The total available 
    // shared memory space will be organized into pages of fixed size. 
    uint64_t sharedPages;
    
    // Memory which the stack and data segments of the process takes.
    uint64_t stackAndDataSize;

    protected:
    void readProcStat();
    void readProcMemStat();
    void scanProcStats(std::string contents);
    void scanProcMemStats(std::string contents);
    void findProcessState(char state);
};

class ProcessStatistics
{
    public:
    typedef std::list<ProcStats *> ProcStatsList;

    // The list of stats corresponding to each processs.
    ProcStatsList procStatsList; 

    ProcessStatistics();
    ProcessStatistics(int32_t pid);
    ~ProcessStatistics();
    ProcStats *getProcStatsForPID(int32_t pid);
};

}; /*namespace SAFplus*/
#endif
