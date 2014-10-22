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
 * ModuleName  : utils
 * File        : parser.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is a wrapper on ezxml
 *****************************************************************************/
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef SOLARIS_BUILD
#include <strings.h>
#endif

#include "clParserApi.h"
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clDebugApi.h>

/*
 * Define the possible tag types
 */

#define CL_PARSER_TAG_RESULT_CHECK(rc)          \
    (rc) = ( (rc) == CL_OK ) ? CL_TAG_OK : (rc)

/*
 * Tag,instance and child result interpretation.
 * Multiple gotos where some of them could have been combined
 * are intentional for _readability_.
*/

#define CL_PARSER_TAG_RESULT(rc,label) do {     \
        CL_PARSER_TAG_RESULT_CHECK(rc);         \
        if( !( (rc) & CL_TAG_FILTER_MASK) )     \
        {                                       \
            goto label;                         \
        }                                       \
        if(( (rc) & CL_TAG_CONT_MASK))          \
        {                                       \
            goto label;                         \
        }                                       \
        if(( (rc) & CL_TAG_SKIP_UPDATE_MASK))   \
        {                                       \
            (rc) &= ~CL_TAG_SKIP_UPDATE_MASK;   \
            goto label;                         \
        }                                       \
}while(0)

#define CL_PARSER_TAG_DISPLAY_RESULT(rc,label) do { \
        CL_PARSER_TAG_RESULT_CHECK(rc);             \
        if(!( (rc) & CL_TAG_FILTER_MASK))           \
        {                                           \
            goto label;                             \
        }                                           \
        if(( (rc) & CL_TAG_CONT_MASK))              \
        {                                           \
            goto label;                             \
        }                                           \
        if(( (rc) & CL_TAG_SKIP_UPDATE_MASK))       \
        {                                           \
            (rc) &= ~CL_TAG_SKIP_UPDATE_MASK;       \
        }                                           \
}while(0)

#define CL_PARSER_TAGS_RESULT(tag,rc,label) do {                    \
        CL_PARSER_TAG_RESULT_CHECK(rc);                             \
        if(!( (rc) & CL_TAG_FILTER_MASK))                           \
        {                                                           \
            clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Error processing tag "  \
                                           ":%s\n",(tag)->pTag);   \
            goto label;                                             \
        }                                                           \
        if(( (rc) & CL_TAG_CONT_MASK))                              \
        {                                                           \
            goto label;                                             \
        }                                                           \
        if(( (rc) & CL_TAG_SKIP_CHILD_MASK))                        \
        {                                                           \
            goto label;                                             \
        }                                                           \
}while(0)

#define CL_PARSER_INSTANCE_RESULT(rc,label) do {            \
        CL_PARSER_TAG_RESULT_CHECK(rc);                     \
        if(!( (rc) & CL_TAG_FILTER_MASK))                   \
        {                                                   \
            goto label;                                     \
        }                                                   \
        (rc) &= ~(CL_TAG_CONT_MASK|CL_TAG_SKIP_CHILD_MASK|  \
                  CL_TAG_OK_MASK);                          \
        if((rc) == CL_TAG_FILTER_MASK)                      \
        {                                                   \
            (rc) = CL_OK;                                   \
        }                                                   \
        else                                                \
        {                                                   \
            goto label;                                     \
        }                                                   \
}while(0)

