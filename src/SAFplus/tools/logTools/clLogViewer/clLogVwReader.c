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
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#include "clLogVwReader.h"
#include "clLogVwParser.h"

#include "clLogVwHeader.h"
#include "clLogVwConstants.h"
#include "clLogVwUtil.h"
#include "clLogVwErrors.h"


static ClLogVwMetaDataInfoT *mtdInfo = NULL;


/**************************************************************************/

static ClRcT clLogVwProcessOneRec(ClLogVwRecNumT *recNumToBeShown);

static ClRcT clLogVwSwitchIfEndOfFile(ClLogVwRecNumT *recNumToBeShown);

static ClRcT clLogVwDispPrevRecs(ClLogVwRecNumT *recNum);

static ClRcT clLogVwGetCurRecNum(ClLogVwRecNumT *recNum);

static ClRcT clLogVwReadOneRecord(ClInt32T fd);

static ClRcT clLogVwSeekToCurrentRecord(ClLogVwRecNumT *curRecNum);

static ClRcT clLogVwGetNextFile(void);

static ClRcT clLogVwInitMetaData(ClCharT *fileName, ClInt32T fd);

static ClRcT clLogVwGetCurMaxFilesRotated(void);

static ClRcT clLogVwGetCurMaxRecSize(void);

static ClRcT clLogVwGetCurMaxRecNumInPhyFile(void);

static ClRcT clLogVwGetCurNextRec(ClLogVwRecNumT *nextRec);

static ClRcT clLogVwMapMetaDataFile(void);

static ClRcT clLogVwIsValidCfgFile(ClInt32T mFd);

static ClRcT clLogVwCheckIsValidLogFile(void);

static ClRcT clLogVwCalNumFilesToBeSwitched(ClLogVwRecNumT  numRecVal,
                                            ClUint16T
                                            *numFilesToBeSwitched);

static ClRcT clLogVwReadAllRecords(ClInt32T fd);

/**************************************************************************/

/*
 * This sets the 'dispPrevRecs' flag
 * to 'CL_TRUE' and 'numPrevRecs' variable
 * to the value specified in -p option
 * or to the maximum record num if the
 * value specified is greater than 
 * maximim record number for log files.
 *
 * It sets 'numPrevRecs' to max 
 * value to display entire log in
 * case of -a option
 *
 * @param - numRec
            string specified in -p option
            NULL in case of -a
            
 * @param - setAllFlag
            CL_TRUE if -a flag is specified
            CL_FALSE otherwise

 * @return - rc
             CL_OK if everything is OK
             otherwise error code
 */

ClRcT
clLogVwSetPrevRecVal(ClCharT *numRec, ClBoolT setAllFlag)
{
    ClRcT           rc          =   CL_OK;

    ClLogVwRecNumT  numRecVal   =   0;
    ClInt32T        recNumDiff  =   0;

    ClUint16T       numFilesToBeSwitched    =   0;

    if(CL_FALSE == setAllFlag)
    {
        CL_LOGVW_NULL_CHK(numRec);
    }

    CL_LOGVW_NULL_CHK(mtdInfo);

    mtdInfo->dispPrevRecs = CL_TRUE;

    if(CL_FALSE == setAllFlag)
    {
        numRecVal = atol(numRec);
    }

    if( CL_TRUE == setAllFlag   ||
            numRecVal >= (mtdInfo->maxRecNum - mtdInfo->maxFilesRotated))
    {

        if(CL_TRUE == setAllFlag)
        {
            fprintf(stdout, "\nDisplaying entire log.. \nMaximum number of records "
                    "which may be present in this log file : ");
        }
        else
        {

            fprintf(stdout, "\nDisplaying entire log.. "
                    "\nThe specified value in -p flag "
                    " is greater than max record number for this file :  ");
        }

        if( 0 == mtdInfo->maxFilesRotated )
        {
            fprintf(stdout, "%d ", 
                    (mtdInfo->maxRecNum - mtdInfo->maxFilesRotated - 1) );

            numRecVal = mtdInfo->maxRecNum - mtdInfo->maxFilesRotated - 1;
        }
        else
        {
            fprintf(stdout, "%d ", 
                    (mtdInfo->maxRecNum - mtdInfo->maxFilesRotated) );

            numRecVal = mtdInfo->maxRecNum - mtdInfo->maxFilesRotated;
        }

        fprintf(stdout, "\n----------------------------------------------------------------------\n\n");
    }

    /*
     * Number of records to be displaed
     */
    mtdInfo->numPrevRecs = numRecVal;

    rc = clLogVwCalNumFilesToBeSwitched(numRecVal, 
            &numFilesToBeSwitched);

    if(CL_OK != rc)
    {
        return rc;
    }

    numRecVal += numFilesToBeSwitched;

    recNumDiff = mtdInfo->nextRecNum - numRecVal;

    if( recNumDiff <= 0 )
    {
        mtdInfo->nextRecNum = mtdInfo->maxRecNum + recNumDiff;
        return CL_OK;
    }

    mtdInfo->nextRecNum -= numRecVal ;

    return CL_OK;
}

