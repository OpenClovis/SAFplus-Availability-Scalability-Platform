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
 * File        : clParserApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *
 * This is a wrapper on ezxml
 *
 *
 *****************************************************************************/

#ifndef _CL_PARSER_API_H_
#define _CL_PARSER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <ezxml.h>

    /******************************************************************************
     *  Constant and Macro Definitions
     *****************************************************************************/
#define CL_PARSER_BUFSIZE   EZXML_BUFSIZE 
#define CL_PARSER_NAMEM     EZXML_NAMEM   
#define CL_PARSER_TXTM      EZXML_TXTM 
#define CL_PARSER_DUP       EZXML_DUP   

    /******************************************************************************
     *  Data Types 
     *****************************************************************************/

#define ClParserPtrT   ezxml_t

typedef enum ClParserTagType
{
    CL_PARSER_STR_TAG,
    CL_PARSER_STREAM_TAG,
    CL_PARSER_IP_TAG,
    CL_PARSER_BOOL_TAG,
    CL_PARSER_UINT8_TAG,
    CL_PARSER_UINT16_TAG,
    CL_PARSER_UINT32_TAG,
    CL_PARSER_UINT64_TAG,
    /*User defined tags*/
    CL_PARSER_CUSTOM_TAG,
} ClParserTagTypeT;

/*
 * We use the high 16 bits.not used by std. ASP errorcodes.
 */
#define CL_TAG_FILTER_SHIFT (16)

#define CL_TAG_FILTER_MASK (256 << CL_TAG_FILTER_SHIFT )

#define __CL_TAG_FILTER(mask)  (CL_TAG_FILTER_MASK | (mask) )

#define CL_TAG_OK_MASK (1<<0)

#define CL_TAG_CONT_MASK (1<<1)

#define CL_TAG_SKIP_CHILD_MASK (1<<2)

#define CL_TAG_SKIP_CHILD_INSTANCES_MASK (1<<3)

#define CL_TAG_STOP_AFTER_CHILD_MASK (1<<4)

#define CL_TAG_SKIP_UPDATE_MASK (1<<5)

/*
 * Below return codes are selected to avoid error code clashes.
 */
#define CL_TAG_OK __CL_TAG_FILTER(CL_TAG_OK_MASK)
 /*
 * Skip processing the current child and continue with the next child   
 * at the tag which returns this code.
 */
#define CL_TAG_CONT __CL_TAG_FILTER(CL_TAG_CONT_MASK)
/*
 * Continue with tag processing but skip child eventually
*/
#define CL_TAG_SKIP_CHILD __CL_TAG_FILTER(CL_TAG_SKIP_CHILD_MASK)
/*
 * Skip the instances of the child thats being processed.
*/
#define CL_TAG_SKIP_CHILD_INSTANCES                     \
    __CL_TAG_FILTER(CL_TAG_SKIP_CHILD_INSTANCES_MASK)
/*
 * Stop tag processing after processing of the children
*/
#define CL_TAG_STOP_AFTER_CHILD __CL_TAG_FILTER(CL_TAG_STOP_AFTER_CHILD_MASK)
/*
 * Skip updating the value of the tag   
*/
#define CL_TAG_SKIP_UPDATE __CL_TAG_FILTER(CL_TAG_SKIP_UPDATE_MASK)

typedef enum ClParserTagOp
{
    CL_PARSER_TAG_FMT,
    CL_PARSER_TAG_DISPLAY,
} ClParserTagOpT;

struct ClParserData;
typedef struct ClParserTag
{
#define CL_PARSER_ATTR_SIZE (0x100)
    const ClCharT *pTag;
    ClParserTagTypeT tagType; /*the type of tag*/
    ClUint32T tagSize; /*size of the tag.*/
    ClUint32T tagOffset; /*The offset into the target*/
    /*tag format routine*/
    ClRcT (*pTagFmt)(struct ClParserTag *,struct ClParserData*,
                     ClPtrT,ClCharT *,ClPtrT,ClParserTagOpT);
    ClBoolT (*pTagValidate)(struct ClParserTag *,ClPtrT);
} ClParserTagT;

/*
 * A generic parser for parsing the IOC config.
 * The below is a data representation of the XML structure.
 */
typedef struct ClParserData
{
    const ClCharT *pTag;/*The name of this TAG*/
    ClUint32T numTags; /*number of tags at this level*/
    ClParserTagT *pTags;/*list of tag definitions at this level*/
    ClRcT (*pDataInit)(ClPtrT,ClUint32T*,ClPtrT *,ClUint32T *,ClParserTagOpT);
    /*
     * If base is statically defined, then datainit callback neednt be defined.
     */
    ClPtrT pBase;
    ClUint32T baseOffset;
    /*
     * Num of instances of this tag.
     * If 0, iterate through all the entries in the list.
     *
     */
    ClUint32T numInstances; 
    /*Set this to true if you want your child tag to be always present*/
    ClBoolT dontSkip;
    ClUint32T numChild; /*num of childs of this tag*/
    struct ClParserData *pTagChildren;/*children for this tag*/
} ClParserDataT;

typedef ClRcT (ClParserTagFmtFunctionT) (ClParserTagT *,ClParserDataT *pData,ClPtrT pBase,ClCharT *pAttr,ClPtrT pValue,ClParserTagOpT op);

typedef ClRcT (ClParserDataInitFunctionT)(ClPtrT pParentBase,ClUint32T *pNumInstances,ClPtrT *pBase,ClUint32T *pBaseOffset,ClParserTagOpT op);

