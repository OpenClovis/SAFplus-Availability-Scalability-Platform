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
#ifndef clCheckpointH
#define clCheckpointH

#include <stdio.h>
#include <string.h>

#include <saCkpt.h>
#include <clOsalApi.h>
#include <iterator>

namespace clCheckpoint
{

  extern SaCkptHandleT     svcHdl;
  extern ClOsalMutexT      svcLock;
  
  /**
   \brief Initialize the checkpoint infrastructure
   
   \par Description
      This function must be called before any checkpointing operations are
      used. It initialize the C++ checkpoint layer and the underlying SAF
      library.

   \par Exceptions
      clAsp::Error is raised if the SAF layer cannot be initialized.

   \sa clCheckpoint::Finalize()
   */
  void Initialize();


  /**
   \brief Finalize the checkpoint infrastructure
   
   \par Description
      This function must be called during program termination to shut down
      the checkpointing infrastructure.  No checkpoint APIs can be called
      after this function is called.

   \sa clCheckpoint::Initialize()
   */
  void Finalize();


  /**
   \brief Representation of checkpointed data
   
   \par Description
      This class is a wrapper for a pointer and length so that a chunk of
      data can be represented as a single unit.

   \sa clCheckpoint::Table
   */
  class Data
  {
  public:
    /** Default constructor, elements are initialized to 0 */
    Data() { length=0; value=0;}

    /** Constructor */    
    Data(void* val, int len)
      {
        value = val; length = len;
      }

    /** The length of the buffer */
    unsigned int length;
    /** The buffer */
    void*        value;
  };

  /**
   \brief Representation of checkpoint table
   
   \par Description
      A checkpoint table is similar to a database table.  It stores
      many records of data (sections) indexed by keys (called
      'section ids').

   \sa clCheckpoint::Data
   */  
  class Table
  {
  public:

    /** Default 2-phase constructor, elements are initialized to 0.
        Initialize must be called before use!
        
        \sa Initialize()
     */    
    Table();

    /** 1-phase constructor, see Initialize() for parameter descriptions, note that
        initialize should not be called.
        \sa Initialize()
     */        
    Table(const char* name, SaCkptCheckpointCreationFlagsT flags, unsigned int retentionMs, unsigned int maxSections, unsigned int maxSectionSizeBytes, unsigned int maxSectionIdSizeBytes);

  /**
   \brief Initialize the checkpoint table (phase 2 of construction)

   \param name The string identifying this table cluster-wide.

   \param flags SAF SaCkptCheckpointCreationFlagsT.  See the SAF docs for
   details. Your choices are:
   SA_CKPT_WR_ALL_REPLICAS,
   SA_CKPT_WR_ACTIVE_REPLICA,
   SA_CKPT_WR_ACTIVE_REPLICA_WEAK,
   SA_CKPT_CHECKPOINT_COLLOCATED

   
   \param retentionMs How long to keep this table around even if noone has
   it open.  This field is used to ensure that memory is reclaimed.
   However, note that setting it to a low value could mean that the table is
   lost during multi-node failures.  Whether and how long you choose to
   retain tables is very specific to your HA model.

   \param maxSections The number of rows in the table

   \param maxSectionSizeBytes The max length of the data in each section (aka table row)

   \param maxSectionIdSizeBytes  The maximum length of the section id (aka table key)
   
   \par Description
      This function is called to initialize the checkpoint table if the
      default constructor was used to create the table.  This function
      creates and initializes an underlying SAF checkpoint table.

   \par Exceptions
      clAsp::Error is raised if the SAF checkpoint table cannot be
      opened.

   */
    
    void Initialize(const char* name, SaCkptCheckpointCreationFlagsT flags, unsigned int retentionMs, unsigned int maxSections, unsigned int maxSectionSizeBytes, unsigned int maxSectionIdSizeBytes);

    /**
       \brief Convenience feature where C++ layer calls Activate at the
        appropriate time.
        
       \par Description
       Call this with true to change this checkpoint object's behavior to
       automatically "activate" on write.  Call just after calling
       Initialize or any time during the life of the object.  "Activation"
       is only needed for "WR_ACTIVE" style tables.  See SAF docs.
    */
    void SetAutoActivate(bool val);
 
  /**
   \brief Read a section of the checkpoint table
  
   \param key The section to read

   \param value The read data is put here

   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF read error
      that cannot be automatically handled
   */
    void Read (const Data& key, Data* value);

  /**
   \brief Write a section of the checkpoint table
  
   \param key The section to write

   \param value The data to write

   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF write error
      that cannot be automatically handled
   */    
    void Write(const Data& key, const Data& value);

  /**
   \brief Delete a section of the checkpoint table
  
   \param key The section to delete

   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF delete error
      that cannot be automatically handled
   */        
    void Delete(const Data& key);

  /**
   \brief For Async only (WR_ACTIVE), make this program the table "writer"
  
   \param key The section to delete

   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF error that 
      cannot be automatically handled
   */        
    void Activate();

  /**
   \brief For Async only (WR_ACTIVE), push changes to all other replicas
  
   \param timeout How long to try synchronizing (NOT IMPLEMENTED)

   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF error that 
      cannot be automatically handled
   */    
    void Synchronize(unsigned int timeout=0);

    class Iterator
    {
    private:
      Data *pData;
      Data *pKey;
      SaCkptSectionIterationHandleT sectionIterator;
    public:
      Iterator(SaCkptCheckpointHandleT *handle);
      ~Iterator();

      Iterator(Data *pData, Data *pKey);

      // assignment
      Iterator& operator=(const Iterator& otherValue);
      bool operator !=(const Iterator& otherValue);

      // increment the pointer to the next value
      Iterator& operator++();
      Iterator& operator++(int);

      SaCkptCheckpointHandleT *handle;
    };

    // the begin and end of the iterator look up
    Iterator begin();
    Iterator end();

  protected:

    SaCkptCheckpointHandleT  handle;
    SaTimeT                  sectionExpiration;
    bool                     autoActivate;
    SaTimeT                  tryAgainDelayMs;
    SaCkptCheckpointCreationFlagsT flags;
    int                      maxSections;

    bool GetActiveStatus(SaCkptCheckpointDescriptorT* status=NULL);
    
    void ErrorHandler(SaAisErrorT returnCode);

  };



};
#endif
