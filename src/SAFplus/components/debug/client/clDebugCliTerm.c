/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
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
/*******************************************************************************
 * ModuleName  : debug
 * File        : clDebugCliTerm.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains routine for getting command for DebugCli.
 *****************************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#ifndef VXWORKS_BUILD
#include    <termios.h>
#endif
#include    <unistd.h>
#include    <clCommon.h>

#define MAX_ARGS		20
#define MAX_ARG_BUF_LEN	128

#define	COMMAND_TOTAL	100
#define	CLI_CMD_SZ	(MAX_ARGS*MAX_ARG_BUF_LEN)/* This is the maximun length of a command */
#define	CLI_CMD_LEN	CLI_CMD_SZ	/* The maximun of a command */
#define CL_FALSE           0


ClCharT _clDebugCliBuf[CLI_CMD_LEN];
extern ClUint32T okToSetSettings;

void clDebugCliCommandGets (ClUint32T idx, ClCharT *ptrPrompt);
static ClCharT dbgCommands[COMMAND_TOTAL][80];
static ClUint32T dbgFirstCommand = 0;
static ClUint32T dbgLastCommand = 0;
static ClUint32T dbgInputSetting = 0;
#ifndef VXWORKS_BUILD
static ClUint32T dbgLocalSetting = 0;
#endif
static ClUint32T dbgToBeExecutedCommand = 0;
static ClUint32T dbgTotalCommand = 0;
extern ClUint32T clDebugCmdCompletion (ClUint32T idx, ClCharT *ptrPrompt);
/*****************************************************************************/

/*****************************************************************************/
/**
* This function sets the termial to raw and no echo mode.
*
* @return	None
*
******************************************************************************/
#ifdef VXWORKS_BUILD

static void
dbgSetSettings ()
{
    dbgInputSetting=ioctl(0,FIOGETOPTIONS,0);
    ioctl(0,FIOSETOPTIONS,OPT_CRMOD | OPT_TANDEM | OPT_7_BIT); /* set raw */
}

static void
dbgRestoreSettings ()
{
    ioctl(0, FIOSETOPTIONS, dbgInputSetting);
}

#else

#define CL_INPUT_SETTING  (~INPCK & ~IGNPAR & ~PARMRK & ~IGNPAR & \
                           ~ISTRIP & ~IGNBRK & ~BRKINT & ~IGNCR & \
                           ~INLCR & ~ICRNL & ~IXON & ~IXOFF & \
                           ~IXANY & ~IMAXBEL)

#define CL_LOCAL_SETTING (~ECHO & ~ICANON)

static void
dbgSetSettings ()
{
#if 1
  struct termios settings;
  ClUint32T result = 0;

  memset ((void *) &settings, 0, sizeof (struct termios));

  result = tcgetattr (STDIN_FILENO, &settings);
  if (result != 0)
    {
      return;
    }
  dbgInputSetting = settings.c_iflag;
  dbgLocalSetting = settings.c_lflag;

  settings.c_iflag &= (CL_INPUT_SETTING);
  settings.c_lflag &= CL_LOCAL_SETTING;

  result = tcsetattr (STDIN_FILENO, TCSANOW, &settings);
  if (result != 0)
    {
      return;
    }
#else
  system ("stty raw");
  system ("stty -echo");
#endif
}

static void
dbgRestoreSettings ()
{
#if 1
  struct termios settings;
  ClUint32T result = 0;

  memset ((void *) &settings, 0, sizeof (struct termios));

  result = tcgetattr (STDIN_FILENO, &settings);
  if (result != 0)
    {
      return;
    }
  settings.c_iflag |= dbgInputSetting;
  settings.c_lflag |= dbgLocalSetting;

  result = tcsetattr (STDIN_FILENO, TCSANOW, &settings);
  if (result != 0)
    {
      return;
    }
#else
  system ("stty -raw");
  system ("stty echo");
#endif
}

#endif

