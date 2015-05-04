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
#include "clLogApi.hxx"
#include "clNodeStats.hxx"
#include "clProcFileSystem.hxx"

using namespace SAFplusI;

namespace SAFplus
{

NodeStatistics::NodeStatistics()
{
    readLoadAvg();
    readNodeStats();
    readUpTime();
    readDiskStats();
}

NodeStatistics::~NodeStatistics()
{
    DiskStatList::iterator it;
    for(it = diskStatList.begin(); it != diskStatList.end(); it++)
    {
        delete *it;
    }
}

NodeStatistics & operator-(const NodeStatistics& b)
{
    //TODO:implement this.
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

    str = const_cast<char *>(fileBuf.c_str());

    if (sscanf(str, "%lf %lf %lf", &avg1, &avg5, &avg15) < 3)
    {
        logDebug("STAT", "SCAN", "Unable to read the CPU load average");
        throw statAccessErrors("Unable to access Node Statistics");
    }
    
    loadAvg = avg15;
    logTrace("STAT", "SCAN", "CPU utilization in last 15 "
             "minutes is %lf", loadAvg);

    return;
}

void NodeStatistics::scanNodeStats(std::string fBuf) 
{
    const char *str;
    const char *subStr;

    uint64_t timeNicedProcUserMode = 0;
    uint64_t timeForIdletask = 0;

    str = fBuf.c_str();

    subStr = strstr(str, "cpu ");
    if (subStr)
    {
        if (sscanf(subStr, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu ", 
                   &timeSpentInUserMode,
                   &timeNicedProcUserMode,
                   &timeSpentInSystemMode,
                   &timeForIdletask,
                   &timeIoWait,
                   &timeServicingInterrupts,
                   &timeServicingSoftIrqs) < 7)
        {
            logDebug("STAT", "SCAN", "Unable to read Statistics");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }

    subStr = strstr(subStr, "ctxt ");
    if (subStr)
    {
        if (sscanf(subStr, "ctxt %Lu ", &numCtxtSwitches) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read number of context switches");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }

    subStr = strstr(subStr, "btime ");
    if (subStr)
    {
        if (sscanf(subStr, "btime %Lu ", &bootTime) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read system bootup time");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }


    subStr = strstr(subStr, "processes ");
    if(subStr)
    {
        if (sscanf(subStr, "processes %Lu ", &numProcesses) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read the number "
                     "of processes forked since boot");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    } 
    
    subStr = strstr(subStr, "procs_running ");
    if(subStr)
    {
        if (sscanf(subStr, "procs_running %Lu ", &numProcRunning) < 1)
        {
            logDebug("STAT", "SCAN", "Unable to read the number of "
                     "processes currently in running state");
            throw statAccessErrors("Unable to access Node Statistics");
        }
    }

    subStr = strstr(subStr, "procs_blocked ");
    if(subStr)
    {
        if (sscanf(subStr, "procs_blocked %Lu ", &numProcBlocked) < 1)
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

        if (sscanf(lineBuf,"   %*d    %*d %*s %u %u %llu %*u %u %u %llu %*u %u %*u %*u",
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
