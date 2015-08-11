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

#ifndef CL_NODESTATS_HXX
#define CL_NODESTATS_HXX

#include <clCommon.hxx>

namespace SAFplus
{

class statAccessErrors: public Error
{
    public:
    statAccessErrors(const char *err):Error(err)
    {
    }
};

class DiskStatistics
{
    public:
    //name of the disk device.
    std::string disKDevName;
    
    //Number of reads completed successfully on the device. 
    uint64_t numReadsCompleted;

    // Number of writes completed succesfully on the device.
    uint64_t numWritesCompleted;

    // Number of reads whcih are merged.
    // Reads which are adjacent to each other may be merged for 
    // efficiency. Thus two 4K reads may become one 8K read 
    // and it will be counted (and queued) as only one I/O.
    uint64_t numReadsMerged;

    // Number of writes which are merged.
    uint64_t numWritesMerged;

    //Number of sectors read succesfully from the device
    uint64_t numSectorsRead;

    // Number of sectors written succesfully into the device.
    uint64_t numSectorsWritten;

    // Number of I/O operations currently in progress.
    uint64_t numIoInProgess;

    enum
    {
        MaxDevNameSize = 32,
    };

    DiskStatistics(const char *devName);
    ~DiskStatistics();
};

typedef std::list<DiskStatistics *> DiskStatList;


class NodeStatistics
{
    protected:

    void readLoadAvg();
    void readNodeStats();
    void readUpTime();
    void readDiskStats();
    int  getProcessCount();

    void scanProcLoadAvg(std::string fileBuf);
    void scanNodeStats(std::string fBuf);
    void scanUpTimeStats(std::string fBuf);
    void scanDiskStats(std::string fBuf);

    public:
    void read();

    //? The avarage of CPU and IO utilization for last 1 minute.
    double loadAvg;

    //? The uptime of the system in seconds.
    double sysUpTime;

    // The list of disk I/O statistics for each disk device 
    DiskStatList diskStatList;

    //? Total time: this is the sum of all the other time spent, provided as a convenience to do %age calculations
    uint64_t timeTotal;

    //? Time spent in user mode
    uint64_t timeSpentInUserMode;

    //? Time spent in user mode within a low priority "niced" process 
    uint64_t timeLowPriorityUserMode;

    //? Time waiting for IO to complete.
    uint64_t timeIoWait; 

    //? Time spent in System Mode
    uint64_t timeSpentInSystemMode;
   
    //? Time spent for servicing Interrupts
    uint64_t timeServicingInterrupts;

    //? Time spent for servicing softirqs. 
    uint64_t timeServicingSoftIrqs;

    //? Time spent for servicing softirqs. 
    uint64_t timeIdle = 0;

    //? The number of context switches that system underwent
    uint64_t numCtxtSwitches;

    //? The time at which the system booted up, in seconds since the Epoch.
    uint64_t bootTime;

    //? Total number of processes forked since boot.
    uint64_t cumulativeProcesses;

    //? Total number of processes running now.
    uint64_t numProcesses;
    
    //? The number of processes currently in runnable state.
    uint64_t numProcRunning;

    //? Number of processes blocked waiting for I/O to complete
    uint64_t numProcBlocked;

    NodeStatistics();
    ~NodeStatistics();
    NodeStatistics operator-(const NodeStatistics& b); 

    //? Convert the values into percentages * 10 (so 543 = 54.3%) for numbers where doing so makes sense
    void percentify(void);
};

}; /*namespace SAFplus*/
#endif
