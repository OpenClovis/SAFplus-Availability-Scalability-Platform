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
#include "nodeStats.hxx"
#include "proc.hxx"

NodeStatistics::NodeStatistics()
{
	readLoadAvg();
    readNodeStats();
    readUpTime();
    readDiskStats();
}

NodeStatistics::~NodeStatistics()
{
}

NodeStatistics & operator-(const NodeStatistics& b)
{
}

void NodeStatistics::readLoadAvg()
{
    double avg15;
    ProcFile file("/proc/loadavg"); 
    std::string fBuf = file.getFileBuf();
    scanProcLoadAvg(fBuf);
    return;
}

void NodeStatistics::readNodeStats()
{
    ProcFile file("/proc/stat");
    std::string fBuf = file.getFileBuf();
    scanNodeStats(fBuf);
    return;
}

void NodeStatistics::readUpTime()
{
    ProcFile file("/proc/uptime");
    std::string fBuf = file.getFileBuf();
    scanUpTimeStats(fBuf);
    return;
}

void NodeStatistics::readDiskStats()
{
    ProcFile file("/proc/diskstats");
    std::string fBuf = file.getFileBuf();
    scanDiskStats(fBuf);
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
        //TODO: throw exception;
    }
    
    loadAvg = avg15;
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
            //TODO: Throw exception
        }
    }

    subStr = strstr(subStr, "ctxt ");
    if (subStr)
    {
        sscanf(subStr, "ctxt %Lu ", &numCtxtSwitches);
    }

    subStr = strstr(subStr, "btime ");
    if (subStr)
    {
        sscanf(subStr, "btime %Lu ", &bootTime);
    }


    subStr = strstr(subStr, "processes ");
    if(subStr)
    {
        sscanf(subStr, "processes %Lu ", &numProcesses);
    } 
    
    subStr = strstr(subStr, "procs_running ");
    if(subStr)
    {
        sscanf(subStr, "procs_running %Lu ", &numProcRunning);
    }

    subStr = strstr(subStr, "procs_blocked ");
    if(subStr)
    {
        sscanf(subStr, "procs_blocked %Lu ", &numProcBlocked);

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
        //Thorw exception
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
        DiskStatistics dev(device);
        sscanf(lineBuf,"   %*d    %*d %*s %u %u %llu %*u %u %u %llu %*u %u %*u %*u",
                &dev.numReadsCompleted,
                &dev.numReadsMerged,
                &dev.numSectorsRead,
                &dev.numWritesCompleted,
                &dev.numWritesMerged,
                &dev.numSectorsWritten,
                &dev.numIoInProgess);

        //TODO: now add dev into the diskStatList in NOdeStatistics class 
    
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
