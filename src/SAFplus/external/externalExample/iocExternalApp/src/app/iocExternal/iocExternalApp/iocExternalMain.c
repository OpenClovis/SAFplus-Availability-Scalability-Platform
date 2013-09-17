#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clHeapApi.h>
#ifdef USE_EO
#include <clEoApi.h>
#endif
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocParseConfig.h>
#include "iocExternalDefs.h"
#include "iocExternalReceiver.h"
#include "iocExternalSender.h"

extern ClBoolT gIsNodeRepresentative;
static ClIocConfigT *gpClIocConfig;
ClInt32T clAspLocalId;

typedef enum {
  UDP = 1,
  TCP,
  TIPC,
  IOC
}ClProtocolsT;
ClUint8T clEoBasicLibs[1];
ClUint8T clEoClientLibs[1];
//ClEoConfigT clEoConfig;
extern ClHeapConfigT *pHeapConfigUser;
static ClHeapConfigT heapConfig = {.mode = CL_HEAP_NATIVE_MODE };
static ClEoMemConfigT memConfig = {.memLimit = 0 };
void
usage(int exit_code)
{
    printf("Usage:\n");
    printf("To run external IOC App as a receiver:\n");
    printf("\n");
    printf("  externalIoc -r -P <protocol> -l <local-address> [-p <port>] -m <mode> -R <node-rep>\n");
    printf("\n");
    printf("To external IOC App as a sender:\n");
    printf("  externalIoc -t -P <protocol> -l <local-address> -d <dest-address> [-p <dest-port>]\n");
    printf("         -R <node-rep>\n");
    printf("\n");
    printf("Where:\n");
    printf("  <protocol>      communication protocol to be used(e.g. UDP, TCP, IOCu, IOCr, TIPCu, TIPCr\n");
    printf("                  IOCu  - unreliable IOC(IOC over TIPC or UDP depending on compilation)\n"
           "                  IOCr  - reliable IOC(IOC over TIPC only)\n");
    printf("  <local-address> For IOC  : Local IOC node address (e.g., 1, 2, ...)\n");
    printf("  <dest-address>  For IOC  : Receiver's IOC node address (e.g., 1, 2, ...)\n");
    printf("  <dest-port>     IOC/UDP port of receiver (default is 7777)\n");
    printf("  <node-rep>      1 : It is a node representative application\n");
    printf("                  0 : Not a node representative application\n");
    printf("\n");
    
    exit(exit_code);
}

static ClRcT clMemInitialize(void)
{
    ClRcT rc;

    if((rc = clMemStatsInitialize(&memConfig)) != CL_OK)
    {
        return rc;
    }
    pHeapConfigUser =  &heapConfig;
    if((rc = clHeapInit()) != CL_OK)
    {
        return rc;
    }
    return CL_OK;
}

ClRcT
Initialize ( ClInt32T ioc_address_local )
{
    ClRcT rc = CL_OK;
    
	clAspLocalId = ioc_address_local;

    rc = clIocParseConfig(NULL, &gpClIocConfig);
    if(rc != CL_OK)
    {
        clOsalPrintf("Error : Failed to parse clIocConfig.xml file. error code = 0x%x\n",rc);
        exit(1);
    }

    if ((rc = clOsalInitialize(NULL)) != CL_OK)
    {
        printf("Error: OSAL initialization failed\n");
        return rc;
    }
    if ((rc = clMemInitialize()) != CL_OK)
    {
        printf("Error: Heap initialization failed\n");
        return rc;
    }
    if ((rc = clTimerInitialize(NULL)) != CL_OK)
    {
        printf("Error: Timer initialization failed\n");
        return rc;
    }
    if ((rc = clBufferInitialize(NULL)) != CL_OK)
    {
        printf("Error: Buffer initialization failed\n");
        return rc;
    }
    return rc;
}