/* Calculate number of
 * files to be switched when displaying
 * 'numRecVal' previous records
 
 *@param -  numRecVal
            value provided in -p option
            
 *@param -  numFilesToBeSwitched
            contains number of files
            from which records will 
            be displyed
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwCalNumFilesToBeSwitched( ClLogVwRecNumT  numRecVal,
                                ClUint16T   *numFilesToBeSwitched)
{
    ClLogVwRecNumT  curRecNum   =   0;
    ClRcT           rc          =   CL_OK;

    rc = clLogVwGetCurRecNum(&curRecNum);

    if( CL_OK != rc )
    {
        fprintf(stdout, "\n\nERROR : Unable to get the current record.\n"); 
        return rc;
    }

    if(numRecVal < curRecNum )
    {
        *numFilesToBeSwitched = 0;
        return CL_OK;
    }

    numRecVal -= curRecNum;

    *numFilesToBeSwitched = (numRecVal / (mtdInfo->numRecInPhyFile - 1 )) + 1;

    return CL_OK;
}


/*
 * Displays the 'numPrevRecs' previous records
 * as set in mtdInfo.

 *@param -  recNum
            It is the 1st record number to be shown
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwDispPrevRecs(ClLogVwRecNumT *recNum)
{
    ClInt32T    counter     =   0, 
                recCount    =   0;

    ClRcT       rc          =   CL_OK;
    ClUint16T   numErrRecs  =   0;

    ClLogVwRecNumT    recNumToBeShown   =   0;
                      
    CL_LOGVW_NULL_CHK(mtdInfo);

    recCount = mtdInfo->numPrevRecs; 

    recNumToBeShown = *recNum;
    counter = 0;

    while(counter < recCount)
    {
        rc = clLogVwProcessOneRec(&recNumToBeShown);

        if( CL_OK != rc )
        {
            numErrRecs++;

            if( numErrRecs > 2)
            {
                fprintf(stdout, "\n\n[Too many errors "
                        "while processing records.]\n\n");
                return rc;
            }
        }
        else
        {
            numErrRecs = 0;
        }

        rc = clLogVwSwitchIfEndOfFile(&recNumToBeShown);

        if( CL_OK != rc)
        {
            return rc;
        }

        counter++; 
    }

    if(numErrRecs > 0 || 0 == counter)
    {
        fprintf(stdout, "\n\n-------------------------------------------------------------------------");
        fprintf(stdout, "\ninfo :\tIf log viewer doesn't show any record, then please check if the\n"
                "\tlog file contains ASCII type records "
                "because log viewer supports\n\tonly Binary records (Buffer and TLV types).");

        fprintf(stdout, "\n-------------------------------------------------------------------------");
    }

    return CL_OK;
}


/*
 * It seeks the file pointer to the
 * current record and then starts
 * reading the log records

 * It polls on nextRec of metadata
 * to get next record number to be written
 * and wait for the reord to be written
 
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
            
 * It returns when there are too many errors
 * while reading the records.
 */

ClRcT
clLogVwReadRecords(void)
{
    ClLogVwRecNumT  recNum      =   0,  
                    prevRecNum  =   0;

    ClLogVwRecNumT  recNumToBeShown =   0;

    ClUint32T       counter     =   0, 
                    recCount    =   0;

    ClUint16T       numErrRecs  =   0;

    ClRcT           rc          =   CL_OK;

    ClInt32T        recDiff     =   0;

    CL_LOGVW_NULL_CHK(mtdInfo);
    CL_LOGVW_NULL_CHK(mtdInfo->phyFileName);

    mtdInfo->recBuf = (ClLogVwByteT*)  calloc(1, mtdInfo->maxRecSize + 10);

    CL_LOGVW_NO_MEMORY_CHK(mtdInfo->recBuf);

    rc = clLogVwSeekToCurrentRecord(&recNum);

    if( CL_OK != rc)
    {
        fprintf(stdout, "Unable to seek to current record.\n");
        CL_LOGVW_PRINT_ASCII_REC_ERROR();
        return rc;
    }

    if(CL_TRUE == mtdInfo->dispPrevRecs)
    {
        rc = clLogVwDispPrevRecs(&recNum);

        if( CL_OK != rc)
        {
            return rc;
        }

        return CL_OK;
    }

    fprintf(stdout, "\nDisplaying log records (waiting for the log records to " 
            "be written into the log file)..\n\n");

    for(;;)
    {

        fflush(stdout);

        prevRecNum = recNum;

        rc = clLogVwGetCurRecNum(&recNum);

        if( CL_OK != rc )
        {
            fprintf(stdout, "\n\nERROR : Unable to get the current record.\n"); 
            return rc;
        }

        while(recNum == prevRecNum)
        {
            usleep(100); 

            rc = clLogVwGetCurRecNum(&recNum);

            if( CL_OK != rc )
            {
                fprintf(stdout, "\n\nERROR : Unable to get the current record.\n"); 
                return rc;
            }
        }

        recDiff = recNum - prevRecNum;

        if( recDiff < 0 )
        {
            /*Subtracting 1 for clovis header*/
            recCount = mtdInfo->numRecInPhyFile - prevRecNum + recNum - 1;
        }
        else
        {
            recCount = recNum - prevRecNum;
        }

        rc = clLogVwSwitchIfEndOfFile(&recNumToBeShown);

        if( CL_OK != rc)
        {
            return rc;
        }

        recNumToBeShown = prevRecNum;
        counter = 0;

        while(counter < recCount)
        {
            rc = clLogVwSwitchIfEndOfFile(&recNumToBeShown);

            if( CL_OK != rc)
            {
                return rc;
            }

            rc = clLogVwProcessOneRec(&recNumToBeShown);

            if( CL_OK != rc )
            {
                numErrRecs++;

                if( numErrRecs > 2)
                {
                    fprintf(stdout, "\n\nToo many errors "
                            "while processing records.\n\n");
                    return rc;
                }
            }
            else
            {
                numErrRecs = 0;
            }

            counter++; 
        }

    }

    return CL_OK;
}