#define CL_PARSER_CHILD_RESULT(tag,rc,label) do {                   \
        CL_PARSER_TAG_RESULT_CHECK(rc);                             \
        if(!( (rc) & CL_TAG_FILTER_MASK))                           \
        {                                                           \
            clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Error processing tag "  \
                                           ":%s\n",(tag)->pTag);   \
            goto label;                                             \
        }                                                           \
        (rc) &= ~(CL_TAG_SKIP_CHILD_INSTANCES_MASK|CL_TAG_OK_MASK); \
        if((rc) == CL_TAG_FILTER_MASK)                              \
        {                                                           \
            (rc) = CL_OK;                                           \
        }                                                           \
        else                                                        \
        {                                                           \
            goto label;                                             \
        }                                                           \
}while(0)
        
#define CL_PARSER_RESULT(tag,rc,label) do {                         \
        CL_PARSER_TAG_RESULT_CHECK(rc);                             \
        if(!( (rc) & CL_TAG_FILTER_MASK))                           \
        {                                                           \
            clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Error processing tag "  \
                                           ":%s\n",(tag)->pTag);   \
            goto label;                                             \
        }                                                           \
        (rc) = CL_OK ;                                              \
}while(0)

#define PARSER_LOG_AREA_PARSER		"PAR"
#define PARSER_LOG_CTX_VERSION		"VER"
#define PARSER_LOG_CTX_DISPLAY		"DISPLAY"
#define PARSER_LOG_CTX_TAG			"TAG"
#define PARSER_LOG_CTX_CHILD		"CHD"
#define PARSER_LOG_CTX_XML			"XML"

static ClRcT clParseDisplayChild(ClParserDataT *pData,ClPtrT pParentBase,ClUint32T level);
static ClRcT clParseInstance(ClParserPtrT node,
                             ClParserDataT *pData,
                             ClPtrT pCurrentBase);

ClParserPtrT clParserParseStr(char *s, size_t len)
{
    return ezxml_parse_str(s, len);
}

ClParserPtrT clParserParseFd(int fd)
{
    return ezxml_parse_fd(fd);
}

ClParserPtrT clParserParseFile(const char *file)
{
    return ezxml_parse_file(file);
}

ClParserPtrT clParserParseFp(FILE *fp)
{
    return ezxml_parse_fp(fp);
}

ClParserPtrT clParserChild(ClParserPtrT xml, const char *name)
{
    return ezxml_child(xml, name);
}

ClParserPtrT clParserIdx(ClParserPtrT xml, int idx)
{
    return ezxml_idx(xml, idx);
}

const char *clParserAttr(ClParserPtrT xml, const char *attr)
{
    return ezxml_attr(xml, attr);
}


ClParserPtrT clParserGet(ClParserPtrT xml, ...)
{
    /*return ezxml_get(xml, ...);*/
    clOsalPrintf("Not Implemanted ............\n");
    return NULL;
}

char *clParserToXml(ClParserPtrT xml)
{
    return ezxml_toxml(xml);
}


const char **clParserPI(ClParserPtrT xml, const char *target)
{
    return ezxml_pi(xml, target);
}

void clParserFree(ClParserPtrT xml)
{
    ezxml_free(xml);
}

const char *clParserError(ClParserPtrT xml)
{
    return ezxml_error(xml);
}

ClParserPtrT clParserNew(const char *name)
{
    return ezxml_new(name);
}


ClParserPtrT clParserAddChild(ClParserPtrT xml, const char *name, size_t off)
{
    return ezxml_add_child(xml, name, off);
}

ClParserPtrT clParserSetTxt(ClParserPtrT xml, const char *txt)
{
    return ezxml_set_txt(xml, txt);
}

void clParserSetAttr(ClParserPtrT xml, const char *name, const char *value)
{
    ezxml_set_attr(xml, name, value);
}


ClParserPtrT clParserSetFlag(ClParserPtrT xml, short flag)
{
    return ezxml_set_flag(xml, flag);
}

void clParserRemove(ClParserPtrT xml)
{
    ezxml_remove(xml);
}

ClParserPtrT clParserOpenFileAllVer(const ClCharT *path, 
                                    const ClCharT *file)
{
    ClParserPtrT    top  = NULL;
    ClCharT         fileName[CL_MAX_NAME_LENGTH] = {0};
    ClCharT         configPath[CL_MAX_NAME_LENGTH] = {0};
    ClCharT         *pathName = NULL;
    ClCharT         *pTempChar = NULL;

    if(path == NULL || file == NULL)
        return NULL;

    strncpy(configPath, path, sizeof(configPath));

    pathName = strtok_r(configPath, ":", &pTempChar);
    while(pathName != NULL)
    {
        snprintf(fileName, sizeof(fileName), "%s/%s", pathName, file); 
        top = clParserParseFile(fileName);

        if(top != NULL)
            break;
        else
        {
            top = NULL;
        }
        pathName = strtok_r(NULL, ":", &pTempChar);
    }
#ifdef VXWORKS_BUILD
    /*
     * Incase the path name had a colon which is a partition indicator,
     * retry the open again with the absolute filename including the path.
     */
    if(!top)
    {
        snprintf(fileName, sizeof(fileName), "%s/%s", path, file);
        top = clParserParseFile(fileName);
    }
#endif    
    return top;
}

ClParserPtrT clParserOpenFileWithVer(const ClCharT *path, 
                                     const ClCharT *file, 
                                     ClVersionT *version)
{
    ClParserPtrT    top  = NULL;
    ClCharT         verStr[CL_MAX_NAME_LENGTH] = {0};
    ClParserPtrT    verTag = NULL;
    ClBoolT         found = 0;
    const ClCharT   *attrVal = NULL;
    ClUint32T       vIndex = 0;
    ClCharT         attrName[CL_MAX_NAME_LENGTH] = {0};

    if (path == NULL || file == NULL || version == NULL)
        return NULL;

    top = clParserOpenFileAllVer(path, file);
    if (top == NULL) goto done;
    
    snprintf(verStr,
             sizeof(verStr),
             "%d.%d.%d",
             version->releaseCode,
             version->majorVersion,
             version->minorVersion);
    
    /* top is pointing to <OpenClovisAsp> tag. Look for the child
     * with given version */
    verTag = clParserChild(top, "version");
    while (verTag != NULL)
    {
        vIndex = 0;
        snprintf(attrName, sizeof(attrName), "v%d",vIndex);
        attrVal = clParserAttr(verTag, attrName);
        while (attrVal != NULL)
        {
            if (!strcmp(attrVal, verStr))
            {
                found = 1;
                break;
            }
            vIndex++;
            snprintf(attrName, sizeof(attrName), "v%d",vIndex);
            attrVal = clParserAttr(verTag, attrName);
        }

        if (found == 1)
            break;

        verTag = verTag->next;
    }

    if (!found)
    {
        clLogWarning(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_VERSION,
                     "Could not find the entry for version %s in %s",verStr,file);
        goto done;
    }

    /* Get the attributes and look for version */
    return verTag->child;
done:
    return top;
}

ClParserPtrT clParserOpenFile(const ClCharT *path, const ClCharT *file)
{
    ClParserPtrT    top  = NULL;
    ClVersionT      currentVersion;

    currentVersion.releaseCode = CL_RELEASE_VERSION;
    currentVersion.majorVersion = CL_MAJOR_VERSION;
    currentVersion.minorVersion = CL_MINOR_VERSION;
    top = clParserOpenFileWithVer(path, file, &currentVersion);
    
    return top;
}

/*Process each tag*/
static ClRcT clParseTag(ClParserPtrT node,ClParserTagT *pTag,ClParserDataT *pData,ClPtrT pCurrentBase)
{
    ClRcT rc = CL_OK;
    const ClCharT *pAttr;
    ClPtrT pValue;
    ClPtrT pDest;
    pAttr = clParserAttr(node,pTag->pTag);

    if(pAttr == NULL)
    {
        clLogWarning(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"XML attribute [%s] not defined, but may be optional.",pTag->pTag);
        /* clDbgCodeError(CL_LOG_SEV_WARNING,("Required attribute %s in xml configuration file is not defined\n",pTag->pTag)); */
        goto out;
    }

    /*The place to copy*/
    pDest = (ClPtrT) ((ClUint8T*)pCurrentBase+pTag->tagOffset);

    pValue = (ClPtrT)calloc(1,pTag->tagSize);

    if(pValue == NULL)
    {
        rc = CL_PARSER_RC(CL_ERR_NO_MEMORY);
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"Error allocating memory\n");
        goto out;
    }

    switch(pTag->tagType)
    {
    case CL_PARSER_STR_TAG:
        {
            strncpy((ClCharT*)pValue,pAttr,pTag->tagSize);
        }
        break;
        /*byte stream*/
    case CL_PARSER_STREAM_TAG:
        {
            memcpy(pValue,pAttr,pTag->tagSize);
        }
        break;
    case CL_PARSER_IP_TAG:
        {
            in_addr_t addr = inet_addr((char *)pAttr);
            bcopy((const char*)&addr,pValue,pTag->tagSize);
        }
        break;
    case CL_PARSER_BOOL_TAG:
        {
            ClBoolT v = CL_FALSE;
            if(!strcasecmp(pAttr,"true"))
            {
                v = CL_TRUE;
            }
            else if(!strcasecmp(pAttr,"false"))
            {
                v = CL_FALSE;
            }
            else
            {
                clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"Unknown bool value:%s " \
                                               "for tag:%s\n",
                                               pAttr,pData->pTag);
                rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);
             
                goto out_free;
            }
            memcpy(pValue,&v,pTag->tagSize);
        }
        break;
    case CL_PARSER_UINT8_TAG:
        {
            ClUint8T v = (ClUint8T)atoi(pAttr);
            *(ClUint8T*)pValue = v;
        }
        break;
    case CL_PARSER_UINT16_TAG:
        {
            ClUint16T v = (ClUint16T)atoi(pAttr);
            memcpy(pValue,&v,pTag->tagSize);
        }
        break;
    case CL_PARSER_UINT32_TAG:
        {
            ClUint32T v = (ClUint32T)strtoul(pAttr,NULL,10);
            memcpy(pValue,&v,pTag->tagSize);
        }
        break;
    case CL_PARSER_UINT64_TAG:
        {
            ClUint64T v = (ClUint64T)strtoll(pAttr,NULL,10);
            memcpy(pValue,&v,pTag->tagSize);
        }
        break;
    case CL_PARSER_CUSTOM_TAG:
        if(pTag->pTagFmt == NULL)
        {
            rc = CL_PARSER_RC(CL_ERR_UNSPECIFIED);
            clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"Tag:%s - Custom tag type with no fmt handler\n",pTag->pTag);
            goto out_free;
        }
        break;
    default:
        {
            rc = CL_PARSER_RC(CL_ERR_UNSPECIFIED);
            clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"Unknown tag type:%d\n",pTag->tagType);
            goto out_free;
        }
    }

    if(pTag->pTagFmt)
    {
        rc = pTag->pTagFmt(pTag,pData,pCurrentBase,(ClCharT*)pAttr,pValue,CL_PARSER_TAG_FMT);

        CL_PARSER_TAG_RESULT(rc,out_free);
        /*
         * Validate tag
         */
        goto validate_tag;
    }

    /*Call tag validate*/
    validate_tag:

    if(pTag->pTagValidate &&
       pTag->pTagValidate(pTag,pValue) == CL_FALSE
       )
    {
        rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"Tag validation failed for tag:%s\n",pTag->pTag);
        goto out_free;
    }

    /*Now commit the value to the tag*/
    memcpy(pDest,pValue,pTag->tagSize);

    out_free:
    free(pValue);

    out:
    return rc;
}

