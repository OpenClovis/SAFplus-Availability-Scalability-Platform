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

#ifndef CL_INTERFACE_STATS_HXX
#define CL_INTERFACE_STATS_HXX

#include <clCommon.hxx>

namespace SAFplus
{

class InterfaceDevStats
{
    public:

    InterfaceDevStats(const char *intfName);
    ~InterfaceDevStats();

    // Interface Name
    std::string ifName;

    // Total no. of bytes received at the interface 
    uint64_t rxBytes;

    // Total no. of packets received at the interface.
    uint64_t rxPackets;

    // Total no. of receive errors detected by the device drivers.
    uint64_t rxErrors;

    // No. of received packets dropeed at the interface
    uint64_t rxDrop;

    // No. of FIFO buffer errors
    uint64_t rxFifo;

    // No. of packet framing errors
    uint64_t rxFrame;

    // No. of compressed packets received
    uint64_t rxCompressed;

    // No. of multicast frames received
    uint64_t rxMulticast;

    // Total no. of bytes transmitted by the interface
    uint64_t txBytes;

    // Total no. of packets transmitted by the interface.
    uint64_t txPackets;

    // Total no. of transmit errors detected by the device drivers.
    uint64_t txErrors;

    // Total no. of transmitted packets dropped at the interface.
    uint64_t txDrop;

    // Mo. of transmit FIFO buffer errors
    uint64_t txFifo;

    // No. of collisions setected at the intreface
    uint64_t txCollisions;

    // No. of carrier losses detected 
    uint64_t txCarrier;

    // No. of compressed packets transmitted.
    uint64_t txCompressed;

};

class InterfaceStatisctics
{
    public:
    typedef std::list<InterfaceDevStats *> InterfaceDevStatsList;

    // List of network device status information for each interface
    InterfaceDevStatsList ifStatsList;

    //todo: add fields from /proc/net/netstat, /proc/net/sockstat,
    // and /proc/net/protocols files.

    InterfaceStatisctics();
    ~InterfaceStatisctics();
    InterfaceStatisctics(const char *ifName);
    InterfaceDevStats *getIfStatsForIntf(const char *ifName);

    protected:

    void readInterfaceDevStats();
    void scanInterfaceDevStats(std::string contents);
};

}; /*namespace SAFplus*/
#endif