/*
 * Processes a single record.
 * 
 * It calls the parser to parse that record.

 * It switches to the next file when it 
 * reaches the end of current file.

 *@param -  recNumToBeShown
            record number in the current file

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */

ClRcT
clLogVwProcessOneRec(ClLogVwRecNumT *recNumToBeShown)
{
    ClRcT           rc         =   CL_OK;

    (*recNumToBeShown)++;

    if(CL_TRUE == mtdInfo->dispPrevRecs
            && -1 == mtdInfo->pFd)
    {
        return CL_OK;
    }

    rc = clLogVwReadOneRecord(mtdInfo->pFd);

    if( CL_OK != rc)
    {
        if(CL_TRUE == mtdInfo->dispPrevRecs)
            return CL_OK;

        return rc;
    }

    rc = clLogVwParseOneRecord(mtdInfo->recBuf);  /* Parse the record */

    if( CL_OK != rc)
    {
        return rc;
    }
    
    return CL_OK;
}


/*
 * Switch to the next file when reading 
 * reaches the end of current file.

 *@param -  recNumToBeShown
            record number in the current file

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSwitchIfEndOfFile(ClLogVwRecNumT *recNumToBeShown)
{
    ClRcT   rc  =   CL_OK;

    if( *recNumToBeShown >= mtdInfo->numRecInPhyFile)
    {
        rc = clLogVwGetNextFile();
        
        if( CL_OK != rc )
        {
            fprintf(stdout, "Unable to get next physical file:%s\n", 
                    mtdInfo->phyFileName);

            return rc;
        }

        if( -1 != mtdInfo->pFd)
        {
            if( -1 == close(mtdInfo->pFd) )
            {
                fprintf(stderr, "\n\nUnable to close the file.\n\n");
            }
        }

        if( -1 == (mtdInfo->pFd = open(mtdInfo->phyFileName, O_RDONLY)))
        {
            /* Temp fix for -a or -p option
             * when some of the files may not
             * actually exists
             */
            if(CL_TRUE == mtdInfo->dispPrevRecs)
            {
                mtdInfo->pFd = -1;
                *recNumToBeShown = 1;/* Reset record number */
                return CL_OK;
            }

            fprintf(stdout, "Unable to open physical file:%s\n", 
                    mtdInfo->phyFileName);

            CL_LOGVW_PRINT_ASCII_REC_ERROR();
            return CL_LOGVW_ERROR;
        }

        rc = clLogVwCheckIsValidLogFile();
        
        if( CL_OK != rc )
        {
            fprintf(stdout, "Log file '%s' format not recognized.\n", 
                    mtdInfo->phyFileName);

            return rc;
        }
        
        //fprintf(stdout, "\n\n Switched to file :%s\n\n", mtdInfo->phyFileName);
        
        *recNumToBeShown = 1;/* Reset record number */

    }

    return rc; 

}


/*
 * Checks whether the file is a valid log file.
 * 

 *@return - rc
            CL_OK if file is valid log file
            otherwise error code
 */
ClRcT
clLogVwCheckIsValidLogFile(void)
{

    if( CL_LOGVW_LOG_FILE_MARKER_LEN != 
            read(mtdInfo->pFd, mtdInfo->recBuf, CL_LOGVW_LOG_FILE_MARKER_LEN))
    {
        return CL_LOGVW_ERROR;
    }
        
    
   if(0 != strncmp(mtdInfo->recBuf, CL_LOG_DEFAULT_FILE_STRING, CL_LOGVW_LOG_FILE_MARKER_LEN)) 
   {
       return CL_LOGVW_ERROR;
   }

   
    if( lseek(mtdInfo->pFd, mtdInfo->maxRecSize, SEEK_SET) !=
            mtdInfo->maxRecSize)
    {
        fprintf(stdout, "Unable to seek to the 1st record.\n");
        return CL_LOGVW_ERROR;
    }

   return CL_OK;
}


