/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : debug
 * File        : clDebugCli.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Debug parsing routines
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clDebugApi.h>


/* DEFINES */
#define MAX_ARGS		        20
#define MAX_ARG_BUF_LEN		    128
/* FIXME #define MAX_CMDS		        200 */

#define CLI_COMMAND(X, Y) 	    clModTab[X].cmdList[Y].funcName 
#define CLI_FUNC(X, Y) 		    clModTab[X].cmdList[Y].fpCallback
#define CLI_HELP_STRING(X, Y) 	    clModTab[X].cmdList[Y].funcHelp 
/* FIXME #define CLI_RESET		        argvBuf[0][0] = 0 */

#define MOD_MODNAME(X)		    clModTab[X].modName
#define MOD_LIST(X)		    clModTab[X].cmdList
#define MOD_PROMPT(X)		    clModTab[X].modPrompt
#define MOD_HELP(X)		    clModTab[X].help
#define MAX_COM_CMD         sizeof(comTab)/sizeof(comTab[0]) 

#define COM_CMDNAME(X)          comTab[X].cmdName
#define COM_HELP(X)             comTab[X].help


#define CL_TRUE			1
#define CL_FALSE		0

#define CLI_CMD_SZ              80
#define CLI_CMD_LEN             CLI_CMD_SZ
#define COMMAND_TOTAL           100


extern ClDebugModEntryT clModTab[];

/* GLOBALS */
static ClCharT *gArgv[MAX_ARGS];
static ClCharT argvBuf[MAX_ARGS*MAX_ARG_BUF_LEN];
static ClUint32T  gArgc;
static ClUint32T initState = 0;

typedef struct comEntry {
        ClCharT cmdName[80];
        ClCharT help[80];
} comEntry_t;

extern ClCharT _clDebugCliBuf[CLI_CMD_LEN];
extern void clDebugCliCommandGets(ClUint32T idx, ClCharT *ptrPrompt);
/*extern ClCharT dbgCommands[COMMAND_TOTAL][CLI_CMD_LEN];
extern ClUint32T dbgFirstCommand;
extern ClUint32T dbgLastCommand;
extern ClUint32T dbgToBeExecutedCommand;
extern ClUint32T dbgTotalCommand;*/
ClRcT clDebugCmdCompletion(ClUint32T idx, ClCharT *ptrPrompt);


static comEntry_t comTab[] = 
{
        {"end", "exit current mode"},
        {"bye",  "quit current mode"},
        {"quit", "quit current mode"},
        {"exit", "exit current mode"},
        {"help", "lists commands in current mode"},
        {"?", "lists commands in current mode"},
        {"history", "display the last 100 commands executed"},
	{"", ""}
};




/*************************************************************************/
/*                                                                       */
/* NAME: printHelp                                                       */
/*                                                                       */
/* DESCRIPTION:                                                          */
/*                                                                       */
/* ARGUMENTS:                                                            */
/*                                                                       */
/* RETURNS:                                                              */
/*                                                                       */
/*************************************************************************/
/*int main()
{
return 0;
}*/
static void
printHelp(ClUint32T index)
{
    ClUint32T idx =0;

    if (index == -1)
    {
        /* display the help of modules */
        while (MOD_LIST(idx))
        {
            printf("\r\n%s - %s", MOD_MODNAME(idx), MOD_HELP(idx));
            idx++;
        }
        printf("\r\n");
    }
    else
    {
        /* display the help of commands in a particular module */
        while(CLI_FUNC(index, idx)) 
        {
            printf("\r\n%s - %s", CLI_COMMAND(index, idx),
                    CLI_HELP_STRING(index, idx));
            idx++;
        } 
        printf("\r\n");
    }
    idx = 0;
    while(idx < MAX_COM_CMD)
    {
        printf("\r\n%s - %s", COM_CMDNAME(idx), COM_HELP(idx));
        idx++;
    }
    printf("\r\n");
}

/*************************************************************************/
/*                                                                       */
/* NAME: printPrompt                                                     */
/*                                                                       */
/* DESCRIPTION:                                                          */
/*                                                                       */
/* ARGUMENTS:                                                            */
/*                                                                       */
/* RETURNS:                                                              */
/*                                                                       */
/*************************************************************************/
static void
printPrompt(ClCharT *prompt)
{
	printf("\r\n%s", prompt);
}

