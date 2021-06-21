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
#include "clNodeStats.hxx"
#include "clProcFileSystem.hxx"
#include <boost/filesystem.hpp>

using namespace SAFplusI;

namespace SAFplus
{

NodeStatistics::NodeStatistics():loadAvg(0), sysUpTime(0),
                                 timeSpentInUserMode(0),
                                 timeLowPriorityUserMode(0),
                                 timeIdle(0),
                                 timeTotal(0),
                                 timeIoWait(0),
                                 timeSpentInSystemMode(0),
                                 timeServicingInterrupts(0),
                                 timeServicingSoftIrqs(0),
                                 numCtxtSwitches(0),
                                 bootTime(0),
                                 numProcesses(0),
                                 cumulativeProcesses(0),
                                 numProcRunning(0),
                                 numProcBlocked(0),
                                 numThreadsRunning(0),
                                 numThreads(0) 
{
}


int NodeStatistics::getProcessCount()
  {
    boost::filesystem::directory_iterator end;
  int count=0;
  for (boost::filesystem::directory_iterator it("/proc"); it!=end; it++)
    {
      boost::filesystem::path p = it->path();
      std::string name = p.filename().string();
    //int sz = name.length();
    int onlyDigits = 1;
    for (auto c: name)
      {
        if (!isdigit(c)) { onlyDigits=0; break; }
      }
    count += onlyDigits;

    }
  return count;
  }

void NodeStatistics::read()
  {
    numProcesses = getProcessCount();
    readLoadAvg();
    readNodeStats();
    readUpTime();
    // TODO: SEG faults readDiskStats();
  }

NodeStatistics::~NodeStatistics()
{
    DiskStatList::iterator it;
    for(it = diskStatList.begin(); it != diskStatList.end(); it++)
    {
        delete *it;
    }
}