/*
 * Gets the record number of the current log file
 * where nextRec of metadata is pointing
 
 *@param -  recNum
            contains the record number after the return
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */

static ClRcT
clLogVwGetCurRecNum(ClLogVwRecNumT *recNum)
{
    ClLogVwRecNumT  nextRecNum      =   0;

    ClRcT           rc         =   CL_OK;

    if(CL_TRUE == 
            clLogVwIsSameEndianess(mtdInfo->endianess))
    {
        *recNum = mtdInfo->mappedMtd->nextRec % mtdInfo->numRecInPhyFile ;
    }
    else
    {
        rc = clLogVwGetCurNextRec(&nextRecNum);

        if(CL_OK != rc)
        {
            return rc;
        }

        *recNum = (nextRecNum % mtdInfo->numRecInPhyFile) ;
    }

    if( 0 == *recNum)
    {
        *recNum = 1;
    }

    return CL_OK;
}



/*
 * Reads the record from file to record buffer  

 *@param -  fd
            file descriptor of current log file
 
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwReadOneRecord(ClInt32T fd)
{
    fflush(stdout);

    if( mtdInfo->maxRecSize == 
            read(fd, mtdInfo->recBuf, mtdInfo->maxRecSize))
    {
        return CL_OK;
    }

    if(CL_TRUE == mtdInfo->dispPrevRecs)
            return CL_LOGVW_ERROR;

    fprintf(stdout, "\n[Error while reading record!!] \n");
    CL_LOGVW_PRINT_ASCII_REC_ERROR();

    return CL_LOGVW_ERROR;
}


ClRcT
clCheckCfgFile(ClCharT * cfgFileName)
{
    ClUint64T   timeOut         = 0;
    ClUint32T   timeOutPeriod   = 15000; /* Almost 20 Seconds */

    if(NULL == cfgFileName) return CL_ERR_NULL_POINTER;

    if( 0 != access(cfgFileName, F_OK) )
    {
        fprintf(stdout, "config file '%s' not present.", 
                cfgFileName);
        fprintf(stdout, "\nWaiting [60 Secs] for the file to be created by log "
                "server.");

        fflush(stdout);
    }

    do{
        usleep(1000);
        timeOut++;

        if(0 == (timeOut % 1000))
        {
            fprintf(stdout, ".");
            fflush(stdout);
        }

    }while( (0 != access(cfgFileName, F_OK)) && timeOut < timeOutPeriod);

    return CL_OK;
}


/*
 * Reads the log configuration file and initializes
 * the metadata information for global variable
 
 *@param -  logicalFileName 
            common prefix of the log files
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwReadMetaData(ClCharT *logicalFileName) 
{
    ClUint32T   size = strlen(logicalFileName) + 10;
    ClCharT     *cfgFileName    = (ClCharT*) calloc(1, size); 
    ClInt32T    fd =        -1;
    ClRcT       rc =   CL_OK;
    
    CL_LOGVW_NO_MEMORY_CHK(cfgFileName);
    
    strncpy(cfgFileName, logicalFileName, size-1);
    strncat(cfgFileName, ".cfg", (size-strlen(cfgFileName)-1));

    rc = clCheckCfgFile(cfgFileName);

    if( CL_OK != rc )
    {
        return rc;
    }

    if( -1 == (fd = open(cfgFileName, O_RDONLY)))
    {
        fprintf(stdout, "\n\nTimeout occured while opening file '%s'.\n",
                cfgFileName);
        perror(""); 
        free(cfgFileName);
        return CL_LOGVW_ERROR;
    }

    rc = clLogVwIsValidCfgFile(fd);

    if( CL_OK != rc )
    {
        fprintf(stdout, "Config file %s's format not supported.\n",
                cfgFileName);
        free(cfgFileName);
        return rc;
    }
    
    free(cfgFileName);

    rc = clLogVwInitMetaData(logicalFileName, fd);
    
    if( CL_OK != rc )
    {
        fprintf(stdout, "Error in initializing meta data.\n");
        return rc;
    }
    
    return CL_OK;
}


/*
 * Checks whether the file is a valid log config file.
 * 

 *@param -  mFd
            file descriptor to metadata file

 *@return - rc
            CL_OK if file is valid log config file
            otherwise error code
 */