/*Start processing the TAGs for this node*/
static ClRcT clParseTags(ClParserPtrT node,
                         ClParserDataT *pData,
                         ClPtrT pCurrentBase)
{
    register ClUint32T i;
    ClUint32T numTags = pData->numTags;
    ClParserTagT *pTags = pData->pTags;
    ClRcT rc = CL_OK;

    for(i = 0;i < numTags;++i)
    {
        ClRcT tagRc = rc;
        rc = clParseTag(node,pTags+i,pData,pCurrentBase);
        CL_PARSER_TAG_RESULT(rc,out);
        rc |= tagRc;
    }
    out:
    return rc;
}

/*Process through each child tag*/
static ClRcT clParseChild(ClParserPtrT parent,
                          ClParserPtrT child,
                          ClParserDataT *pChild,
                          ClPtrT pParentBase
                          )
{
    ClParserPtrT tempNode;
    ClUint32T numInstances = 0;
    ClPtrT pBase = NULL;
    ClUint32T baseOffset = 0;
    register ClUint32T i;
    
    ClRcT rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);

    pBase = pChild->pBase;
    baseOffset = pChild->baseOffset;
    numInstances = pChild->numInstances;

    if(!numInstances)
    {
        CL_PARSER_GET_INSTANCES(child,numInstances);
    }

    CL_ASSERT(numInstances >= 1);

    if(pChild->pDataInit 
        &&
       (rc = pChild->pDataInit(pParentBase,&numInstances,&pBase,&baseOffset,
                               CL_PARSER_TAG_FMT)
        ) != CL_OK
       )
    {
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_CHILD,"Error in data init function for tag:%s\n",pChild->pTag);
        goto out;
    }

    rc = CL_PARSER_RC(CL_ERR_UNSPECIFIED);

    if(pBase == NULL)
    {
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_CHILD,"Data base isnt set\n");
        goto out;
    }

    for(i = 0, tempNode = child ; i < numInstances && tempNode ; 
        ++i,tempNode = tempNode->next)
    {
        ClPtrT pCurrentBase = (ClPtrT) ((ClUint8T*)pBase + i*baseOffset);
        rc = clParseInstance(tempNode,pChild,pCurrentBase);
        CL_PARSER_INSTANCE_RESULT(rc,out);
    }
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clParseInstance(ClParserPtrT node,
                             ClParserDataT *pData,
                             ClPtrT pCurrentBase)
{
    ClRcT rc = CL_OK;
    register ClUint32T i;

    /*
     * First pass: process tags of this instance.
     * Second pass: process children
     *
     */
    rc = clParseTags(node,pData,pCurrentBase);

    CL_PARSER_TAGS_RESULT(pData,rc,out);

    /*
     * Iterate through the children of the parent tag.
    */
    for(i = 0; i < pData->numChild; ++i)
    {
        ClParserDataT *pChild = pData->pTagChildren+i;
        ClParserPtrT child = clParserChild(node,pChild->pTag);
        ClRcT childRc = rc ;
        if(child == NULL)
        {
            /*This could be a conditional child*/
            if(pChild->dontSkip == CL_TRUE)
            {
                clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_CHILD,"Child tag:%s not present when it "\
                                              "was supposed to be\n",
                                              pChild->pTag);
                rc = CL_PARSER_RC(CL_ERR_UNSPECIFIED);
                goto out;
            }
            /*safely skip*/
            continue;
        }
        rc = clParseChild(node,child,pChild,pCurrentBase);
        CL_PARSER_CHILD_RESULT(pChild,rc,out);
        rc = childRc;
    }
    out:
    return rc;
}