  NodeStatistics NodeStatistics::operator-(const NodeStatistics& b)
{
  NodeStatistics ret;
  ret.loadAvg = loadAvg - b.loadAvg;
  ret.timeTotal = timeTotal - b.timeTotal;
  ret.sysUpTime = sysUpTime - b.sysUpTime;
  
  ret.timeSpentInUserMode = timeSpentInUserMode - b.timeSpentInUserMode;
  ret.timeLowPriorityUserMode = timeLowPriorityUserMode - b.timeLowPriorityUserMode;
  ret.timeIdle = timeIdle - b.timeIdle;

  ret.timeIoWait = timeIoWait - b.timeIoWait;
  ret.timeSpentInSystemMode = timeSpentInSystemMode - b.timeSpentInSystemMode;
  ret.timeServicingInterrupts = timeServicingInterrupts - b.timeServicingInterrupts;
  ret.timeServicingSoftIrqs = timeServicingSoftIrqs - b.timeServicingSoftIrqs;

  ret.numCtxtSwitches = numCtxtSwitches-b.numCtxtSwitches;
  ret.bootTime = bootTime;  // Does not make sense to subtract a constant value
  ret.cumulativeProcesses = cumulativeProcesses-b.cumulativeProcesses;  // Now cumulativeProcesses is the number of newly spawned processes
  ret.numProcRunning = numProcRunning; // -b.numProcRunning;
  ret.numProcBlocked = numProcBlocked; //-b.numProcBlocked;
  return ret;
  // TODO disk stat list
}

void NodeStatistics::percentify()
  {  
  timeSpentInUserMode = timeSpentInUserMode*1000/timeTotal;
  timeLowPriorityUserMode = timeLowPriorityUserMode*1000/timeTotal;
  
  timeIoWait = timeIoWait*1000/timeTotal;
  timeIdle   = timeIdle*1000/timeTotal;

  timeSpentInSystemMode = timeSpentInSystemMode*1000/timeTotal;
  timeServicingInterrupts = timeServicingInterrupts*1000/timeTotal;
  timeServicingSoftIrqs = timeServicingSoftIrqs*1000/timeTotal;

  int curProcs = numProcRunning + numProcBlocked;
  numProcRunning = numProcRunning*1000/curProcs;
  numProcBlocked = numProcBlocked*1000/curProcs;
  
  }

void NodeStatistics::readLoadAvg()
{
    double avg15;
    ProcFile file("/proc/loadavg"); 
    std::string fBuf = file.loadFileContents();
    if (fBuf.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }
    scanProcLoadAvg(fBuf);
    return;
}

void NodeStatistics::readNodeStats()
{
    ProcFile file("/proc/stat");
    std::string fBuf = file.loadFileContents();
    if (fBuf.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }
    scanNodeStats(fBuf);
    return;
}

void NodeStatistics::readUpTime()
{
    ProcFile file("/proc/uptime");
    std::string fBuf = file.loadFileContents();
    if (fBuf.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }
    scanUpTimeStats(fBuf);
    return;
}

void NodeStatistics::readDiskStats()
{
    ProcFile file("/proc/diskstats");
    std::string fBuf = file.loadFileContents();
    if (fBuf.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }
    scanDiskStats(fBuf);
    return;
}

void NodeStatistics::scanProcLoadAvg(std::string fileBuf)
{
    char *str;
    double avg1;
    double avg5;
    double avg15;
    pid_t lastPidCreated;

    str = const_cast<char *>(fileBuf.c_str());

    if (sscanf(str, "%lf %lf %lf %" PRIu64 "/%" PRIu64 " %d", &avg1, &avg5, &avg15,&numThreadsRunning,&numThreads,&lastPidCreated) < 3)
    {
        logDebug("STAT", "SCAN", "Unable to read the CPU load average");
        throw statAccessErrors("Unable to access Node Statistics");
    }
    
    loadAvg = avg1;
    logTrace("STAT", "SCAN", "Avergage job in last minute is %lf", loadAvg);

    return;
}

void NodeStatistics::scanNodeStats(std::string fBuf) 
{
    const char *str;
    const char *subStr;

    str = fBuf.c_str();

    subStr = strstr(str, "cpu ");
    if (subStr)
    {
        if (sscanf(subStr, "cpu  %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " ", 
                   &timeSpentInUserMode,
                   &timeLowPriorityUserMode,
                   &timeSpentInSystemMode,
                   &timeIdle,
                   &timeIoWait,
                   &timeServicingInterrupts,
                   &timeServicingSoftIrqs) < 7)
        {
            logDebug("STAT", "SCAN", "Unable to read Statistics");
            throw statAccessErrors("Unable to access Node Statistics");
        }
        timeTotal =  timeSpentInUserMode + timeLowPriorityUserMode + timeSpentInSystemMode + timeIdle + timeIoWait + timeServicingInterrupts + timeServicingSoftIrqs;
    }

    subStr = strstr(subStr, "ctxt ");
    if (subStr)
    {
        if (sscanf(subStr, "ctxt %" PRIu64, &numCtxtSwitches) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read number of context switches");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }

    subStr = strstr(subStr, "btime ");
    if (subStr)
    {
        if (sscanf(subStr, "btime %" PRIu64, &bootTime) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read system bootup time");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }


    subStr = strstr(subStr, "processes ");
    if(subStr)
    {
      if (sscanf(subStr, "processes %" PRIu64, &cumulativeProcesses) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read the number "
                     "of processes forked since boot");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    } 
    
    subStr = strstr(subStr, "procs_running ");
    if(subStr)
    {
        if (sscanf(subStr, "procs_running %" PRIu64, &numProcRunning) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read the number of "
                     "processes currently in running state");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }

    subStr = strstr(subStr, "procs_blocked ");
    if(subStr)
    {
        if (sscanf(subStr, "procs_blocked %" PRIu64, &numProcBlocked) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read the number of "
                     "processes blocked for an I/O to complete");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }

    return;
}

void NodeStatistics::scanUpTimeStats(std::string fBuf)
{
    const char *str;
    uint32_t ret;

    str = fBuf.c_str();

    if (sscanf(str, "%lf ", &sysUpTime) < 1)
    {
        throw statAccessErrors("Unable to access Node Statistics");
    }

    return;
}

void NodeStatistics::scanDiskStats(std::string fBuf)
{
    std::istringstream sStream(fBuf);
    std::string line;
    const char *lineBuf;

    char *device = new char[DiskStatistics::MaxDevNameSize + 1];

    while(std::getline(sStream, line))
    {
        //std::fill(device, device + strlen(device), '\0');
        memset(device, 0, strlen(device));

        lineBuf = line.c_str();
        sscanf(lineBuf, "   %*d    %*d %s", device);
        DiskStatistics *dev = new DiskStatistics(device);

        if (sscanf(lineBuf,"   %*d    %*d %*s %" PRIu64 " %" PRIu64 " %" PRIu64 " %*d %" PRIu64 " %" PRIu64 " %" PRIu64 "%*d %" PRIu64 "%*d %*d",
                &(dev->numReadsCompleted),
                &(dev->numReadsMerged),
                &(dev->numSectorsRead),
                &(dev->numWritesCompleted),
                &(dev->numWritesMerged),
                &(dev->numSectorsWritten),
                &(dev->numIoInProgess)) < 7)
        {
            logDebug("STAT", "SCAN", "Unable to read the disk statistics");
            throw statAccessErrors("Unable to access Node Statistics");
        }

	    //All the device entries will be put into the list and 
	    //later will be deleted in the destructor
	    diskStatList.push_front(dev);
	 
        line.clear(); 
    }

    delete [] device;
    return; 
}

DiskStatistics::DiskStatistics(const char *devName)
{
    disKDevName = devName;
}

DiskStatistics::~DiskStatistics()
{
}

}/* namespace SAFplus */