ClRcT
clLogVwIsValidCfgFile(ClInt32T mFd)
{

    ClCharT *cfgBuf = (ClCharT*) calloc(1, CL_LOGVW_MTD_CFG_MARKER_LEN + 5);

    CL_LOGVW_NO_MEMORY_CHK(cfgBuf);

    if( lseek(mFd, CL_LOGVW_MTD_CFG_MARKER_INDEX, SEEK_SET) !=
            CL_LOGVW_MTD_CFG_MARKER_INDEX)
    {
        fprintf(stdout, "Unable to do lseek on metadata file.\n");

        free(cfgBuf);
        return CL_LOGVW_ERROR;
    }


    if( CL_LOGVW_MTD_CFG_MARKER_LEN !=
            read(mFd, cfgBuf, CL_LOGVW_MTD_CFG_MARKER_LEN))
    {
        free(cfgBuf);
        return CL_LOGVW_ERROR;
    }

    if(0 != strncmp(cfgBuf, CL_LOG_DEFAULT_CFG_FILE_STRING, CL_LOGVW_MTD_CFG_MARKER_LEN)) 
    {
        free(cfgBuf);
        return CL_LOGVW_ERROR;
    }

    free(cfgBuf);
    return CL_OK;
}



/*
 * Based on current mtdInfo->nextRecNum
 * it calculates the file number
 * and the record number 'curRecNum' from where
 * reading of log records is supposed to start
 * It seeks the file descriptor 
 * to the record number pointed by 'curRecNum'
 * and sets the current record number to 'curRecNum'
 
 *@param -  curRecNum 
            current record number
            where fd is seeked 
            in current log file
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSeekToCurrentRecord(ClLogVwRecNumT *curRecNum)
{
    ClLogVwRecNumT  nextRecNum  =   0;

    ClInt64T        curByte     =   0;

    ClRcT           rc          =   CL_OK;

    memset(mtdInfo->phyFileName, '\0', strlen(mtdInfo->phyFileName));
    strcpy(mtdInfo->phyFileName, mtdInfo->logicalFileName);
    strncat(mtdInfo->phyFileName, ".",1);

    if( 0 == mtdInfo->maxFilesRotated )
    {
        /*Set the current file number*/
        mtdInfo->curFileNum = 0;   
        strcat(mtdInfo->phyFileName, "0");
    }
    else
    {
        nextRecNum = mtdInfo->nextRecNum;    
        nextRecNum %= mtdInfo->maxRecNum;

        /*Set the current file number*/
        mtdInfo->curFileNum = nextRecNum / mtdInfo->numRecInPhyFile;

        if( mtdInfo->curFileNum >= mtdInfo->maxFilesRotated )
        {
            fprintf(stdout, "Incorrect Metadata info..Error in calculating"
                    " current file number.\n");
            return CL_LOGVW_ERROR;
        }
        ClCharT *fileNumString = (ClCharT*) calloc(1, 50);

        CL_LOGVW_NO_MEMORY_CHK(fileNumString);

        sprintf(fileNumString, "%hd", mtdInfo->curFileNum);
        strcat(mtdInfo->phyFileName, fileNumString);

        free(fileNumString);
    }
#if 0
    if( -1 != mtdInfo->pFd)
    {
        if( -1 == close(mtdInfo->pFd) )
        {
            fprintf(stderr, "\n\nUnable to close the file.\n\n");
        }
    }
#endif
    if( -1 == (mtdInfo->pFd = open(mtdInfo->phyFileName, O_RDONLY)))
    {
        /* Temp fix for -a or -p option
         * when some of the files may not
         * actually exists
         */
        if(CL_TRUE == mtdInfo->dispPrevRecs)
        {
            mtdInfo->pFd = -1;
            
            *curRecNum = mtdInfo->nextRecNum % mtdInfo->numRecInPhyFile;

            if(0 == *curRecNum)
            {
                *curRecNum = 1;
            }
            return CL_OK;
        }
        
        fprintf(stdout, "Unable to open physical file:%s\n", 
                mtdInfo->phyFileName);

        return CL_LOGVW_ERROR;
    }

    rc = clLogVwCheckIsValidLogFile();

    if( CL_OK != rc )
    {
        fprintf(stdout, "Log file '%s' format not recognized.\n", 
                mtdInfo->phyFileName);

        return rc;
    }


    *curRecNum = mtdInfo->nextRecNum % mtdInfo->numRecInPhyFile;

    if(0 == *curRecNum)
    {
        *curRecNum = 1;
    }

    curByte = (*curRecNum) * mtdInfo->maxRecSize; 

    if( lseek(mtdInfo->pFd, curByte, SEEK_SET) != curByte)
    {
        fprintf(stdout, "Unable to do lseek on file:%s\n", 
                mtdInfo->phyFileName);

        return CL_LOGVW_ERROR;
    }

    return CL_OK;

}


