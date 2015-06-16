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

#include "clLogApi.hxx"
#include "clNodeStats.hxx"
#include "clInterfaceStats.hxx"
#include "clProcFileSystem.hxx"

using namespace SAFplusI;

namespace SAFplus
{

InterfaceDevStats::InterfaceDevStats(const char *name):ifName(name)
{
}

InterfaceDevStats::~InterfaceDevStats()
{
}

InterfaceStatisctics::InterfaceStatisctics()
{
    readInterfaceDevStats();
}

InterfaceStatisctics::~InterfaceStatisctics()
{
    InterfaceDevStatsList::iterator it;
    for(it = ifStatsList.begin(); it != ifStatsList.end(); it++)
    {
        delete *it;
    }

}

void InterfaceStatisctics::readInterfaceDevStats()
{
    ProcFile file("/proc/net/dev");
    std::string contents = file.loadFileContents();
    if (contents.empty())
    {
       throw statAccessErrors("Unable to load file contents");
    }

    scanInterfaceDevStats(contents);
    return;
}

void InterfaceStatisctics::scanInterfaceDevStats(std::string contents)
{
    std::istringstream sStream(contents);
    std::string line;
    char *token;
    char *sp;
    char *ifLine;
    const char *lineBuf;

    //Ignore the first two lines in /proc/net/dev file
    for (uint16_t iLoop = 2; iLoop > 0; iLoop--)
    {
        std::getline(sStream, line);
        line.clear();
    }

    while(std::getline(sStream, line))
    {
        uint64_t lineLen = line.size();

        // We will do strtok on the line, which may modify the line. 
        // So making a new copy of the line. 
        ifLine = new char[lineLen + 1];
        memset(ifLine, 0, (lineLen + 1));
        std::strncpy(ifLine, line.c_str(), lineLen);

        token = strtok_r(ifLine, ": ", &sp);
        if (token == NULL)
        {
            //throe exception
        }

        InterfaceDevStats *ifStat = new InterfaceDevStats(token);

        memset(ifLine, 0, (lineLen + 1));
        std::strncpy(ifLine, line.c_str(), lineLen);

        char *subStr = strstr(ifLine, ": ");
        sscanf(subStr, "%llu %llu %llu %llu %llu %llu %llu %llu "
                       "%llu %llu %llu %llu %llu %llu %llu %llu ",
                        &(ifStat->rxBytes),
                        &(ifStat->rxPackets),
                        &(ifStat->rxErrors), 
                        &(ifStat->rxDrop),
                        &(ifStat->rxFifo),
                        &(ifStat->rxFrame),
                        &(ifStat->rxCompressed),
                        &(ifStat->rxMulticast),
                        &(ifStat->txBytes),
                        &(ifStat->txPackets),
                        &(ifStat->txErrors),
                        &(ifStat->txDrop),
                        &(ifStat->txFifo),
                        &(ifStat->txCollisions),
                        &(ifStat->txCarrier),
                        &(ifStat->txCompressed));

        //Inserting into the dev stats list
        ifStatsList.push_front(ifStat);

        line.clear();
        delete [] ifLine;
    }
    
    return;
}

InterfaceDevStats * InterfaceStatisctics::getIfStatsForIntf(const char *ifName)
{
    return NULL;
}

InterfaceStatisctics::InterfaceStatisctics(const char *ifName)
{
}

} /*namespace SAFplus*/
