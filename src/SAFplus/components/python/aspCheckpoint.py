"""
@namespace aspCheckpoint

This module provides interfaces to access the services provided by OpenClovis ASP Checkpoint Service.
"""
import asp 
import misc
import types
import aspMisc

class CheckpointError(misc.Error):
  """Basic error exception class for Checkpoint problems.
  """

  def __init__(self,s,aspretcode=0):
    """Constructor
    @param s:             A string describing the problem
    @param aspretcode:    Optional error code returned from ASP
    """
    misc.Error.__init__(self,s,aspretcode)
    self.text = s
    self.aspErrorCode = aspretcode

  def __str__(self):
    return "%s: Return code '%s' " % (self.text, aspMisc.getErrorString(self.aspErrorCode[0]))

 
class Table:
    """Interface to ASP Checkpointing Service functionality
       User would create a Table object on which he can do read and write
    """

    def __init__(self, name, maxRows, maxRowSizeBytes, maxRowKeySizeBytes, flagsp=asp.SA_CKPT_WR_ALL_REPLICAS, retentionMs=asp.SA_TIME_MAX):
        """Constructor function for the Table class.
           It creats a new checkpoint with the given parameters.
        @param  name                   Name of the table/checkpoint to be created
        @param  flagsp                 This parameter indicates the type of checkpoint to be created. 
                                       The supported values are: SA_CKPT_WR_ALL_REPLICAS, SA_CKPT_WR_ACTIVE_REPLICA, 
                                       SA_CKPT_WR_ACTIVE_REPLICA_WEAK, SA_CKPT_CHECKPOINT_COLLOCATED. The meaning of these flags are as below:
           
                                       SA_CKPT_WR_ALL_REPLICAS - Creates a checkpoint with synchronous update option
                                       SA_CKPT_WR_ACTIVE_REPLICA - Creates a checkpoint with the asynchronous update option 
                                       and providing atomicity when updating the replicas
                                       SA_CKPT_WR_ACTIVE_REPLICA_WEAK - Creates a checkpoint with the asynchronous update option
                                       but there is no guarantee of atomicity with updating the replicas.
                                       SA_CKPT_CHECKPOINT_COLLOCATED - If this flag is specified at the creation time, there is no active
                                       replica created until user explicitly sets an Active replica for the checkpoint.

                                       For more details about these flags and their usage, refer to SAF Checkpoint Service specification.

        @param  retentionMs            Retention time indicates that any checkpoint that is inactive for this 
                                       duration will automatically deleted by the checkpoint service
        @param  maxSections            Maximum number of checkpoints for this checkpoint
        @param  maxSectionSizeBytes    Maximum size of each section
        @param  maxSectionIdSizeBytes  Maximum length of the section identifier.

        """
        #Initialize few attributes of _this_ checkpoint
        self.ckptName = name
        self.flags = flagsp
        self.retentionMs = retentionMs
        self.maxSections = maxRows
        self.maxSectionSizeBytes = maxRowSizeBytes
        self.maxSectionIdSizeBytes = maxRowKeySizeBytes
        self.sectionExpiration = asp.SA_TIME_END #by default sections will never expire
        self.autoActivate=True
        self.tryAgainDelayMs=250    #ms

        #First initialize checkpoint library
        ver=asp.SaVersionT()
        ver.releaseCode = ord("B")
        ver.majorVersion = 1
        ver.minorVersion = 1

        try:
            self.svcHdl = asp.saCkptInitialize(None,ver)
            if self.svcHdl == 0:
                raise CheckpointError("Checkpoint service initialize failed. NULL service handle returned from initialize", asp.SA_AIS_ERR_INIT)
        except Exception, e:
            print e
            raise CheckpointError("Checkpoint service initialize failed ", e)
            
        #Now open the checkpoint with given attributes
        create_atts=asp.SaCkptCheckpointCreationAttributesT()
        create_atts.creationFlags = flagsp
        create_atts.checkpointSize = self.maxSections * self.maxSectionSizeBytes
        create_atts.retentionDuration = retentionMs
        create_atts.maxSections       = self.maxSections
        create_atts.maxSectionSize    = self.maxSectionSizeBytes
        create_atts.maxSectionIdSize  = self.maxSectionIdSizeBytes

        print("Opening checkpoint %s attributes: flags: %d, size: %d, retention %u, maxSections: %d, maxSectionSize: %d, maxSectionIdSize: %d\n" % (name, create_atts.creationFlags, create_atts.checkpointSize, create_atts.retentionDuration, create_atts.maxSections, create_atts.maxSectionSize, create_atts.maxSectionIdSize))

        try:
            self.ckptHdl = asp.saCkptCheckpointOpen(self.svcHdl,
                                                name,
                                                create_atts,
                                                (asp.SA_CKPT_CHECKPOINT_CREATE | asp.SA_CKPT_CHECKPOINT_WRITE | asp.SA_CKPT_CHECKPOINT_READ),
                                                asp.SA_TIME_MAX)
            if self.ckptHdl == 0:
                raise CheckpointError("Checkpoint open failed. NULL handle returned from checkpoint open",asp.SA_AIS_ERR_BAD_HANDLE)
        except Exception, e:
            raise CheckpointError("Checkpoint open failed ", e)
            
        
    def ErrorHandler(self,s,e):
        """Function to handle the common error codes returned by ASP functions.
           @param s  String description for the exception
           @param e  Exception caught from invoking the ASP APIs
           """

        errorCode = e[0]
        if errorCode != asp.SA_AIS_OK:
            if errorCode == asp.SA_AIS_ERR_TRY_AGAIN:
                #We will wait for some time and try to redo the 
                #operation again
                time.sleep(self.tryAgainDelayMs)
                return

            elif errorCode == asp.SA_AIS_ERR_TIMEOUT:
                #The ASP function returned timeout. So we can just retry again
                return

            elif errorCode == asp.SA_AIS_ERR_INVALID_PARAM:
                #Input values to the API were not proper
                raise CheckpointError("Input values to the API were not proper",e);

            elif errorCode == asp.SA_AIS_ERR_LIBRARY:
                #There was an error from checkpoint library. Library cannot be used anymore
                raise CheckpointError("checkpoint library is dead", e);

            elif errorCode == asp.SA_AIS_ERR_BAD_HANDLE:
                #Passed handle was not proper
                raise CheckpointError("Passed handle to the library was not proper", e);

            else:
                #Other errors
                raise CheckpointError(s,e[0])

    def SetAutoActivate(self,val=True):
        """Set the global value of autoActivate to given value. Default being True
           When autoActivate is set to True, the library will take care of setting the
           Active repliaca for the asynchronous checkpoints. Otherwise, user will have to explicitly
           activate the replica using Activate() function.
           @param  val  Boolean value indicate whether to enable auto activate or not"""
        #Set the autoActivate variable to specified value
        self.autoActivate = val

    def Activate(self):
        """Set the active replica for the asynchronous checkpoints"""
        #Synchronous checkpoints never need an active replica.
        if (self.flags & asp.SA_CKPT_WR_ALL_REPLICAS) == asp.SA_CKPT_WR_ALL_REPLICAS:
            print("Activate is not required for Synchronous checkpoints")
            return

        tryAgain = 5
        while tryAgain > 0:
            tryAgain = tryAgain-1
            try:
                asp.saCkptActiveReplicaSet(self.ckptHdl)
                return
            except Exception, e:
                self.ErrorHandler("Failed to set Active replica for the checkpoint", e)

        #Failed even after 5 retries
        raise CheckpointError("Failed to set Active replica for the checkpoint", e)

    def Synchronize(self,timeout=asp.SA_TIME_END):
        """Synchronize checkpoint replicas
           @param  timeout  Timeout to wait for the completion of the update from the checkpoint service"""
        #Synchronous checkpoints never need to be explicitly synchronized
        if (self.flags & asp.SA_CKPT_WR_ALL_REPLICAS) == asp.SA_CKPT_WR_ALL_REPLICAS:
            print("Activate is not required for Synchronous checkpoints")
            return

        tryAgain = 5
        while tryAgain > 0:
            tryAgain = tryAgain-1
            try:
                asp.saCkptCheckpointSynchronize(self.ckptHdl, timeout)
                return
            except Exception, e:
                self.ErrorHandler("Failed to synchronize the replicsa for the checkpoint", e)

        #Failed even after 5 retries
        raise CheckpointError("Failed to synchronize the replicsa for the checkpoint", e)

    def Delete(self,key):
        """Delete the section of the checkpointed denoted by given key
           @param key String identifier for the checkpoint section"""
        tryAgain = 5

        #You cant delete the default section
        if self.maxSections == 1:
            return

        while tryAgain > 0:
            tryAgain = tryAgain-1
            
            try:
                asp.saCkptSectionDelete(self.ckptHdl,key)
                return
            except Exception, e:
                if e[0] == asp.SA_AIS_ERR_NOT_EXIST:
                    if self.GetActiveStatus(): #Is there an active replica
                        break       #User deleted a non-existing section. This is deemed ok since the end result is what user wants
                    else:
                         if self.autoActivate == True:
                            self.Activate()
                            continue
                         else:
                              raise CheckpointError("No Active replica",e)
                self.ErrorHandler("Failed to delete the section for the key %s" % key, e)

        #Failed even after 5 retries
        raise CheckpointError("Failed to delete the section for the key %s" % key, e)


    def Read(self,keys):
        """Read the checkpoint for the given section ids.
           @param  keys  A list of section ids or keys
           @return data  Returns the data stored in the given sections"""

        tryAgain = 5

        if type(keys) != list:
            print "Invalid input: The parameter should be a list of keys"

        while tryAgain > 0:
            tryAgain = tryAgain-1
            try:
                data = asp.saCkptCheckpointRead(self.ckptHdl, keys)
                return data
            except Exception, e:
                if e[0] == asp.SA_AIS_ERR_NOT_EXIST:
                    if self.GetActiveStatus() == True:
                        raise CheckpointError("Data does not exist for the given keys %s" % keys, e)
                    else:
                        print "Trying to set the active replica for the checkpoint"
                        if self.autoActivate == True:
                            self.Activate()
                            continue
                        else:
                            raise CheckpointError("No active replica set", e)
                #Its a different error, so invoke error handler
                self.ErrorHandler("Failed to read from the sections with keys " , e)

        #Failed even after 5 attempts
        raise CheckpointError("Failed to read from the sections with keys %s" % keys, e)

    def Write(self,keyOrContainer,data=None):
        """
        Write data into the table.  There are several ways to call this functions:
        Write("rowname","data"):  Writes the value "data" into the row named "rowname" (will TRUNCATE the existing record if new record is shorter)
        Write([("rowname","data"),("rowname2","data2")]):  List of tuples format  (will NOT truncate the existing record)
        Write({"rowname":"data","rowname2":"data2"}):  Dictionary format (will NOT truncate)

        @param keyOrContainer Either a string, in which case this is interpreted as the row (section) identifier, and "data" will be the data to write OR a container of key/data pairs (see the function discription for formats)
        @param data This is the data to write into the row if key is a string, otherwise this is NOT USED.
        """
        tryAgain = 5

        if type(keyOrContainer) == str:
            if data == None:
                print "ERROR: You need to provide the data to be written for the key"

        while tryAgain > 0:
            tryAgain = tryAgain-1
            try:
                if type(keyOrContainer) == str:
                    asp.saCkptSectionOverwrite(self.ckptHdl,keyOrContainer,data)
                else:
                    asp.saCkptCheckpointWrite(self.ckptHdl,keyOrContainer)

                #Write was successful. Return
                print "Section write was successful"
                return
            except Exception, e:
                if e[0] == asp.SA_AIS_ERR_NOT_EXIST:
                    if self.maxSections != 1:
                        try:
                            print "Section is not there. Trying to create the same"
                            asp.saCkptSectionCreate(self.ckptHdl,keyOrContainer,data)
                            continue
                        except Exception, e:
                            raise CheckpointError("Section creation failed: ", e)

                    #The real problem was no active replica. Since the user is doing section write, the user wants this node to be active replica.
                    if self.autoActivate == True:
                        self.Activate()
                        continue
                    else:
                        raise CheckpointError("No active replica set. Cannot write into the section", e)
                #The error is something other than asp.SA_AIS_ERR_NOT_EXIST
                self.ErrorHandler("Failed to write into the checkpoint", e)

        #Failed even after 5 attempts
        raise CheckpointError("Failed to write into the checkpoint ", e)

    def GetActiveStatus(self):
        """Get the active status of the checkpoint.
          @return  Returns the boolean value indicating the status of the checkpoint
        """

        tryAgain = 5

        while tryAgain > 0:
            tryAgain = tryAgain - 1
            
            status = asp.SaCkptCheckpointDescriptorT()
            try:
                asp.saCkptCheckpointStatusGet(self.ckptHdl, status)
                return True
            except Exception, e:
                if e[0] == asp.SA_AIS_ERR_NOT_EXIST:
                    return False
                else:
                    self.ErrorHandler("Failed to get the active status for the checkpoint", e)

        #Failed even after 5 attempts
        raise CheckpointError("Failed to get the active status for the checkpoint", e)