/*
 * The main parser entry point
 *
*/
ClRcT clParseXML(ClCharT *pPath,const ClCharT *pFileName,ClParserDataT *pData)
{
    ClRcT rc = CL_OK;
    ClParserPtrT root;

    rc = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);
    if(pFileName == NULL || pData == NULL)
    {
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_XML,"Invalid parameter\n");
        goto out;
    }
    if(pPath == NULL)
    {
        pPath = getenv("ASP_CONFIG");
        if(pPath == NULL)
        {
            clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_XML,"Please export ASP_CONFIG\n");
            goto out;
        }
    }

    /*Start the parse*/
    root = clParserOpenFile(pPath,pFileName);
    if(root == NULL)
    {
        clLogInfo(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_XML,"Opening xml configuration file [%s] failed",
                                       pFileName);
        goto out;
    }
    rc = clParseChild(NULL,root,pData,NULL);
    CL_PARSER_RESULT(pData,rc,out_free);

    out_free:
    clParserFree(root);

    out:
    return rc;
}

static ClRcT clParseDisplayTag(ClParserDataT *pData,ClParserTagT *pTag,
                               ClPtrT pBase,ClUint32T level
                               )
{
    ClPtrT pValue = NULL;
    ClPtrT pValueSource = NULL;
    ClRcT rc = CL_OK;
    ClCharT spaces[80] ;
    ClCharT attr[CL_PARSER_ATTR_SIZE] = {0};
    memset(spaces,' ',sizeof(spaces));

    pValue = calloc(1,pTag->tagSize);
    if(pValue == NULL)
    {
        goto out;
    }
    pValueSource =  (ClPtrT)( (ClUint8T*)pBase + pTag->tagOffset );

    /*Make a copy of the value*/
    memcpy(pValue,pValueSource,pTag->tagSize);

    if(pTag->pTagFmt)
    {
        rc = pTag->pTagFmt(pTag,pData,pBase,attr,pValue,CL_PARSER_TAG_DISPLAY);
        CL_PARSER_TAG_DISPLAY_RESULT(rc,out_free);
    }

    switch(pTag->tagType)
    {
    case CL_PARSER_STR_TAG:
        {
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value:%s\n",level,spaces,
                             pTag->pTag,(ClCharT*)pValue);
        }
        break;
    case CL_PARSER_STREAM_TAG:
        {
            CL_PARSER_OUTPUT("%.*sTAG Name:%s",level,spaces,pTag->pTag);
            CL_PARSER_OUTPUT(",Value:%.*s\n",pTag->tagSize,(ClCharT*)pValue);
        }
        break;
    case CL_PARSER_IP_TAG:
        {
            ClCharT *pIp = inet_ntoa(*(struct in_addr*)pValue);
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value:%s\n",level,spaces,
                             pTag->pTag,pIp);
        }
        break;
    case CL_PARSER_BOOL_TAG:
        {
            ClBoolT v = *(ClBoolT*)pValue;
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value:%s\n",level,spaces,
                             pTag->pTag,v == CL_TRUE ? "true" : "false");
        }
        break;
    case CL_PARSER_UINT8_TAG:
        {
            ClUint8T v = *(ClUint8T*)pValue;
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value:%#02x\n",level,spaces,
                             pTag->pTag,v);
        }
        break;
    case CL_PARSER_UINT16_TAG:
        {
            ClUint16T v = *(ClUint16T*)pValue;
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value:%d\n",level,spaces,
                             pTag->pTag,v);
        }
        break;
    case CL_PARSER_UINT32_TAG:
        {
            ClUint32T v = *(ClUint32T*)pValue;
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value:%d\n",level,spaces,
                             pTag->pTag,v);
        }
        break;
    case CL_PARSER_UINT64_TAG:
        {
            ClUint64T v = *(ClUint64T*)pValue;
            CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value: %lld\n",level,spaces,
                             pTag->pTag,v);
            
        }
        break;
    case CL_PARSER_CUSTOM_TAG:
        {
            /*Expected to fill in attr through tagfmt callback*/
            if(attr[0])
            {
                CL_PARSER_OUTPUT("%.*sTAG Name:%s,Value: %s\n",level,spaces,
                                 pTag->pTag,attr);

            }
        }
        break;
    default:
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_TAG,"Unknown tag type for tag:%s\n",pTag->pTag);
        rc = CL_PARSER_RC(CL_ERR_UNSPECIFIED);
        goto out_free;
    }
    
    out_free:
    free(pValue);

    out:
    return rc;
}