/*
 * It finds and sets 'curFileNum' to the next file number 
 * where the log records are written
 * it also changes the name of physical file to the 
 * current file

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwGetNextFile(void)
{
    ClRcT   rc  =   CL_OK;
    
    memset(mtdInfo->phyFileName, '\0', strlen(mtdInfo->phyFileName));
    strcpy(mtdInfo->phyFileName, mtdInfo->logicalFileName);

    if( 0 == mtdInfo->maxFilesRotated)
    {
        strcat(mtdInfo->phyFileName, ".0");
        mtdInfo->curFileNum = 0;
    }
    else
    {
        if( mtdInfo->curFileNum < ( mtdInfo->maxFilesRotated - 1 ) )
        {
            ClCharT *fileNumString = (ClCharT*) calloc(1, 50);

            CL_LOGVW_NO_MEMORY_CHK(fileNumString);

            strcat(mtdInfo->phyFileName, ".");

            (mtdInfo->curFileNum)++;

            sprintf(fileNumString, "%hd", mtdInfo->curFileNum);
            strcat(mtdInfo->phyFileName, fileNumString);

            free(fileNumString);
        }
        else
        {/* Reset curFileNum */
            strcat(mtdInfo->phyFileName, ".0");
            mtdInfo->curFileNum = 0;
        }
    }

    return rc;
    
}


/*
 * Initializes the metadata information
 * after reading it from mapped memory
 
 *@param -  fileName
            common prefix to log files
 
 *@param -  fd
            file descriptor to metadata
            file
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwInitMetaData(ClCharT *fileName, ClInt32T fd)
{
    ClRcT rc = CL_OK;

    mtdInfo = (ClLogVwMetaDataInfoT*) malloc(sizeof(ClLogVwMetaDataInfoT));

    CL_LOGVW_NO_MEMORY_CHK(mtdInfo);

    mtdInfo->logicalFileName = NULL;
    mtdInfo->phyFileName = NULL;
    mtdInfo->buf = NULL;
    mtdInfo->mFd = fd;

    rc = clLogVwMapMetaDataFile();
    
    if( CL_OK != rc)
    {
        fprintf(stdout, "Error in mapping meta data file.");
        return CL_LOGVW_ERROR;
    }


    mtdInfo->pFd = -1;

    mtdInfo->logicalFileName = (ClCharT*)
        calloc(1, strlen(fileName) + 1);

    CL_LOGVW_NO_MEMORY_CHK(mtdInfo->logicalFileName);

    mtdInfo->buf = (ClLogVwByteT*) calloc(1, 10);

    CL_LOGVW_NO_MEMORY_CHK(mtdInfo->buf);

    strcpy(mtdInfo->logicalFileName, fileName);

    lseek(mtdInfo->mFd, CL_LOGVW_MTD_ENDIAN_INDEX, SEEK_SET);
    if( sizeof(ClLogVwFlagT) != read(mtdInfo->mFd, mtdInfo->buf, sizeof(ClLogVwFlagT)))
    {
        return CL_LOGVW_ERROR;
    }
    memcpy(&mtdInfo->endianess, mtdInfo->buf, sizeof(ClLogVwFlagT));

    rc = clLogVwGetCurMaxFilesRotated();

    if(CL_OK != rc)
    {
        return rc;
    }

    rc = clLogVwGetCurMaxRecSize(); 
    
    if(CL_OK != rc)
    {
        return rc;
    }

    if(0 == mtdInfo->maxRecSize)
    {
        fprintf(stdout, "Found invalid data for 'maxRecSize' "
                "in meta data file.\n");
        return CL_LOGVW_ERROR;
    }

    rc = clLogVwSetParserRecSize(mtdInfo->maxRecSize);

    if(CL_OK != rc)
    {
        return rc;
    }

    rc = clLogVwGetCurMaxRecNumInPhyFile();
    
    if(CL_OK != rc)
    {
        return rc;
    }

    /*numRecInPhyFile is  (fileUnitSize / maxRecSize) */
    mtdInfo->numRecInPhyFile /= mtdInfo->maxRecSize;

    if( 0 == mtdInfo->numRecInPhyFile ||
            1 == mtdInfo->numRecInPhyFile)
    {
        fprintf(stdout, "Found invalid data '%d' for 'numRecInPhyFile' "
                "in meta data file.\n", mtdInfo->numRecInPhyFile);
        return CL_LOGVW_ERROR;
    }

    if( 0 == mtdInfo->maxFilesRotated )
    {
         mtdInfo->maxRecNum = mtdInfo->numRecInPhyFile;
    }
    else
    {
         mtdInfo->maxRecNum = mtdInfo->numRecInPhyFile * mtdInfo->maxFilesRotated;
    }

    rc = clLogVwGetCurNextRec(&mtdInfo->nextRecNum);

    if(CL_OK != rc)
    {
        return rc;
    }

    mtdInfo->phyFileName = (ClCharT*) calloc(1, strlen(fileName) + 50);

    CL_LOGVW_NO_MEMORY_CHK(mtdInfo->phyFileName);

    strncpy(mtdInfo->phyFileName, fileName,strlen(fileName));
    strncat(mtdInfo->phyFileName, ".0",2);

    mtdInfo->headerSize = sizeof(ClLogVwFlagT) + sizeof(ClLogVwSeverityT) +
        sizeof(ClLogVwStreamIdT) + sizeof(ClLogVwCmpIdT) + sizeof(ClLogVwServiceIdT) +
        sizeof(ClLogVwTimeStampT) + sizeof(ClLogVwMsgIdT);


    rc = clLogVwSetParserHeaderSize(mtdInfo->headerSize);

    if(CL_OK != rc)
    {
        return rc;
    }
    
    return CL_OK;

}


