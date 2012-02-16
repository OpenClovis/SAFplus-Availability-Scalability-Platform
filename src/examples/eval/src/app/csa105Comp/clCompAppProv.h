/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/templates/default/clCompAppMain.h $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provides a skeleton for writing a SAF aware component. Application
 * specific code should be added between the ---BEGIN_APPLICATION_CODE--- and
 * ---END_APPLICATION_CODE--- separators.
 *
 * Template Version: 1.0
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef CL_COMP_APP_PROV
#define CL_COMP_APP_PROV

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Application Transaction Life Cycle Management Functions
 *****************************************************************************/

/**
 ***********************************************************
 *  \brief Transaction start callback function which will be called
 *  before forwarding any of the transaction requests to this application.
 *
 *  \param txnHandle    (in) : Unique handle for the transaction.
 *
 *  \retval 
 *  None.
 *
 *  \par Description:
 *  This callback function is used to let the user know that the transactional
 *  operations are going to be forwarded to this application. This will be
 *  called before forwarding any of the transaction requests.
 *
 *  The 'txnHandle' is used to uniquely identify the transaction.
 *
 *  \note
 *  None
 *
 *  \sa
 *  clCompAppProvTxnEnd
 *
 */
void
clCompAppProvTxnStart(ClHandleT txnHandle);

/**
 ***********************************************************
 *  \brief Transaction end callback function which will be called
 *  after all the transaction requests for this application are 
 *  completed.
 *
 *  \param txnHandle    (in) : Unique handle for the transaction.
 *
 *  \retval 
 *  None.
 *
 *  \par Description:
 *  This callback function is used to let the user know that all the 
 *  transactional operations for this application are completed. This will be
 *  called at the end of all the transaction callbacks.
 *
 *  The 'txnHandle' is used to uniquely identify the transaction.
 *
 *  \note
 *  None
 *
 *  \sa
 *  clCompAppProvTxnStart
 *
 */
void
clCompAppProvTxnEnd(ClHandleT txnHandle);

#ifdef __cplusplus
}
#endif

#endif //CL_COMP_APP_PROV