/*****************************************************************************/
/**
* This function gets a line of command from cli prompt.
*
* @return	None
*
******************************************************************************/
void
clDebugCliCommandGets (ClUint32T idx, ClCharT *ptrPrompt)
{
  ClUint32T c;
  ClInt32T  his;
  ClUint32T commandDone = 0;
  ClUint32T i, index = 0, j, jj, toBeCopied, l, maxLenth = 0;
  ClUint32T ret = 0;

  for (j = 0; j < CLI_CMD_LEN - 1; j++)
  {
      _clDebugCliBuf[j] = ' ';
  }
  _clDebugCliBuf[CLI_CMD_LEN - 1] = '\0';

  index = 0;
  maxLenth = 0;
  fflush (stdin);

  dbgSetSettings ();

  while (commandDone == 0)
  {
      c = getchar ();
      switch (c)
	  {
	     case '\t':		/* tab */
	        ret = clDebugCmdCompletion (idx, ptrPrompt);
	        for (i = 0; (i < CLI_CMD_SZ - 1); i++)
	        {
	            if (((_clDebugCliBuf[i] == 0x00) ||
		        (_clDebugCliBuf[i] == 0x20)) &&
	            ((_clDebugCliBuf[i + 1] == 0x00) ||
        	    (_clDebugCliBuf[i + 1] == 0x20)))
		        {
		            maxLenth = index = i;
    		        break;
	    	    }
	        }
    	    if (ret == CL_FALSE)
	            break;
	        else
        	    continue;
	    case 13:		/* new line */
	    case '\n':		/* new line */
/*
                        maxLenth = index;
*/
       	     dbgRestoreSettings ();
	         printf ("\n");
        	 fflush (stdout);
        	 commandDone = 1;
    	     _clDebugCliBuf[maxLenth] = '\0';
        	 if (strlen (_clDebugCliBuf) > 0)
             {
	            if (dbgTotalCommand < COMMAND_TOTAL)
    	        {
	    	         dbgTotalCommand++;
    		    }
	            else
		        {
    	             dbgFirstCommand = (dbgFirstCommand + 1) % COMMAND_TOTAL;
	    	    }
	            if (_clDebugCliBuf[0] == '#')
                {
                   strcpy (dbgCommands[dbgLastCommand], &(_clDebugCliBuf[1]));
	            }
                else
	            {
	               strcpy (dbgCommands[dbgLastCommand], _clDebugCliBuf);
	            }
	            dbgLastCommand = (dbgLastCommand + 1) % COMMAND_TOTAL;
	            dbgToBeExecutedCommand = dbgLastCommand;
	         }
    	     _clDebugCliBuf[maxLenth] = '\n';
	         _clDebugCliBuf[maxLenth + 1] = '\0';
	         break;
	    case 0x7f:		/* Delete char */
    	     c = 0x8;
	    case 0x8:		/* back space */
	        if (index > 0)
	        {
	            printf ("%c", c);
    	        fflush (stdout);
    	        index--;
    	        for (j = index; j < maxLenth; j++)
    		    {
    		        _clDebugCliBuf[j] = _clDebugCliBuf[j + 1];
    		    }

	            toBeCopied = maxLenth - index;
	            for (l = index; l < maxLenth; l++)
		        {
		            if (_clDebugCliBuf[l] == '\0')
    		        {
    		             printf ("%c", ' ');
    		             fflush (stdout);
    		        }
    		        else
    		        {
                         printf ("%c", _clDebugCliBuf[l]);
    		             fflush (stdout);
    		        }
    		    }
    	        for (l = 0; l < toBeCopied; l++)
    		    {
                	  printf ("%c%c%c", 0x1b, 0x5b, 0x44);
            		  fflush (stdout);
    	    	}
                maxLenth--;
          }
    	  break;
	    case 0x1b: /*ESC */
	          c = getchar ();
              if (c == 0x5b) /*[ */
	          { 
    	           c = getchar ();
        	       if (c == 0x44)	/* left */
    	           {
        	            if (index > 0)
    	                {
            		        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                	        fflush (stdout);
        		            index--;
    	                }
	               }
	               else if (c == 0x43)	/* right */
		           {
        		      if (index < maxLenth)
    		          {
            		      printf ("%c%c%c", 0x1b, 0x5b, 0x43);
            		      fflush (stdout);
    		              index++;
    		           }
    		       }
                   else if (c == 0x41)	/* up */
		           {
    		           while (index > 0)
    		           {
            		      printf ("%c%c%c", 0x1b, 0x5b, 0x44);
    		              fflush (stdout);
    		              index--;
    		           }
	    	           while (index < maxLenth)
		               {
    	         	      printf (" ");
    		              fflush (stdout);
    		              index++;
    		           }
	    	           j = 0;
		               do
		               {
    		             dbgToBeExecutedCommand =
    			         (dbgToBeExecutedCommand + COMMAND_TOTAL -
    			         1) % COMMAND_TOTAL;
    		             j++;
                       } while (!strlen (dbgCommands[dbgToBeExecutedCommand])
			                && (j < COMMAND_TOTAL));
             		   strcpy ( _clDebugCliBuf,
                                dbgCommands[dbgToBeExecutedCommand]);
    		           while (index > 0)
    		           {
            		      printf ("%c%c%c", 0x1b, 0x5b, 0x44);
		                  fflush (stdout);
    		              index--;
	    	           }
    		           printf ("%s", _clDebugCliBuf);
	    	           fflush (stdout);
    		           maxLenth = index = strlen (_clDebugCliBuf);
	    	       }
    	           else if (c == 0x42)	/* down */
	    	       {
    		          while (index > 0)
    		          {
    		              printf ("%c%c%c", 0x1b, 0x5b, 0x44);
    		              fflush (stdout);
    		              index--;
    		          }
    		          while (index < maxLenth)
    		          {
    		             printf (" ");
    		             fflush (stdout);
    		             index++;
    		          }
    		          j = 0;
    		          do
	    	          {
    		              dbgToBeExecutedCommand =
    			         (dbgToBeExecutedCommand + COMMAND_TOTAL +
    			          1) % COMMAND_TOTAL;
    		              j++;
    		          }
    		          while (!strlen (dbgCommands[dbgToBeExecutedCommand])
    			           && (j < COMMAND_TOTAL));
    		          strcpy ( _clDebugCliBuf,
                              dbgCommands[dbgToBeExecutedCommand]);
    		          while (index > 0)
    		          {
    		              printf ("%c%c%c", 0x1b, 0x5b, 0x44);
    		              fflush (stdout);
    		              index--;
    		          }
    		          printf ("%s", _clDebugCliBuf);
                      fflush (stdout);
                      maxLenth = index = strlen (_clDebugCliBuf);
    		     }  
                 else if(c == 0x31)
                 {
                    c = getchar();
                    for(l = index ; l > 0 ;l--)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
	                    fflush (stdout);
	                    index--;
                    }
            
                 }
                 else if(c == 0x34)
                 {
                     c = getchar();
                     while(index < maxLenth)
                     {
                         printf ("%c%c%c", 0x1b, 0x5b, 0x43);
        	             fflush (stdout);
        	             index++;
                     }
                 }    
                 else if(c == 0x33)       
                 {
                     c = getchar();
                     for (j = index; j < maxLenth; j++)
                     { 
                         _clDebugCliBuf[j] = _clDebugCliBuf[j + 1];
                     }
                     toBeCopied = maxLenth - index;
                     for (l = index; l < maxLenth; l++)
                     { 
                         if (_clDebugCliBuf[0] == '\0')
                         {
                             printf ("%c", ' ');
                             fflush (stdout);
                         }
                         else
                         {
                             printf ("%c", _clDebugCliBuf[l]);
                             fflush (stdout);
                         }   
                     }    
                     for (l = 0; l < toBeCopied; l++)
                     {
                         printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                         fflush (stdout);
                     }
                 }
	        }
	        else
	        {  
	       /* break for now */
	        break;
	        }
	        break;
      default:
    	    if (index >=(CLI_CMD_LEN -2))
    	    {
	           printf ("%c", 0x7);
    	       fflush (stdout);
    	       break;
    	    }
	        if (index >= maxLenth)
    	    {
	           printf ("%c", c);
    	       fflush (stdout);
    	       _clDebugCliBuf[index++] = c;
    	       maxLenth = index;
    	    }
    	    else
    	    {
    	      /*
    	       * Move one over to the right for all chars on the
               right hand side of index (include index)
    	       */
    	       toBeCopied = maxLenth - index;
    	       for (j = maxLenth; j > index; j--)
    		   {
    		       _clDebugCliBuf[j] = _clDebugCliBuf[j - 1];
    		   }
    	       maxLenth++;
    	       _clDebugCliBuf[index++] = c;

	           for (l = index - 1; l < maxLenth; l++)
    		   {
    		       printf ("%c", _clDebugCliBuf[l]);
    		       fflush (stdout);
    		   }
	           for (l = 0; l < toBeCopied; l++)
    		   {
    		      printf ("%c%c%c", 0x1b, 0x5b, 0x44);
    		      fflush (stdout);
    		   }
    	   }
	      break;
     }
  }
  commandDone = 0;
  if (_clDebugCliBuf[0] == '!')
  {
      sscanf (_clDebugCliBuf + 1, "%d", &his);
      if (his != 0)
      {
    	    if (dbgTotalCommand <= COMMAND_TOTAL)
    	    {
    	        j = 1;
    	    }
    	    else
    	    {
    	        j = dbgTotalCommand - COMMAND_TOTAL;
    	    }
    	    for (i = dbgFirstCommand; j <= dbgTotalCommand;
    	         i = (i + 1) % COMMAND_TOTAL, j++)
    	    {
    	       if ((ClInt32T) j == his)
    		   {
    	       	   strcpy (_clDebugCliBuf, dbgCommands[i]);
    		       printf ("          %s\n", dbgCommands[i]);
    		       jj = strlen (_clDebugCliBuf);
    		       _clDebugCliBuf[jj] = '\n';
    		       _clDebugCliBuf[jj + 1] = '\0';
    		       strcpy (dbgCommands[dbgToBeExecutedCommand - 1],
    	  		  _clDebugCliBuf);
    		       dbgCommands[dbgToBeExecutedCommand - 1][jj] = '\0';
    		       break;
    		   }
    	    }
      }
  }
  if (!strcmp (_clDebugCliBuf, "history\n"))
  {
       if (dbgTotalCommand <= COMMAND_TOTAL)
	   {
    	     j = 1;
	   }
       else
	   {
    	     j = dbgTotalCommand - COMMAND_TOTAL;
	   }
       for (i = dbgFirstCommand; j <= dbgTotalCommand;
	        i = (i + 1) % COMMAND_TOTAL, j++)
	   {
	         printf ("%8d  %s\n", j, dbgCommands[i]);
	   }
   }
  dbgRestoreSettings ();
}

