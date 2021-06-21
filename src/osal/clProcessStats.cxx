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

#include <string>
#include "clLogIpi.hxx"
#include "clProcFileSystem.hxx"
#include "clProcessApi.hxx"
#include "clProcessStats.hxx"
#include "clNodeStats.hxx"

using namespace SAFplusI;

namespace SAFplus
{

ProcessStatistics::ProcessStatistics()
{
    ProcessList processes;

    ProcessList::ProcList::iterator it;
    for (it = processes.pList.begin(); it != processes.pList.end(); ++it)
    {
        ProcStats *pStat = new ProcStats((*it)->pid);
        procStatsList.push_back(pStat);
    }
    
}

ProcessStatistics::ProcessStatistics(int32_t pid)
{
}

ProcessStatistics::~ProcessStatistics()
{
    ProcStatsList::iterator it;
    for (it = procStatsList.begin(); it != procStatsList.end(); ++it)
    {
        delete *it;
    }
}


ProcStats *ProcessStatistics::getProcStatsForPID(int32_t pid)
{
}

ProcStats::ProcStats(int32_t processsID):pid(processsID)
{
    readProcStat();
    readProcMemStat();
}

ProcStats::~ProcStats()
{
}

void ProcStats::readProcStat()
{
    char path[256];
    memset(path, 0, sizeof(path));

    sprintf(path, "/proc/%d/stat", pid);

    ProcFile file(path);
    std::string fileContents = file.loadFileContents();
    if (fileContents.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }
    
    scanProcStats(fileContents);
    return;
}

void ProcStats::readProcMemStat()
{
    char path[256];
    memset(path, 0, sizeof(path));

    sprintf(path, "/proc/%d/statm", pid);

    ProcFile file(path);
    std::string fileContents = file.loadFileContents();
    if (fileContents.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }
    
    scanProcMemStats(fileContents);
    return;
}

void ProcStats::findProcessState(char state)
{
    switch (state)
    {
    
    case 'R':
        procState = ProcStats::ProcessState::Running;
        break;

    case 'S':
        procState = ProcStats::ProcessState::Sleeping;
        break;

    case 'D':
        procState = ProcStats::ProcessState::WaitingInDiskSleep;
        break;

    case 'Z':
        procState = ProcStats::ProcessState::Zombie;
        break;

    case 'T':
        procState = ProcStats::ProcessState::Stopped;
        break;

    case 't':
        procState = ProcStats::ProcessState::TracingStop;
        break;
#if 0
    case 'W':
        procState = ProcStats::ProcessState::Paging;
        break;
#endif

    case 'x': case 'X' :
        procState = ProcStats::ProcessState::Dead;
        break;

    case 'K':
        procState = ProcStats::ProcessState::Wakekill;
        break;

    case 'W':
        procState = ProcStats::ProcessState::Waking;
        break;

    case 'P':
        procState = ProcStats::ProcessState::Parked;
        break;

    default:
        procState = ProcStats::ProcessState::InvalidState;
        break;
    }

    return;
}

void ProcStats::scanProcStats(std::string contents)
{
    const char *str;
    char state;

    str = contents.c_str();
    
    // For the debugger to make it easy:
    // each line of scan has five elements.
    // the fields we are trying to read are
    // 3rd, 10th, 12th, 14th, 15th, 20th and 43rd.
    if (sscanf(str, "%*d %*s %c %*d %*d "
                "%*d %*d %*d %*u %" PRIu64 " "
                "%*u %" PRIu64 " %*u %" PRIu64 " %" PRIu64 " "
                "%*d %*d %*d %*d %" PRIu64 " "
                "%*d %*u %*u %*d %*u "
                "%*u %*u %*u %*u %*u "
                "%*u %*u %*u %*u %*u "
                "%*u %*u %*d %*d %*u "
                "%*u %*u %" PRIu64 " ",
                &state,
                &numMinorFaults,
                &numMajorFaults,
                &timeSchedInUserMode,
                &timeSchedInKernelMode,
                &numThreads,
                &guestTime) < 7)
    {
        logDebug("STAT", "SCAN", "Unable to read the process statictics");
        throw statAccessErrors("Unable to access process statistics");
    }
    
    findProcessState(state);
    return;
}

void ProcStats::scanProcMemStats(std::string contents)
{
    const char *str;
    str = contents.c_str();

    if (sscanf(str, "%" PRIu64 " %" PRIu64 " %" PRIu64 " %*u %*u %" PRIu64 " %*u",
                &virtualMemSize, &residentSetSize,
                &sharedPages, &stackAndDataSize) < 4)
    {
        logDebug("STAT", "SCAN", "Unable to read the process memory "
                 "related statictics");
        throw statAccessErrors("Unable to access process memory statistics");
    }

    return;
}

} /*namespace SAFplus*/
