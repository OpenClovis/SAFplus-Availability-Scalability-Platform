#ifndef checkpointFnsH
#define checkpointFnsH

#ifdef __cplusplus
extern "C" {
#endif

#define CKPT_NAME     "csa103Ckpt"  /* Checkpoint name for this application  */
SaCkptHandleT  ckptLibraryHandle; /* Checkpointing service handle       */
SaCkptCheckpointHandleT ckpt_handle;  /* Checkpoint handle        */
#define CKPT_SID_NAME "1"           /* Checkpoint section id                 */
extern SaCkptSectionIdT ckpt_sid;

extern SaAisErrorT checkpoint_initialize(void);
extern void        checkpoint_finalize(void);
extern SaAisErrorT       checkpoint_write_seq(ClUint32T);
extern SaAisErrorT      checkpoint_read_seq(ClUint32T*);

#ifdef __cplusplus
  }
#endif

#endif