int
main(int argc, char **argv)
{
    int role = 0;                /* 1: receiver, 2: sender */
    ClProtocolsT protocol_code = 0 ;
    int mode = 0;                /* FIXME: need to define various modes */
    int ioc_port = DEF_IOC_PORT; /* IOC port number, default is DEF_IOC_PORT */
    int ioc_address_local = -1;
    int ioc_address_dest = -1;
    int nodeRep = 0;
    extern ClIocConfigT pAllConfig;
    printf("start ioc benmark\n");
    int socket_type;
    char protocol[LOCAL_BUF_SIZE];
    char local_addr[LOCAL_BUF_SIZE];
    char dest_addr[LOCAL_BUF_SIZE];
    int optch;
    static char stropts[] = "hrtTl:P:p:d:s:n:b:i:m:R:";
    ClRcT rc = CL_OK;

	memset( local_addr, 0, LOCAL_BUF_SIZE );
	memset( dest_addr, 0, LOCAL_BUF_SIZE );
    /*
     * Command line parsing
     */
    while ( ( optch = getopt(argc, argv, stropts)) != EOF)
    {
	switch ( optch )
	    {
	        case 0:
		        break;
                
	        case 'r':
                if (role != 0) /* has already be set to be a receiver */
                {
                    printf("Error: Role already set to be a receiver.\n");
                    usage(1);
                }
                role = 1;
                break;
            case 't':
                if (role != 0) /* has already be set to be a sender */
                {
                    printf("Error: Role already set to be a sender\n");
                    usage(1);
                }
                role = 2;
                break;
            case 'P':
                strcpy( protocol, optarg);
                break;
            case 'l':
				strcpy( local_addr, optarg );
				break;
            case 'p':
		        if (sscanf(optarg, "%d", &ioc_port) != 1)
                {
                    printf("Error: Bad port number: %s\n", optarg);
                    usage(1);
                }
		        break;
            case 'd':
				strcpy( dest_addr, optarg );
				break;

            case 'm':
		        if (sscanf(optarg, "%d", &mode) != 1 ||
                    mode < 0 ||
                    mode > 0 /* for now */)
                {
                    printf("Error: Bad mode number: %s\n", optarg);
                    usage(1);
                }
		        break;
            case 'R':
                if(sscanf(optarg, "%d", &nodeRep) != 1)
                {
                    printf("Error: Bad node rep code %s\n", optarg);
                    usage(1);
                }
                break;
            case 'h':
            default:
                   usage(1);
        }
    }

    /*
     * Post parsing checks
     */
    if (role == 0)
    {
        printf("Error: Specify one of -t, -r, -T, or -R\n");
        usage(1);
    }
#if 0
	else
	{
		switch (role)
		{
		case 2:
			if (sscanf(dest_addr, "%d", &ioc_address_dest) != 1)
            {
                printf("Error: Bad dest address: %s\n", dest_addr);
                usage(1);
            }
			/* no break here, intentionally */
		case 1:
		    if (sscanf(local_addr, "%d", &ioc_address_local) != 1)
            {
                printf("Error: Bad local address: %s\n", optarg);
                usage(1);
            }
			break;
		}
	}

    if (((role == 1) || (role == 2)) && (ioc_address_local == -1))
    {
        printf("Error: Local IOC address has to be defined\n");
        usage(1);
    }
    if ((role == 2) && (ioc_address_dest == -1))
    {
        printf("Error: In transmitter mode receiver's address must be set\n");
        usage(1);
    }
#endif
   

    
    if(strcmp(protocol, "IOCu") == 0) {
        protocol_code = IOC;
        socket_type = CL_IOC_UNRELIABLE_MESSAGING;
    } else if(strcmp(protocol, "IOCr") == 0) {
        protocol_code = IOC;
        socket_type = CL_IOC_RELIABLE_MESSAGING;
    } 
    else {
        printf("Error : Value for -P option that you have passed is not supported.\n");
        usage(1);
    }
    
    

    /*
  	 * Start sender or receiver, respectively
   	 */
    switch(protocol_code)
    {
    case IOC:
	    if (sscanf(local_addr, "%d", &ioc_address_local) != 1)
        {
            printf("Error: Bad local address: %s\n", optarg);
            usage(1);
        }
        rc = Initialize( ioc_address_local );
        if (rc != CL_OK)
        {
            printf("Error: failed to Initialize ASP libraries\n");
            exit(1);
        }
        if(nodeRep == 1)
        {
            pAllConfig.iocConfigInfo.isNodeRepresentative = CL_TRUE;
            gIsNodeRepresentative = CL_TRUE;
        }
        else
            pAllConfig.iocConfigInfo.isNodeRepresentative = CL_FALSE;

        if ((rc = clIocLibInitialize(NULL)) != CL_OK)
        {
            printf("Error: IOC initialization failed with rc = 0x%x\n", rc);
            exit(1);
        }
        
        if(role == 1) {
        	externalIocReceiverStart(socket_type, ioc_address_local, ioc_port, mode);
        } else {
			if (sscanf(dest_addr, "%d", &ioc_address_dest) != 1)
            {
                printf("Error: Bad dest address: %s\n", dest_addr);
                usage(1);
            }
        	externalIocSenderStart(socket_type,
                              ioc_address_local,
                          	  ioc_address_dest,
                          	  ioc_port,
                          	  mode);
        } 
        break;    
    default :
        printf("not support");
        break;
    }
	

    /*
     * Cleanup phase
     */
    /* FIXME: need to be cleanly exiting */
    
	if ( protocol_code == IOC )
	{
		clIocLibFinalize();
	}
    exit(0);
}
    
