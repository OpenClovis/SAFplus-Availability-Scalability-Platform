#include <clIocDebugCli.h>
#include <clDebugApi.h>
#include <clTransport.h>

void iocCliPrint(char *str,ClCharT ** ret)
{
     *ret = clHeapAllocate(strlen(str)+1);
      if(NULL == *ret)
      {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Malloc Failed \r\n"));
           return;
      }
      snprintf(*ret, strlen(str)+1, str);
      return;
}

ClRcT clIocDebugCliTransportMcastPeerListAdd(int          argc, 
        char         **argv ,
        ClCharT      **ret)
{
    if (argc != 2)
    {
        iocCliPrint("Usage : mcastPeerListAdd <peerAddr>\n"
                     "peerAddr            [STRING]     peer address to be added to the multicast list\n", ret);
        return 0;
    }
    
    ClRcT rc = clTransportMcastPeerListAdd(argv[1]);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
    {
        iocCliPrint("Duplicated peer address in the list\n", ret);        
    }
    
    return rc;   
}
ClRcT clIocDebugCliTransportMcastPeerListDelete(int          argc, 
        char         **argv ,
        ClCharT      **ret)
{
    if (argc != 2)
    {
        iocCliPrint("Usage : mcastPeerListDelete <peerAddr>\n"
                     "peerAddr            [STRING]     peer address to be deleted from the multicast list\n", ret);
        return 0;
    }

    ClRcT rc = clTransportMcastPeerListDelete(argv[1]);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        iocCliPrint("peer address not exist in the list\n", ret);
    }
    return rc;
}
ClRcT clIocDebugCliTransportMcastPeerListGet(int          argc, 
        char         **argv ,
        ClCharT      **ret)
{
    if (argc != 1)
    {
        iocCliPrint("Usage : mcastPeerListGet\n", ret);
        return 0;
    }

    ClDebugPrintHandleT inMsg = 0;

    ClIocAddrMapT *peers;
    peers = clHeapCalloc(CL_MCAST_MAX_NODES, sizeof(*peers));

    ClUint32T pNumPeers;

    ClRcT rc = clTransportMcastPeerListGet(peers, &pNumPeers);
    if(rc != CL_OK)
    {
        clLogDebug("MCAST", "MAP", "Mcast peer list get failed with [%#x]", rc);
        clHeapFree(peers);
        return rc;
    }

    clLogDebug("MCAST", "MAP", "NumPeerListGet: [%d]", pNumPeers);

    clDebugPrintInitialize(&inMsg);
    clDebugPrint(inMsg,"%s\n","McastPeerListGet:");
    ClUint32T i;
    for(i = 0; i < pNumPeers; ++i)
    {
        clDebugPrint(inMsg,"%s\n",peers[i].addrstr);
        clLogDebug("MCAST", "MAP", "[%s]", peers[i].addrstr);
    }
    clDebugPrintFinalize(&inMsg,ret);

    clHeapFree(peers);

    return rc;
}