/*
 * Sets the 'maxFilesRotated' element of metadata 
 * to the maximum number of log files

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwGetCurMaxFilesRotated(void)
{
    ClRcT   rc =   CL_OK;

    lseek(mtdInfo->mFd, CL_LOGVW_MTD_MAX_FILES_ROTATED_INDEX, SEEK_SET);
    if( sizeof(ClLogVwMaxFileRotatedT) != read(mtdInfo->mFd, mtdInfo->buf,
                sizeof(ClLogVwMaxFileRotatedT)))
    {
        return CL_LOGVW_ERROR;
    }
    memcpy(&mtdInfo->maxFilesRotated, mtdInfo->buf,
            sizeof(ClLogVwMaxFileRotatedT));
    
    if(CL_FALSE == 
            clLogVwIsSameEndianess(mtdInfo->endianess))
    {
        rc = clLogVwGetRevSubBytes(&mtdInfo->maxFilesRotated, 
                                   0, 
                                   sizeof(mtdInfo->maxFilesRotated));

        if(CL_OK != rc)
        {
            return rc;
        }
    }

    return CL_OK;
}



/*
 * Sets the 'maxRecSize' element of metadata 
 * to the maximum size of a record in bytes

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwGetCurMaxRecSize(void)
{

    ClRcT   rc =   CL_OK;

    lseek(mtdInfo->mFd, CL_LOGVW_MTD_MAX_REC_SIZE_INDEX, SEEK_SET);
    if( sizeof(ClLogVwMaxRecSizeT) != read(mtdInfo->mFd, mtdInfo->buf,
                sizeof(ClLogVwMaxRecSizeT)))
    {
        return CL_LOGVW_ERROR;
    }
    memcpy(&mtdInfo->maxRecSize, mtdInfo->buf,
            sizeof(ClLogVwMaxRecSizeT));
        
    if(CL_FALSE == 
            clLogVwIsSameEndianess(mtdInfo->endianess))
    {
        rc = clLogVwGetRevSubBytes(&mtdInfo->maxRecSize, 
                                   0,
                                   sizeof(mtdInfo->maxRecSize));
        
        if(CL_OK != rc)
        {
            return rc;
        }
    }
    return CL_OK;
}


/*
 * Sets the 'maxRecNum' element of metadata 
 * to the total number of records in all 
 * log files

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */

static ClRcT
clLogVwGetCurMaxRecNumInPhyFile(void)
{

    ClRcT   rc =   CL_OK;

    lseek(mtdInfo->mFd, CL_LOGVW_MTD_FILE_UNIT_SIZE_INDEX, SEEK_SET);
    
    if( sizeof(ClLogVwRecNumT) != read(mtdInfo->mFd, mtdInfo->buf,
                sizeof(ClLogVwRecNumT)))
    {
        return CL_LOGVW_ERROR;
    }
    
    memcpy(&mtdInfo->numRecInPhyFile, mtdInfo->buf,
            sizeof(ClLogVwRecNumT));
    
    if(CL_FALSE == 
            clLogVwIsSameEndianess(mtdInfo->endianess))
    {
       rc = clLogVwGetRevSubBytes(&mtdInfo->numRecInPhyFile, 
                                  0,
                                  sizeof(mtdInfo->numRecInPhyFile));
         if(CL_OK != rc)
         {
             return rc;
         }
    }
    return CL_OK;
}


/*
 * Sets the 'nextRecNum' element of metadata 
 * to the next record number to be written

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwGetCurNextRec(ClLogVwRecNumT *nextRec)
{

    ClRcT   rc =   CL_OK;

    *nextRec = mtdInfo->mappedMtd->nextRec;
    
    if(CL_FALSE == 
            clLogVwIsSameEndianess(mtdInfo->endianess))
    {
        rc = clLogVwGetRevSubBytes(nextRec, 
                                   0,
                                   sizeof(ClLogVwRecNumT));
        
         if(CL_OK != rc)
         {
             return rc;
         }
         
    }
    return CL_OK;
}

/*
 * Gets the record size in bytes

 *@param -  recSize
            contains the record size after return
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwGetRecSize(ClLogVwMaxRecSizeT *recSize)
{
    if(NULL == mtdInfo)
        return CL_LOGVW_ERROR;

    *recSize = mtdInfo->maxRecSize;
    return CL_OK;
}


/*
 * Gets the header size in bytes

 *@param -  headerSize
            cpntains the header size after return
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwGetHeaderSize(ClUint16T *headerSize)
{
    *headerSize =  mtdInfo->headerSize;
    return CL_OK;
}


/*
 * Unmaps the metadata file
 * closes the file descriptor
 * for metadata and log file

 * Frees up the allocated memory

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */

