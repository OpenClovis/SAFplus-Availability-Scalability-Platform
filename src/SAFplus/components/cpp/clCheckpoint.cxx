/*
 * Copyright (C) 2002-2008 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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
#include <clDebugApi.h>
#include <stdio.h>
#include <unistd.h>

#define clprintf printf

#include "clCheckpoint.hxx"
#include "clException.hxx"

namespace clCheckpoint
{
  using namespace SAFplus;
  
  SaCkptHandleT svcHdl = 0;
  ClOsalMutexT  svcLock;

  Table::Table()
  {
    handleTbl            = 0;
    sectionExpiration = 0;    
    autoActivate      = 0;
    tryAgainDelayMs   = 0;
    maxSections       = 0;
    flags             = 0;
  }

  Table::Table(const char* name, SaCkptCheckpointCreationFlagsT flagsp, unsigned int retentionMs, unsigned int maxSections, unsigned int maxSectionSizeBytes, unsigned int maxSectionIdSizeBytes)
    {
      Initialize(name,flagsp, retentionMs,maxSections, maxSectionSizeBytes,maxSectionIdSizeBytes);
    }

  void Table::Initialize(const char* name, SaCkptCheckpointCreationFlagsT flagsp, unsigned int retentionMs, unsigned int maxSectionsp, unsigned int maxSectionSizeBytes, unsigned int maxSectionIdSizeBytes)
    {
      SaAisErrorT rc = SA_AIS_OK;
      SaCkptCheckpointCreationAttributesT create_atts;
      maxSections = maxSectionsp;
      create_atts.creationFlags     = flagsp;
      create_atts.checkpointSize    = maxSections * maxSectionSizeBytes;
      create_atts.retentionDuration = ((SaTimeT)retentionMs) * 1000LL * 1000LL;
      create_atts.maxSections       = maxSections;
      create_atts.maxSectionSize    = maxSectionSizeBytes;
      create_atts.maxSectionIdSize  = maxSectionIdSizeBytes;

      SaNameT    ckpt_name;
      ckpt_name.length = strlen(name);
      strncpy ((char*) ckpt_name.value,name,SA_MAX_NAME_LENGTH);
      ckpt_name.value[SA_MAX_NAME_LENGTH-1] = 0; /* in case the strncpy did not NULL term */


      /* By default sections will never expire */
      sectionExpiration = SA_TIME_END;
      handleTbl            = 0;
      autoActivate      = true;
      tryAgainDelayMs   = 250;  // 1/4 of a second
      flags             = flagsp;

      clLogInfo("CPP","CKP","Opening checkpoint %s attributes: flags: %d, size: %d, retention %llu, maxSections: %d, maxSectionSize: %d, maxSectionIdSize: %d", ckpt_name.value, (int) create_atts.creationFlags, (int) create_atts.checkpointSize, create_atts.retentionDuration, (int) create_atts.maxSections, (int) create_atts.maxSectionSize, (int) create_atts.maxSectionIdSize);

      rc = saCkptCheckpointOpen(svcHdl,      // Service handle
                                &ckpt_name,         // Checkpoint name
                                &create_atts,       // Optional creation attr.
                                (SA_CKPT_CHECKPOINT_WRITE |
                                 SA_CKPT_CHECKPOINT_READ | 
                                 SA_CKPT_CHECKPOINT_CREATE),
                                (SaTimeT)-1,        // No timeout
                                &handleTbl);      // Checkpoint handle

      if (rc != SA_AIS_OK)
        {
          clprintf("Checkpoint open returned 0x%x\n", rc);
          if (rc == SA_AIS_ERR_EXIST)
            {
              clDbgCodeError(SA_AIS_ERR_EXIST,("You reopened a checkpoint with different attributes then how it was originally opened"));
            }
          else
            {
              clDbgPause();
            }
          throw Error(ClError,rc,NoExtendedError,"Checkpoint open failure");
        }
    }


  void Table::SetAutoActivate(bool val)
  {
    autoActivate = val;
  }

  void Initialize()
    {  
      //clOsalMutexLock(&svcLock);
      //clOsalMutexInit(&activateMutex);

      SaAisErrorT rc = SA_AIS_OK;
      SaVersionT ckpt_version = {'B', 1, 1};
           
      /* Initialize checkpointing service instance */
      rc = saCkptInitialize(&svcHdl, NULL, &ckpt_version); 
      if (rc != SA_AIS_OK)
        {
          clDbgPause();
          clLogError("CPP","CKP","Failed to initialize checkpoint service error %x\n", rc);
          throw Error(SafError,rc);
        }
      clLogInfo("CPP","CKP","Checkpoint service initialized (handle=0x%llx)\n", svcHdl);
    }


  void Finalize()
    {
        SaAisErrorT rc = SA_AIS_OK;
        if (svcHdl != 0)
        {

            clLogInfo("CPP","CKP","Checkpoint service finalize (handle=0x%llx)\n", svcHdl);
            rc = saCkptFinalize(svcHdl);
            if (rc != SA_AIS_OK)
            {
                clLogError("CPP", "CKP", "Failed to finalize checkpoint service error %x\n", rc);
            }
            else
            {
                svcHdl = 0;
            }
        }
    }


  void Table::Activate()
    {
      SaAisErrorT rc = SA_AIS_OK;

      /* Synchronous checkpoints never need an "active" replica */
      if ((flags & SA_CKPT_WR_ALL_REPLICAS) == SA_CKPT_WR_ALL_REPLICAS) return;

      int tryAgain = 5;

      while (tryAgain > 0)
        {
          tryAgain--;
          if ((rc = saCkptActiveReplicaSet(handleTbl)) != SA_AIS_OK)
            {
              clLogError("CPP","CKP","checkpoint_replica_activate failed [0x%x] in ActiveReplicaSet", rc);
              ErrorHandler(rc);
            }
          else break;
        }
      if (rc != SA_AIS_OK) throw Error(SafError,rc);
    }

  void Table::ErrorHandler(SaAisErrorT rc)
    {
      if (rc != SA_AIS_OK)
        {
          /* System errors */
          if (rc == SA_AIS_ERR_TRY_AGAIN)
            {
              usleep(tryAgainDelayMs * 1000);
            }
          if (rc == SA_AIS_ERR_TIMEOUT)  // retry
            {

            }

          /* Coding errors */
          if (rc == SA_AIS_ERR_INVALID_PARAM)
            {
              clDbgCodeError(rc,("Invalid parameter"));
              throw Error(SafError,rc);
            }
          if (rc == SA_AIS_ERR_LIBRARY)
            {
              clDbgCodeError(rc,("Library is dead!"));
              throw Error(SafError,rc);
            }
          if (rc == SA_AIS_ERR_ACCESS)
            {
              clDbgCodeError(rc,("Incorrect access mode set."));
              throw Error(SafError,rc);
            }
          if (rc == SA_AIS_ERR_BAD_HANDLE)
            {
              clDbgCodeError(rc,("bad handle."));
              throw Error(SafError,rc);
            }


        }
    }

  void Table::Synchronize(unsigned int timeout)
    {
      /* Synchronous checkpoints never need to be explicitly synchronized */
      if ((flags & SA_CKPT_WR_ALL_REPLICAS) == SA_CKPT_WR_ALL_REPLICAS) return;

      SaAisErrorT rc = SA_AIS_OK;
      int tryAgain = 5;

      while (tryAgain > 0)
        {
          tryAgain--;
          // TODO: implement timeout
          rc = saCkptCheckpointSynchronize(handleTbl, CL_TIME_END );

          if (rc == SA_AIS_OK) break;
          else
            {
              clprintf("Handle [0x%llx]: Failed [0x%x] to synchronize the checkpoint\n",handleTbl, rc);
              ErrorHandler(rc);
            }
        }

      if (rc != SA_AIS_OK) throw Error(SafError,rc);
    }


  void Table::Delete(const Data& key)
    {
      int tryAgain = 5;
      SaAisErrorT rc = SA_AIS_OK;

      /* You can't delete the default section */
      if (maxSections == 1) return;

      SaCkptSectionIdT id =
        {
          (SaUint16T) key.length,
          (SaUint8T*) key.value
        };

      while (tryAgain > 0)
        {
          tryAgain--;

          rc = saCkptSectionDelete(handleTbl, &id);
          if (rc == SA_AIS_OK) tryAgain=0;
          else 
            {
              if (rc == SA_AIS_ERR_NOT_EXIST)
                {
                  if (GetActiveStatus())  // Is there an active replica?
                    break;  // User just deleted nonexistant section, this is deemed ok since the end result is what user wanted.
                  else
                    {
                      if (autoActivate) 
                        {
                          Activate();
                          rc = SA_AIS_OK;
                        }
                      else
                        {
                          throw Error(SafError,SA_AIS_ERR_NOT_EXIST, NoActiveReplica, "No active replica");
                        }
                    }
                }  
              ErrorHandler(rc);

            }
        }
      if (rc != SA_AIS_OK) throw Error(SafError,rc);
    }

  void Table::Read (const Data& key, Data* value)
  {
    SaUint32T err_idx; /* Error index in ioVector */

    SaCkptSectionIdT sid = SA_CKPT_DEFAULT_SECTION_ID;

    if (maxSections != 1)
      {
        sid.idLen = key.length;
        sid.id    = (SaUint8T*) key.value;
      }

    SaCkptIOVectorElementT iov;

    int tryAgain = 5;

    SaAisErrorT rc = SA_AIS_OK;

    while (tryAgain > 0)
      {
        tryAgain--;

        iov.sectionId = sid;
        iov.dataBuffer = value->value;
        iov.dataSize   = value->length;
        iov.dataOffset = 0;
        iov.readSize = value->length;
        
        rc = saCkptCheckpointRead(handleTbl, &iov, 1, &err_idx);
        clLogDebug("CPP","CKP", "Section read returned %d", rc);
        if (rc != SA_AIS_OK)
          {
            if (rc == SA_AIS_ERR_NOT_EXIST)
              {
                if (GetActiveStatus())  // Is there an active replica?
                  throw Error(SafError,SA_AIS_ERR_NOT_EXIST, SectionDoesNotExist, "Section does not exist");  // User read nonexistant section.
                else
                  {
                    if (autoActivate) 
                      {
                        Activate();
                        rc = SA_AIS_OK;
                      }
                    else
                      {
                        throw Error(SafError,SA_AIS_ERR_NOT_EXIST, NoActiveReplica, "No active replica");
                      }
                  }
              }

            clprintf("Error: [0x%x] from checkpoint read", rc );
            ErrorHandler(rc);
          }
        else  /* It returned success! */
          {
            tryAgain = 0;
            if (iov.readSize != value->length)
              {
                clLogWarning("CPP","CKP", "Section size [%d] different than expected [%d]", (int) iov.readSize, (int) value->length);
              }
          }
      }

    if (rc != SA_AIS_OK) throw Error(SafError,rc);
  }


  void Table::Write(const Data& key, const Data& value)
    {
      int tryAgain = 5;

      SaAisErrorT rc = SA_AIS_OK;

      while (tryAgain > 0)
        {
          tryAgain--;
    
          SaCkptSectionIdT sid = SA_CKPT_DEFAULT_SECTION_ID;

          if (maxSections != 1)
            {
              sid.idLen = key.length;
              sid.id    = (SaUint8T*) key.value;
            }
    
          /* Write checkpoint */
          rc = saCkptSectionOverwrite(handleTbl, &sid, value.value, value.length);
          //debugging log clLogWarning("CPP","CKP", "SectionOverwrite return %d", rc);

          if (rc == SA_AIS_OK) tryAgain = 0;  // Great, it worked!
          else
            {
              if (rc == SA_AIS_ERR_NOT_EXIST)  // Either section or active replica does not exist
                {
                  if (maxSections != 1)  // If there is just 1 section, we KNOW that its an activation problem because the section MUST exist
                    {
                      // Start by trying to create the new section.
                      //sid.idLen = key.length;
                      //sid.id    = (SaUint8T*) key.value;
                      SaCkptSectionCreationAttributesT section_atts = { &sid, sectionExpiration };

                      rc = saCkptSectionCreate(handleTbl, &section_atts, (const SaUint8T*) value.value, value.length);
                      //clLogWarning("CPP","CKP", "SectionCreate return %d", rc);
                    }

                  if (rc == SA_AIS_ERR_NOT_EXIST)
                    {
                      // Ok, the real problem was no active replica.  Since user is doing a write, the user must want this node to be active replica.
                      Activate();
                    }
                  else ErrorHandler(rc);
                }
              else ErrorHandler(rc);
            }
        }
      if (rc != SA_AIS_OK) throw Error(SafError,rc);
    }


  bool Table::GetActiveStatus(SaCkptCheckpointDescriptorT* status)
    {
      int tryAgain = 5;
      SaCkptCheckpointDescriptorT tmpStat;
      SaAisErrorT rc = SA_AIS_OK;

      // If the caller doesn't care about the status details, just pass in a temporary data structure
      if (!status) status = &tmpStat;

      while (tryAgain > 0)
        {
          tryAgain--;
    
          rc = saCkptCheckpointStatusGet(handleTbl,status);

          if (rc == SA_AIS_ERR_NOT_EXIST) return false;
          if (rc == SA_AIS_OK) return true;
        }

      throw Error(SafError,rc);
    }

  /*
   *
   */
  Table::~Table()
  {
  }

    Table::Iterator::Iterator(SaCkptCheckpointHandleT *handleTbl)
    {
        SaAisErrorT rc = SA_AIS_OK;
        this->handleIter = handleTbl;
        this->sectionIteratorHdl = 0;

        /*
         * Wrapper checkpoint section iterator
         */
        rc = saCkptSectionIterationInitialize(*handleIter, SA_CKPT_SECTIONS_ANY, 0,
                        &sectionIteratorHdl);

        CL_ASSERT(rc == SA_AIS_OK);

        // Retrieve ckpt data
        getCkptData();
    }

    Table::Iterator::Iterator(Data *pData, Data *pKey) : pData(pData), pKey(pKey)
    {
    }

    Table::Iterator::~Iterator()
    {
        if (pData->value)
        {
            clHeapFree(pData->value);
        }
        if (pKey->value)
        {
            clHeapFree(pKey->value);
        }
        if (sectionIteratorHdl)
        {
            saCkptSectionIterationFinalize(sectionIteratorHdl);
        }
    }

    /*
     * Compare with other node
     */
    bool Table::Iterator::operator!=(const Iterator& otherValue)
    {
        if (pKey->length == otherValue.pKey->length) {
            if ((pKey->value == NULL) || (otherValue.pKey->value == NULL)
                            || (!memcmp(pKey->value, otherValue.pKey->value,
                                            pKey->length))) {
                return false;
            }
        }
        return true;
    }

    void Table::Iterator::getCkptData(void)
    {
        SaAisErrorT rc = SA_AIS_OK;
        SaCkptIOVectorElementT iov;
        SaCkptSectionDescriptorT sectionDescriptor;
        SaUint32T err_idx; /* Error index in ioVector */

        rc = saCkptSectionIterationNext(sectionIteratorHdl,
                &sectionDescriptor);

        if (rc == SA_AIS_OK) {
            clLogDebug("MGT", "SYNC",
                            "clMgtCkptInitSync() Section '%s' expires %llx size "
                            "%llu state %x update %llx\n",
                            sectionDescriptor.sectionId.id,
                            (unsigned long long)sectionDescriptor.expirationTime,
                            (unsigned long long)sectionDescriptor.sectionSize,
                            sectionDescriptor.sectionState,
                            (unsigned long long)sectionDescriptor.lastUpdate);

            iov.sectionId = sectionDescriptor.sectionId;
            iov.dataBuffer = 0;
            iov.dataSize   = 0;
            iov.dataOffset = 0;
            iov.readSize = 0;

            rc = saCkptCheckpointRead(*handleIter, &iov, 1, &err_idx);
            if( SA_AIS_OK == rc )
            {
                pData = new Data(iov.dataBuffer, iov.readSize);
                pKey = new Data(sectionDescriptor.sectionId.id, sectionDescriptor.sectionId.idLen);
            }
        } else {
            pData = new Data();
            pKey = new Data();
        }
    }

    /*
     * Goto next iterator ckpt section
     */
    Table::Iterator& Table::Iterator::operator++(int)
    {
        // Retrieve ckpt data
        getCkptData();

        return (*this);
    }

    Table::Iterator Table::begin() {
        return (Table::Iterator(&handleTbl));
    }

    Table::Iterator Table::end() {
        return (Table::Iterator(new Data(), new Data()));
    }

};