/*************************************************************************/
/*                                                                       */
/* NAME: initArgvBuf                                                     */
/*                                                                       */
/* DESCRIPTION:                                                          */
/*                                                                       */
/* ARGUMENTS:                                                            */
/*                                                                       */
/* RETURNS:                                                              */
/*                                                                       */
/*************************************************************************/
static void
initArgvBuf(ClUint32T *argc, ClCharT **argv)
{
	ClUint32T idx;
	ClUint32T jdx;

	*argc = 0;
	for (idx =0; idx < MAX_ARGS; idx++)
	{
		gArgv[idx] = &argvBuf[idx*MAX_ARG_BUF_LEN];
		for (jdx = 0; jdx < MAX_ARG_BUF_LEN; jdx++)
			gArgv[idx][jdx] = 0;
	}
}


/*************************************************************************/
/*                                                                       */
/* NAME: setupArgv                                                       */
/*                                                                       */
/* DESCRIPTION:                                                          */
/*                                                                       */
/* ARGUMENTS:                                                            */
/*                                                                       */
/* RETURNS:                                                              */
/*                                                                       */
/*************************************************************************/
static void
setupArgv(ClCharT *buf)
{
	ClUint32T i = 0; 
	ClUint32T j = 0;
	ClCharT *ptr=NULL;

	gArgc = 1;
	while(buf[i]==' ') i++;
	while(buf[i] && (gArgc < MAX_ARGS+1))
	{
		ptr = gArgv[gArgc-1];
		if (buf[i] == ' ')
		{
			ptr[j] = 0;
			j = 0;
			gArgc++;
			while(buf[i]==' ') i++;
			continue;

		}
		else if ((buf[i] == '\0') || (buf[i] == 0xa) 
                         || (buf[i] == '\t'))
		{
			ptr[j] = 0;
			break;
		}
		else
		{
			ptr[j] = buf[i];
			i++;
			j++;
		}
	}
	ptr[j] = 0;
}

#if 0
/*************************************************************************/
/*                                                                       */
/* NAME: printArgBuf                                                     */
/*                                                                       */
/* DESCRIPTION:                                                          */
/*                                                                       */
/* ARGUMENTS:                                                            */
/*                                                                       */
/* RETURNS:                                                              */
/*                                                                       */
/*************************************************************************/
void
printArgBuf(ClUint32T argc, ClCharT **argv)
{
	ClUint32T idx = 0;

	printf("\nCommand : %s ", argv[idx++]);
	printf("\nClCharTacters : %c ", *argv[0]);
        printf("\nargc : %d ", argc);
	while(idx < argc)
	{
		printf("%s", argv[idx]);
		if ((idx+1) < argc)
			printf(", ");
		idx++;
	}
	fflush(stdout);
}
#endif

/*************************************************************************/
/*                                                                       */
/* NAME: clearArgBuf                                                     */
/*                                                                       */
/* DESCRIPTION:                                                          */
/*                                                                       */
/* ARGUMENTS:                                                            */
/*                                                                       */
/* RETURNS:                                                              */
/*                                                                       */
/*************************************************************************/
void
clearArgBuf(ClUint32T *argc, ClCharT **argv)
 {
	ClUint32T idx;

	for (idx = 0; idx < *argc; idx++)
		*(gArgv[idx]) = 0;
	*argc =0;
}