typedef ClBoolT (ClParserTagValidateFunctionT)(ClParserTagT *,ClPtrT pValue);

#define CL_TAG_SIZE(type,field) (sizeof(((type*)0)->field))

#define CL_TAG_OFFSET(type,field) ((ClWordT)&((type*)0)->field)

#define CL_PARSER_NUM(st) ( sizeof((st))/sizeof ( (st)[0] ) )

#define CL_PARSER_OUTPUT(...) do {              \
    ClCharT buf[0xff+1];                        \
    snprintf(buf,sizeof(buf),__VA_ARGS__);      \
    clOsalPrintf(buf);                          \
}while(0)

#define CL_PARSER_GET_INSTANCES(node,instances) do {    \
    ClParserPtrT tempNode;                              \
    instances = 0;                                      \
    for(tempNode = node ; tempNode && ++instances;      \
        tempNode = (tempNode)->next);                   \
}while(0)

#define CL_PARSER_RC(rc) CL_RC(CL_CID_PARSER,rc)

/*****************************************************************************
 *  Functions
 *****************************************************************************/

ClParserPtrT clParserParseStr(char *s, size_t len);

ClParserPtrT clParserParseFd(int fd);

ClParserPtrT clParserParseFile(const char *file);

ClParserPtrT clParserParseFp(FILE *fp);

// returns the first child tag (one level deeper) with the given name or NULL if
// not found
ClParserPtrT clParserChild(ClParserPtrT xml, const char *name);

// returns the next tag of the same name in the same section and depth or NULL
// if not found
#define CL_PARSER_NEXT(xml)  ezxml_next((xml))

// Returns the Nth tag with the same name in the same section at the same depth
// or NULL if not found. An index of 0 returns the tag given.
ClParserPtrT clParserIdx(ClParserPtrT xml, int idx);

// returns the given tag's character content or empty string if none
#define CL_PARSER_TXT(xml)   ezxml_txt((xml))

// returns the value of the requested tag attribute, or NULL if not found
const char *clParserAttr(ClParserPtrT xml, const char *attr);

// Traverses the ezxml sturcture to retrieve a specific subtag. Takes a variable
// length list of tag names and indexes. The argument list must be terminated
// by either an index of -1 or an empty string tag name. Example: 
// title = ezxml_get(library, "shelf", 0, "book", 2, "title", -1);
// This retrieves the title of the 3rd book on the 1st shelf of library.
// Returns NULL if not found.
ClParserPtrT clParserGet(ClParserPtrT xml, ...);

// Converts an ezxml structure back to xml. Returns a string of xml data that
// must be freed.
char *clParserToXml(ClParserPtrT xml);

// returns a NULL terminated array of processing instructions for the given
// target
const char **clParserPI(ClParserPtrT xml, const char *target);

// frees the memory allocated for an ezxml structure
void clParserFree(ClParserPtrT xml);
    
// returns parser error message or empty string if none
const char *clParserError(ClParserPtrT xml);

// returns a new empty ezxml structure with the given root tag name
ClParserPtrT clParserNew(const char *name);

// wrapper for ezxml_new() that strdup()s name
#define CL_PARSER_NEW(name) ezxml_new_d((name))

// Adds a child tag. off is the offset of the child tag relative to the start of
// the parent tag's character content. Returns the child tag.
ClParserPtrT clParserAddChild(ClParserPtrT xml, const char *name, size_t off);

// wrapper for ezxml_add_child() that strdup()s name
#define CL_PARSER_ADD_CHILD(xml,name,off)     ezxml_add_child_d((xml), (name), (off))

// sets the character content for the given tag and returns the tag
ClParserPtrT clParserSetTxt(ClParserPtrT xml, const char *txt);

// wrapper for ezxml_set_txt() that strdup()s txt
#define CL_PARSER_SET_TXT(xml,txt)       ezxml_set_txt_d((xml), (txt)) 

// Sets the given tag attribute or adds a new attribute if not found. A value of
// NULL will remove the specified attribute.
void clParserSetAttr(ClParserPtrT xml, const char *name, const char *value);

// Wrapper for ezxml_set_attr() that strdup()s name/value. Value cannot be NULL.
#define CL_PARSER_SET_ATTR(xml,name,value) ezxml_set_attr_d((xml), (name), (value))

// sets a flag for the given tag and returns the tag
ClParserPtrT clParserSetFlag(ClParserPtrT xml, short flag);

// removes a tag along with all its subtags
void clParserRemove(ClParserPtrT xml);

// returns a pointer to parser structure of the file
// path: It is colon saperated string where each string is full path
//      path1:path2:path3 ......
// file: name of the file which is required to be opened
ClParserPtrT clParserOpenFile(const ClCharT *path, const ClCharT *file);

//Returns the root tag i.e. <OpenClovisAsp> which will have all the
//version tags as its root. So user can process what ever version
//of the config file, that is required.
ClParserPtrT clParserOpenFileAllVer(const ClCharT *path, const ClCharT *file);

//Returns the root tag for the given version. It searches for the version tag
//that has the v0, v1.. vn attribute that matches with the given version and then
//returns the root to that version tag.
ClParserPtrT clParserOpenFileWithVer(const ClCharT *path, const ClCharT *file, ClVersionT *version);

ClRcT clParseDisplay(ClParserDataT *pData);

ClRcT clParseXML(ClCharT *pPath,const ClCharT *pFileName,ClParserDataT *pData);

#ifdef __cplusplus
}
#endif

#endif /* _CL_PARSER_API_H_ */