static ClRcT clParseDisplayTags(ClParserDataT *pData,ClPtrT pBase,ClUint32T level,ClUint32T instance)
{
    ClRcT rc = CL_OK;
    register ClUint32T i;
    ClCharT spaces[80] ;
    ClCharT line[80] ;
    memset(spaces,' ',sizeof(spaces));
    memset(line,'-',sizeof(line));
    CL_PARSER_OUTPUT("%.*s%d) %s\n",level,spaces,instance+1,pData->pTag);
    CL_PARSER_OUTPUT("%.*s%.*s\n",level,spaces,(ClUint32T)strlen(pData->pTag)+4,line);
    for(i = 0; i < pData->numTags; ++i)
    {
        ClParserTagT *pTag = pData->pTags + i;
        ClRcT tagRc = rc;
        rc = clParseDisplayTag(pData,pTag,pBase,level);
        CL_PARSER_TAG_DISPLAY_RESULT(rc,out);
        rc |= tagRc;
    }
    out:
    return rc;
}

static ClRcT clParseDisplayInstance(ClParserDataT *pData,ClPtrT pBase,ClUint32T level,ClUint32T instance)
{
    ClRcT rc;
    register ClUint32T i;
    rc = clParseDisplayTags(pData,pBase,level,instance);
    CL_PARSER_TAGS_RESULT(pData,rc,out);

    for(i = 0; i < pData->numChild; ++i)
    {
        ClRcT childRc = rc;
        ClParserDataT *pChild = pData->pTagChildren+i;
        rc = clParseDisplayChild(pChild,pBase,level+2);
        CL_PARSER_CHILD_RESULT(pChild,rc,out);
        rc = childRc;
    }
    out:
    return rc;
}