ClRcT  clDebugCli(ClCharT *nprompt)
{
	ClCharT prompt[80];
	ClCharT buf[80];
	static ClUint32T idx = 0;
	ClUint32T cmdIndex = 0;
	ClUint32T modFound;
    ClCharT tmpCh;
	ClUint32T cmdFound;
	ClUint32T j;
    ClUint32T oldArgc = 0;

	initArgvBuf(&gArgc, gArgv);

    printf("\nto get started type 'help intro'\n");
	sprintf(prompt, "debugCLI[%s]-> ", nprompt);

	if (!initState)
	{
		initState = CL_TRUE;
	}

	modFound = CL_FALSE; 
	while (1)
	{
		printPrompt(prompt);
		memset(buf, 0, sizeof(buf));

	        if (modFound == CL_FALSE)
                   clDebugCliCommandGets(-1, prompt);
                else
                   clDebugCliCommandGets(idx, prompt);

                strcpy (buf,_clDebugCliBuf);

		/* Check if user has entered just the new line
		don't do anything if that is the case */
		
		if ((buf[0] == '\n') || (buf[0] == '#'))
			continue;

		setupArgv(buf);
        memcpy (&tmpCh, gArgv[0], 1);
        j = strlen(gArgv[0]);

        if ((ispunct (tmpCh) != 0) && (tmpCh != '?'))
        {
           printf("\r\n Command ignored due to leading punctuation mark!\r\n");
           continue;
        }
		if (!strncasecmp(gArgv[0], "bye", j))
		{
			printf("\nGoodbye!!\n");
            /* Do the cleanup from here */
			return CL_OK;
		}
		if (!strncasecmp(gArgv[0], "exit", j))
		{
			printf("\nGoodbye!!\n");
            /* Do the cleanup from here */
			return CL_OK;
		}
		if (!strncasecmp(gArgv[0], "quit", j))
		{
			printf("\nGoodbye!!\n");
            /* Do the cleanup from here */
			return CL_OK;
		}

		if (!strncasecmp(gArgv[0], "history", j))
		{
			continue;
		}

		if ((!strncasecmp(gArgv[0], "help", j)) ||
				(!strcasecmp(gArgv[0], "?")))
		{
			if (gArgv[1][0] == '\0')
			{
				if (modFound == CL_FALSE)
					printHelp(-1);
				else
					printHelp(idx);
				continue;
			}
			else
			{
				j = strlen(gArgv[1]);
				if (!strncasecmp(gArgv[1], "end", j) ||
				    !strncasecmp(gArgv[1], "bye", j) ||
                    !strncasecmp(gArgv[1], "quit", j) ||
                    !strncasecmp(gArgv[1], "exit", j) ||
				    !strncasecmp(gArgv[1], "help", j) ||
				    !strncasecmp(gArgv[1], "history", j))
				{
					printf("\nUsage: %s\n", gArgv[1]);
					continue;
				}
				if (!strncasecmp(gArgv[1], "intro", j))
				{
					printf("\n");
					printf("The debug CLI is a commandline interface to debug the component wiht which it is integrated. There are 2 contexts levels of operation, called 'context's. It is possible to set context to and out of these levels. These levels are:\n");
					printf("\t1. generic level - in this level, only the generic commands are available\n");
					printf("\t2. component level - in this level, commands specific to the component are available\n");
					printf("The various commands supported at any level and a single line description of these commands is provided by typing 'help' on the debug CLI\n");
				}
				strcpy(gArgv[0], gArgv[1]);
				oldArgc = gArgc;
				gArgc = 0;
			}
		}
		if (modFound == CL_FALSE) 
		{
			idx = 0;
			while (MOD_LIST(idx))
			{
				if (!strncasecmp(gArgv[0], MOD_MODNAME(idx), j))
				{
					if (gArgc < 1)
					{
						printf("\nUsage: %s\n", gArgv[0]);
						break;
					}
					modFound = CL_TRUE;
					cmdIndex = 0;
					sprintf(prompt, "%s-> ", MOD_PROMPT(idx));
					break;
				}
				idx++;
			}
			if ((modFound == CL_FALSE) && (gArgc > 0))
				printHelp(-1);
		}
		else 
		{
			cmdIndex = 0;
			cmdFound = CL_FALSE;
			while (CLI_FUNC(idx, cmdIndex))
			{
				if (!strncasecmp(gArgv[0], "end", j)) 
				{
					/* go to the original level */
					modFound = CL_FALSE;
					cmdFound = CL_TRUE;
					sprintf(prompt, "DebugCLI[%s]->", nprompt);
					break;
				}
				if (!strncasecmp(gArgv[0], CLI_COMMAND(idx, cmdIndex), j))
				{
					ClCharT *retStr;
					cmdFound = CL_TRUE;
					retStr = NULL;
					CLI_FUNC(idx, cmdIndex)(gArgc, gArgv, &retStr);
					if (NULL != retStr)
					{
						printf("\r\n%s", retStr);
						clHeapFree(retStr);
					}
					break;
				}
				else
					cmdIndex++;
			}
			if (cmdFound == CL_FALSE)
			    printHelp(idx);
		}
		buf[0] = 0;
		if (!gArgc)
		{
			gArgc = oldArgc;
			oldArgc = 0;
		}
		clearArgBuf(&gArgc, gArgv);
	}
	return CL_OK;
}

#if 0
ClRcT 
showHistoryCommand(ClUint32T argc, ClCharT **argv)
{
    ClUint32T i = 0;
    ClUint32T j = 0;
    for ( i = dbgFirstCommand; j <= (dbgTotalCommand - 1); 
          i = (i + 1) % COMMAND_TOTAL, j++)
    {
	printf("%8d  %s\n\r", j, dbgCommands[i]);
    }
    return CL_OK;
}
#endif

