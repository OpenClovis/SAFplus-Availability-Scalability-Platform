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
 * ModuleName  : alarm                                                         
 * File        : clAlarmCorDbgCmdUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *******************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clCorApi.h>
#include <clEoApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <ctype.h>

extern ClRcT clCorNIClassIdGet(char *name, ClCorClassTypeT *key);
extern ClRcT clCorClassTypeFromNameGet(char *name, ClCorClassTypeT *pClassId);

ClInt32T clAlarmCorAtoI(char *str)
{
    ClUint8T len;
    ClUint32T outVal;
                                                                                                                                                             
    len = strlen(str);
    /* if string length < 2, then the number can-not be in hex for sure.*/
    if (len<=2)
    {
        outVal = atoi(str);
        return outVal;
    }
    else
    {
        ClUint8T tmplen = strlen("0x");
        /* check if entered number is in hex format */
        if(!strncmp(str, "0x", tmplen) || !strncmp(str, "0X", tmplen))
        {
            sscanf(str, "%x", &outVal);
            return outVal;
        }
        else
        {
            outVal = atoi(str);
            return outVal;
        }
    }
}


static ClInt32T
clAlarmMoPathWordGet(char *name, ClCharT *word, ClInt32T *idx)
{
    ClInt32T i = 0;
    ClInt32T j = *idx;
                                                                                                                                                             
    if( (NULL == name) || (NULL == word) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    if (name[j] == 0)
        return (0);
                                                                                                                                                             
    while(1)
    {
        if (name[j] && (name[j] != '\\'))
        {
            word[i] = name[j];
            i++;
            j++;
        }
        else
        {
            word[i] = 0;
            if (name[j] == '\\')
                *idx = j+1;
            else
                *idx = j;
            return (1);
        }
    }
    return(1);
}

static ClInt32T
clAlarmMoPathInstanceGet(char *word, ClInt32T *inst)
{
    ClInt32T i = 0;
    ClInt32T j = 0;
    ClCharT   tmp[10];
    ClInt32T    wsize = 0;
                                                                                                                                                             
    if( (NULL == word) || (NULL == inst) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
                                                                                                                                                             
    if(isalpha(word[0]) || !isxdigit(word[0]))
    {
      /* Path is in form of names.*/
      /* Take care of scenario where class type is wildcard */
       while ((word[j] == '_') ||(word[j] == '*') ||(word[j] && isalpha(word[j]))
              || isdigit(word[j]))
       {
          wsize++;
          j++;
       }
                                                                                                                                                             
       if (word[j] && word[j] == '*')
       {
          word[wsize] = 0;
          *inst = CL_COR_INSTANCE_WILD_CARD;
          return 0;
       }
       while(1)
       {
          if (word[j] && (word[j] != '\\'))
          {
              tmp[i] = word[j];
              i++;
              j++;
          }
          else
          {
              ClInt32T k = 0;
              ClInt32T found = 0;
              tmp[i] = 0;
              word[wsize] = 0;
              for(k=0; k<i-1; k++)
              {
                  if(tmp[k] == ':')
                  {
                      found = 1;
                      break;
                  }
              }
              if(found == 1)
                  *inst = clAlarmCorAtoI(&tmp[k+1]);
              else
              {
                     CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Improper agrument \
                                 given \n"));
                     return -1;
              }
              return 0;
          }
      }
   }
   else
   {
      /* Path is in form of actual ids.*/
       while(isxdigit(word[j]) || (word[j] == '*') || (word[j] == 'x') || (word[j] == 'X'))
       {
          wsize++;
          j++;
       }
                                                                                                                                                             
                                                                                                                                                             
       if (word[j] && word[j] == '*')
       {
          word[wsize] = 0;
          *inst = CL_COR_INSTANCE_WILD_CARD;
          return 0;
       }
                                                                                                                                                             
      /* go past '.' to get the inst.*/
      if((word[j] == '.'))
       j++;
       while(1)
       {
          if (word[j] && (word[j] != '\\'))
          {
              tmp[i] = word[j];
              i++;
              j++;
          }
          else
          {
              ClInt32T k = 0;
              ClInt32T found = 0;
              tmp[i] = 0;
              word[wsize] = 0;
              for(k=0; k<i; k++)
              {
                  if(tmp[k] == ':')
                  {
                      found = 1;
                      break;
                  }
              }
              if(found == 1)
                  *inst = clAlarmCorAtoI(&tmp[k+1]);
              else
              {
                     CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n Improper agrument \
                                 given \n"));
                     return -1;
              }
              return 0;
          }
      }
                                                                                                                                                             
   }
}


ClRcT
clAlarmCorXlateMOPath(char *path, ClCorMOIdPtrT cAddr)
{
    ClInt32T  widx;
    ClCharT wrd[80];
    ClInt32T  aidx;
    ClCorClassTypeT tkey;
    ClInt32T inst;
    ClRcT rc = CL_OK;
                                                                                                                                                             
    if( NULL == path )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
                                                                                                                                                             
    widx = 0;
    aidx = 0;
    clCorMoIdInitialize (cAddr);
    if(path[0] != '\\')
    {
        cAddr->qualifier = CL_COR_MO_PATH_RELATIVE;
    }
                                                                                                                                                             
                                                                                                                                                             
    if (path[widx] == '\\')
        widx = 1;
                                                                                                                                                             
    while(clAlarmMoPathWordGet(path, wrd, &widx))
    {
        ClInt32T k = 0;
        inst = 0;
        k = clAlarmMoPathInstanceGet(wrd, &inst);
        if(k == -1)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
    /* Take care of wildcard class type */
    if(wrd[0] != '*')
        {
          if(wrd[0] && !(isdigit(wrd[0])))
          {
              rc = clCorNIClassIdGet(wrd, &tkey);
              if (CL_GET_ERROR_CODE(rc) != CL_COR_UTILS_ERR_INVALID_KEY) /*CL_COR_UTILS_ERR_MEMBER_NOT_FOUND)*/
               {
                   cAddr->node[aidx].type     = tkey;
                   cAddr->node[aidx].instance = inst;
                   cAddr->depth++;
                   aidx++;
               }
              else{
                  return(!(CL_OK));
                  }
           }
           else
           {
              tkey = clAlarmCorAtoI(wrd);
              cAddr->node[aidx].type     = tkey;
              cAddr->node[aidx].instance = inst;
              cAddr->depth++;
              aidx++;
           }
        }
    else
        {
        /* This is a wildcard class type. Do not get it from NI table */
        cAddr->node[aidx].type = CL_COR_INSTANCE_WILD_CARD;
        cAddr->node[aidx].instance = inst;
        cAddr->depth++;
        aidx++;
        }
    }
    if (aidx)
        return(CL_OK);
    else
        return(!(CL_OK));
}