static ClRcT clParseDisplayChild(ClParserDataT *pData,ClPtrT pParentBase,ClUint32T level)
{
    ClUint32T numInstances = 1;
    register ClUint32T i;
    ClPtrT pBase = NULL;
    ClUint32T baseOffset = 0;
    ClRcT rc  = CL_OK;

    pBase = pData->pBase;
    baseOffset = pData->baseOffset;

    if(pData->pDataInit &&
       (rc = pData->pDataInit(pParentBase,&numInstances,&pBase,&baseOffset,
                              CL_PARSER_TAG_DISPLAY)
        ) != CL_OK)
    {
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_DISPLAY,"Error in data init\n");
        goto out;
    }

    rc = CL_PARSER_RC(CL_ERR_UNSPECIFIED);
    if(pBase == NULL)
    {
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_CHILD,"Data base isnt set\n");
        goto out;
    }

    CL_ASSERT(numInstances >= 1);

    for(i = 0; i < numInstances;++i)
    {
        ClPtrT pCurrentBase = (ClPtrT) ( (ClUint8T*)pBase + i*baseOffset );
        rc = clParseDisplayInstance(pData,pCurrentBase,level,i);
        CL_PARSER_INSTANCE_RESULT(rc,out);
    }

    rc = CL_OK;

    out:
    return rc;
}

ClRcT clParseDisplay(ClParserDataT *pData)
{
    ClRcT rc  = CL_PARSER_RC(CL_ERR_INVALID_PARAMETER);

    if(pData == NULL)
    {
        clLogError(PARSER_LOG_AREA_PARSER,PARSER_LOG_CTX_DISPLAY,"Invalid param\n");
        goto out;
    }
    rc = clParseDisplayChild(pData,NULL,0);
    CL_PARSER_RESULT(pData,rc,out);
    out:
    return rc;
}