ClRcT
clDebugCmdCompletion(ClUint32T idx, ClCharT *ptrPrompt)
{
	ClUint32T cmdIndex = 0;
	ClUint32T cmdFound = CL_FALSE;
    ClUint32T foundIdx = 0;
    ClUint32T lastIdx = 0;
    ClUint32T rcvLen = 0;
	ClUint32T i = 0;
    ClUint32T j = 0;
    ClUint32T k = 0;
    ClUint32T len = 0;
    ClUint32T tmp = 0;
    ClUint32T partialMatch = 0;
	cmdIndex = 0;
	cmdFound = CL_FALSE;
    for (i = 0; i < (CLI_CMD_SZ - 1); i++)
    {
        if (((_clDebugCliBuf[i] == 0x00) || (_clDebugCliBuf[i] == 0x20)) && 
            ((_clDebugCliBuf[i+1] == 0x00) || (_clDebugCliBuf[i+1] == 0x20)))
        {
            rcvLen = i;
            break;
        }
    }
    if (idx != -1)
    {
        partialMatch = 0;
        while (CLI_FUNC(idx, cmdIndex))
        {
            if ( (rcvLen < CL_DEBUG_FUNC_NAME_LEN) &&
                    strncasecmp(_clDebugCliBuf, CLI_COMMAND(idx, cmdIndex), rcvLen) == 0)
            {
                if (foundIdx != 0)
                {
                    if (foundIdx == 1)
                    {
                        printf("\n\rAmbiguous command! Matching commandsi" 
                                "found:");
                        fflush(stdout);
                    }
                    printf("\n\r%s", CLI_COMMAND(idx, lastIdx));
                    fflush(stdout);
                    len = strlen(CLI_COMMAND(idx, lastIdx));
                    tmp = strncasecmp(CLI_COMMAND(idx, lastIdx),
                            CLI_COMMAND(idx, cmdIndex),
                            partialMatch);
                    if (tmp != 0)
                    {
                        for (k = rcvLen; k < len; k++)
                        {
                            if (CLI_COMMAND(idx, lastIdx)[k] !=
                                    CLI_COMMAND(idx, cmdIndex)[k])
                            {
                                partialMatch = k;
                                break;
                            }
                        }
                    }
                    lastIdx = cmdIndex;
                    foundIdx++;                  
                }
                else
                {
                    partialMatch = strlen(CLI_COMMAND(idx, cmdIndex));
                    lastIdx = cmdIndex;
                    foundIdx++; 
                }
            }
            cmdIndex++;
        }
        if ((foundIdx != 1) && (foundIdx != 0))
        {
            printf("\n\r%s\n\r", CLI_COMMAND(idx, lastIdx));
            fflush(stdout);
            printPrompt(ptrPrompt);
            fflush(stdout);
            printf (" ");
            fflush(stdout);
            strncpy (&_clDebugCliBuf[0], (ClCharT *)&(CLI_COMMAND(idx, lastIdx)), 
                    partialMatch);
            _clDebugCliBuf[len] = '\0';
            for (i = 0; i < partialMatch; i++)
            {
                printf("%c",_clDebugCliBuf[i]);
                fflush(stdout);
            }
        }
        else
        {
            if (foundIdx == 1)
            {
                cmdFound = CL_TRUE;
                len = strlen(CLI_COMMAND(idx, lastIdx));
                strncpy (&_clDebugCliBuf[0],
                        (ClCharT *)&(CLI_COMMAND(idx, lastIdx)), 
                        len);
                _clDebugCliBuf[len] = '\0';
                printf("\r");
                fflush(stdout); 
                printPrompt(ptrPrompt);
                fflush(stdout);
                printf(" ");
                fflush(stdout);
                for (i = 0; i < len; i++) 
                {                             
                    printf("%c", _clDebugCliBuf[i]);
                    fflush(stdout);
                }
            }
        }
    }
   else
   {
           j = 0; 
	   while (MOD_LIST(j))
	   {
	      if (strncasecmp(_clDebugCliBuf, MOD_MODNAME(j), rcvLen) == 0)
	      {
		  if (foundIdx != 0)
		  {
                      if (foundIdx == 1)
                      {
                          printf("\n\rAmbiguous command! Matching commands" 
                                  "found:");
                          fflush(stdout);
                      }
      		      printf("\n\r%s", MOD_MODNAME(lastIdx));
	              fflush(stdout);
	              len = strlen(MOD_MODNAME(lastIdx));
                      tmp = strncasecmp(MOD_MODNAME(lastIdx),
                                    MOD_MODNAME(j),
                                    partialMatch);
                      if (tmp != 0)
                      {
                          for (k = rcvLen; k < len; k++)
                          {
                              if (MOD_MODNAME(lastIdx)[k] !=
                                  MOD_MODNAME(j)[k])
                              {
                                  partialMatch = k;
                                  break;
                              }
                          }
                      }
		      lastIdx = j;
		      foundIdx++;                  
		  }
	          else
		  {
                      partialMatch = strlen(MOD_MODNAME(j));
		      lastIdx = j;
		      foundIdx++; 
		  }
	      }
	      j++;
	   }
	   if ((foundIdx != 1) && (foundIdx != 0))
	   {
	      printf("\n\r%s\n\r", MOD_MODNAME(lastIdx));
	      fflush(stdout);
	      printPrompt(ptrPrompt);
	      fflush(stdout);
	      printf (" ");
	      fflush(stdout);
	      strncpy (&_clDebugCliBuf[0], (ClCharT *)&(MOD_MODNAME(lastIdx)), 
                       partialMatch);
	      _clDebugCliBuf[len] = '\0';
	      for (i = 0; i < partialMatch; i++)
	      {
		  printf("%c",_clDebugCliBuf[i]);
		  fflush(stdout);
	      }
	   }
	   else
	   {
              if (foundIdx == 1)
              {
	          cmdFound = CL_TRUE;
	          len = strlen(MOD_MODNAME(lastIdx));
	          strncpy (&_clDebugCliBuf[0], (ClCharT *)&(MOD_MODNAME(lastIdx)), 
                           len);
	          _clDebugCliBuf[len] = '\0';
	          printf("\r");
	          fflush(stdout); 
	          printPrompt(ptrPrompt);
	          fflush(stdout);
	          printf(" ");
	          fflush(stdout);
	          for (i = 0; i < len; i++) 
	          {                             
		      printf("%c", _clDebugCliBuf[i]);
		      fflush(stdout);
	          }
              }
	   }
    }
	if ((cmdFound == CL_FALSE) && (foundIdx == 0))
    {
           j = 0;
	   while (j < MAX_COM_CMD)
	   {
	      if (strncasecmp(_clDebugCliBuf, COM_CMDNAME(j), rcvLen) == 0)
	      {
		  if (foundIdx != 0)
		  {
                      if (foundIdx == 1)
                      {
                          printf("\n\rAmbiguous command! Matching commands "
                                  "found:");
                          fflush(stdout);
                      }
     		      printf("\n\r%s", COM_CMDNAME(lastIdx));
	              fflush(stdout);
	              len = strlen(COM_CMDNAME(lastIdx));
                      tmp = strncasecmp(COM_CMDNAME(lastIdx),
                                    COM_CMDNAME(j),
                                    partialMatch);
                      if (tmp != 0)
                      {
                          for (k = rcvLen; k < len; k++)
                          {
                              if (COM_CMDNAME(lastIdx)[k] !=
                                  COM_CMDNAME(j)[k])
                              {
                                  partialMatch = k;
                                  break;
                              }
                          }
                      }
		      lastIdx = j;
		      foundIdx++;                  
		  }
	          else
		  {
                      partialMatch = strlen(COM_CMDNAME(j));
		      lastIdx = j;
		      foundIdx++; 
		  }
	      }
	      j++;
	   }
	   if ((foundIdx != 1) && (foundIdx != 0))
	   {
	      printf("\n\r%s\n\r", COM_CMDNAME(lastIdx));
	      fflush(stdout);
	      printPrompt(ptrPrompt);
	      fflush(stdout);
	      printf (" ");
	      fflush(stdout);
	      strncpy (&_clDebugCliBuf[0], (ClCharT *)&(COM_CMDNAME(lastIdx)), 
                       partialMatch);
	      _clDebugCliBuf[len] = '\0';
	      for (i = 0; i < partialMatch; i++)
	      {
		  printf("%c",_clDebugCliBuf[i]);
		  fflush(stdout);
	      }
	   }
	   else
	   {
              if (foundIdx == 1)
              {
	          cmdFound = CL_TRUE;
	          len = strlen(COM_CMDNAME(lastIdx));
	          strncpy (&_clDebugCliBuf[0], (ClCharT *)&(COM_CMDNAME(lastIdx)), 
                           len);
	          _clDebugCliBuf[len] = '\0';
  	          printf("\r");
	          fflush(stdout); 
	          printPrompt(ptrPrompt);
	          fflush(stdout);
	          printf(" ");
	          fflush(stdout);
	          for (i = 0; i < len; i++) 
	          {                             
		      printf("%c", _clDebugCliBuf[i]);
		      fflush(stdout);
	          }
              }
	   }
    }
	if ((cmdFound == CL_FALSE) && (foundIdx == 0))
    {
        printf("\n\rNo matching commands found!\n\r");
        fflush(stdout);
	    printHelp(idx);
    }
	return (cmdFound);
}