ClRcT
clLogVwCleanUpMetaData(void)
{
    if( NULL == mtdInfo)
        return CL_OK;

    if(NULL != mtdInfo->mappedMtd)
    {
        if(-1 == munmap(mtdInfo->mappedMtd, 
                    sizeof(ClLogFileHeaderT)) )
        {
            //fprintf(stdout, "Unmapping of metadata file failed.\n");
        }
    }

    if( -1 == close(mtdInfo->mFd) )
    {
        //fprintf(stderr, "\n\nUnable to close the  meta data file.\n\n");
    }
    if( -1 == close(mtdInfo->pFd) )
    {
        //fprintf(stderr, "\n\nUnable to close the  physical file.\n\n");
    }
    free(mtdInfo->logicalFileName);
    free(mtdInfo->phyFileName);
    free(mtdInfo->buf);
    free(mtdInfo->recBuf);
    free(mtdInfo);

    return CL_OK;
}



/*
 * Maps the 1st sizeof(ClLogFileHeaderT) bytes
 * of metadata file to memory

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwMapMetaDataFile(void)
{

    mtdInfo->mappedMtd = NULL;
    
    if( (mtdInfo->mappedMtd = (ClLogFileHeaderT*) 
                               mmap(0, 
                                    sizeof(ClLogFileHeaderT), 
                                    PROT_READ, 
                                    MAP_SHARED, 
                                    mtdInfo->mFd, 0)) == MAP_FAILED )
    {
        return CL_LOGVW_ERROR;
    }

    return CL_OK;
    
}


/*
 * Converts binary log file to text file
 * text file name will be "<binary_file_name>.txt" 

 *@param -  fileName
            name of binary log file
 
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
*/
ClRcT 
clLogVwConvertBinFileToTxt(const char *fileName)
{
    ClInt32T    fd  =   -1;
    FILE        *fp =   NULL;

    ClRcT       rc  =   CL_OK;
    ClCharT     *txtFileName = NULL;
    ClUint32T   size = 0;

    if( -1 == (fd = open(fileName, O_RDONLY)))
    {
        fprintf(stdout, "Unable to open binary file:%s\n", 
                fileName);

        return CL_LOGVW_ERROR;
    }

    if(NULL == mtdInfo)
    {
        fprintf(stdout, "Metadata not initialized.\n");
        return CL_LOGVW_ERROR;
    }

    mtdInfo->pFd = fd;

    mtdInfo->recBuf = (ClLogVwByteT*)  calloc(1, mtdInfo->maxRecSize + 10);

    CL_LOGVW_NO_MEMORY_CHK(mtdInfo->recBuf);

    rc = clLogVwCheckIsValidLogFile();

    if( CL_OK != rc )
    {
        fprintf(stdout, "Log file '%s' format not recognized.\n", 
                fileName);
        return rc;
    }

    size = strlen(fileName) + strlen(CL_LOGVW_TXT_FILE_EXT) + 2;
    txtFileName = (ClCharT*) calloc(1,size); 

    CL_LOGVW_NO_MEMORY_CHK(txtFileName);

    strncpy(txtFileName, fileName, size-1);
    strncat(txtFileName, CL_LOGVW_TXT_FILE_EXT, (size - strlen(txtFileName)-1));

    if(NULL == (fp = fopen(txtFileName, "w")) )
    {
        fprintf(stdout, "Unable to open text file for writing:%s\n", 
                txtFileName);
    
        free(txtFileName);
        return CL_LOGVW_ERROR;
    }

    fprintf(stdout, "\n[Converting Log file '%s' to text file '%s'...]\n", 
            fileName, txtFileName);

    free(txtFileName);

    mtdInfo->txtFp = fp;

    rc = clLogVwSetHeaderOutPtr(mtdInfo->txtFp);
    if(CL_OK != rc)
    {
        return rc;
    }

    rc = clLogVwSetParserOutPtr(mtdInfo->txtFp);
    if(CL_OK != rc)
    {
        return rc;
    }

    
    rc = clLogVwReadAllRecords(mtdInfo->pFd);
    if(CL_OK != rc)
    {
        return rc;
    }
    return CL_OK;
}


/*
 * Reads all the record from file and sends it
 * to parser

 *@param -  fd
            file descriptor of current log file
 
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwReadAllRecords(ClInt32T fd)
{

    ClRcT       rc          =   CL_OK;

    ClUint32T   numErrors   =   0; 

    while( mtdInfo->maxRecSize == 
            read(fd, mtdInfo->recBuf, mtdInfo->maxRecSize))
    {
        rc = clLogVwParseOneRecord(mtdInfo->recBuf);  /* Parse the record */
        if(CL_OK != rc)
        {
            numErrors++;
        }
        if(numErrors > 10)
        {
            fprintf(stdout, "\nError while converting binary file "
                    "to text file.\n");
            return CL_LOGVW_ERROR;
        }
    }


    fprintf(stdout, "\nDone.\n\n");

    return CL_OK;
}
