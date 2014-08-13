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
#include <iostream>

#include <clCommon.hxx>
#include <clMgtRoot.hxx>
#include <clMgtObject.hxx>
#include <clMgtDatabase.hxx>

extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */



using namespace std;

namespace SAFplus
  {
  MgtObject::MgtObject(const char* nam)
    {
    name.assign(nam);
    parent = NULL;
    }

  MgtObject::~MgtObject()
    {
    }

  MgtObject* MgtObject::root(void)
    {
    MgtObject* ret = this;
    while(ret->parent) ret = ret->parent;
    return ret;
    }



  ClRcT MgtObject::bind(Handle handle, const std::string module, const std::string route)
    {
    return MgtRoot::getInstance()->bindMgtObject(handle, this, module, route);
    }

  bool MgtObject::match( const std::string &name, const std::string &spec)
    {
    return (name == spec);  // TODO, add wildcard matching (* and ?)
    }

  MgtObject::Iterator MgtObject::begin(void) { return MgtObject::Iterator(); }
  MgtObject::Iterator MgtObject::end(void) { return MgtObject::Iterator(); }
  MgtObject::Iterator MgtObject::multiFind(const std::string &nameSpec) { return MgtObject::Iterator(); }
  MgtObject::Iterator MgtObject::multiMatch(const std::string &nameSpec) { return MgtObject::Iterator(); }

  MgtObject::Iterator::Iterator(const Iterator& i):b(i.b) 
    { 
    i.b->refs++;  
    }  // b must ALWAYS be valid

  MgtObject::Iterator& MgtObject::Iterator::operator=(const Iterator& i)
    { 
    b = i.b;
    b->refs++;  
    }  // b must ALWAYS be valid

  bool MgtIteratorBase::next() { clDbgCodeError(1, "base implementation should never be called");  }
  void MgtIteratorBase::del()  { if (this != &mgtIterEnd) clDbgCodeError(1, "base implementation should never be called"); }

  MgtIteratorBase mgtIterEnd;

  MgtObject* MgtObject::find(const std::string &name) { return nullptr; }
  MgtObject* MgtObject::deepFind(const std::string &name) { return nullptr; }
  MgtObject* MgtObject::deepMatch(const std::string &name) { return nullptr; }

  bool MgtObject::Iterator::operator !=(const MgtObject::Iterator& e) const
    {
    MgtIteratorBase* me = b;
    MgtIteratorBase* him = e.b;

    // in the case of end() e.value will be nullptr, triggering
    // this quick compare
    if (me->current.second != him->current.second) return true;  
    if (him->current.second == nullptr)
      {
      if (me->current.second == nullptr) return false;
      else return true; // one is null other is not; must be !=
      }
    else if (me->current.second == nullptr) return true;  // one is null other is not; must be !=
        
    // ok if this is "real" comparison of two iterators, check the
    // names also
    if (me->current.first != him->current.first) return true;

    // The iterators are pointing at the same object so we'll
    // define that as =, even tho the iterators themselves may not
    // be equivalent.
    return false;
    }

  ClRcT MgtObject::removeChildObject(const std::string& objectName)
    {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This node does not support children");
    return CL_ERR_NOT_EXIST;
    }

  ClRcT MgtObject::addChildObject(MgtObject *mgtObject, std::string const& objectName)
    {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This node does not support children");
    return CL_ERR_BAD_OPERATION;
    }

  ClRcT MgtObject::addChildObject(MgtObject *mgtObject, const char* objectName)
    {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This node does not support children");
    return CL_ERR_BAD_OPERATION;
    }

  void MgtObject::removeAllChildren() {}  // Nothing to do, base class has no children

#if 0
  ClRcT MgtObject::addKey(std::string key)
    {
    ClRcT rc = CL_OK;

    ClUint32T i;
    for(i = 0; i< Keys.size(); i++)
      {
      if (Keys[i].compare(key) == 0)
        {
        rc = CL_ERR_ALREADY_EXIST;
        //clLogWarning("MGT", "OBJ", "Key [%s] already exists", key.c_str());
        return rc;
        }
      }
    Keys.push_back(key);
    return rc;
    }

  ClRcT MgtObject::removeKey(std::string key)
    {
    ClRcT rc = CL_ERR_NOT_EXIST;
    ClUint32T i;

    for(i = 0; i< Keys.size(); i++)
      {
      if (Keys[i].compare(key) == 0)
        {
        Keys.erase (Keys.begin() + i);
        rc = CL_OK;
        break;
        }
      }
    return rc;
    }


  ClRcT MgtObject::addChildName(std::string name)
    {
    ClRcT rc = CL_OK;

    if (mChildren.find(name) != mChildren.end())
      {
      return CL_ERR_ALREADY_EXIST;
      }

    std::vector<MgtObject*> *objs = new (std::vector<MgtObject*>);

    mChildren.insert(pair<string, vector<MgtObject *>* >(name, objs));

    return rc;
    }

  ClRcT MgtObject::removeChildName(std::string name)
    {
    ClRcT rc = CL_ERR_NOT_EXIST;
    map<string, vector<MgtObject*>* >::iterator mapIndex = mChildren.find(name);

    /* Check if MGT module already exists in the database */
    if (mapIndex == mChildren.end())
      {
      return CL_ERR_NOT_EXIST;
      }

    std::vector<MgtObject*> *objs = (vector<MgtObject*>*) (*mapIndex).second;
    /* Remove MGT module out off the database */
    mChildren.erase(name);
    delete objs;

    return rc;
    }
#endif
#if 0
  /*
   * Virtual function called from netconf server to validate object data
   */
  ClBoolT MgtObject::set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
    {
    clDbgNotImplemented("MgtObject::set");

    xmlChar                             *valstr, *namestr;
    int                                 ret, nodetyp, depth;

    char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN);
    if (!strChildData)
      {
      return CL_FALSE;
      }

    char                                keyVal[MGT_MAX_ATTR_STR_LEN];

    char                                strTemp[CL_MAX_NAME_LENGTH];
    std::vector<MgtObject*>           *objs = NULL;
    std::map<std::string, std::string>  keys;
    ClBoolT                             isKey = CL_FALSE;
    ClUint32T                           i;

    //clLogDebug("MGT", "OBJ", "Validate [%.*s] ", (int) buffLen, (const char*)pBuffer);

    xmlTextReaderPtr reader = xmlReaderForMemory((const char*)pBuffer, buffLen, NULL,NULL, 0);
    if(!reader)
      {
      free(strChildData);
      return CL_FALSE;
      }

    ret = xmlTextReaderRead(reader);
    if (ret <= 0)
      {
      xmlFreeTextReader(reader);
      free(strChildData);
      return CL_FALSE;
      }

    namestr = (xmlChar *)xmlTextReaderConstName(reader);
    if (strcmp((char *)namestr, name.c_str()))
      {
      xmlFreeTextReader(reader);
      free(strChildData);
      return CL_FALSE;
      }

    while(ret)
      {
      depth = xmlTextReaderDepth(reader);
      nodetyp = xmlTextReaderNodeType(reader);
      namestr = (xmlChar *)xmlTextReaderConstName(reader);
      valstr = (xmlChar *)xmlTextReaderValue(reader);

      switch (nodetyp) {
      case XML_ELEMENT_NODE:
        /* classify element as empty or start */
        snprintf((char *)strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
        if (depth == 1)
          {
          strcpy(strChildData, strTemp);
          map<string, vector<MgtObject*>* >::iterator mapIndex = find((char *)namestr);

          if (mapIndex == mChildren.end())
            {
            xmlFreeTextReader(reader);
            free(strChildData);
            return CL_FALSE;
            }

          objs = (vector<MgtObject*>*) (*mapIndex).second;

          if (objs->size() == 0)
            {
            xmlFreeTextReader(reader);
            free(strChildData);
            return CL_FALSE;
            }

          keys.clear();
          }
        else if (depth == 2)
          {
          for (i = 0; i< (*objs)[0]->Keys.size(); i++)
            {
            if ((*objs)[0]->Keys[i].compare((char *)namestr) == 0)
              {
              isKey = CL_TRUE;
              break;
              }
            }
          }
        else if(depth > 2)
          {
          strcat(strChildData, strTemp);
          }
        break;
      case XML_ELEMENT_DECL:
        snprintf((char *)strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
        if (depth == 1)
          {
          strcat(strChildData, strTemp);

          for (i = 0; i< objs->size(); i++)
            {
            MgtObject* mgtObject = (*objs)[i];

            if (mgtObject->isKeysMatch(&keys) == CL_TRUE)
              {
              if(mgtObject->set(strChildData, strlen(strChildData), t) == CL_FALSE)
                {
                xmlFreeTextReader(reader);
                free(strChildData);
                return CL_FALSE;
                }
              }
            }
          }
        else if (depth == 1)
          {
          strcat(strChildData, strTemp);
          if (isKey == CL_TRUE)
            {
            keys.insert(pair<string, string>((char *)namestr, keyVal));
            isKey = CL_FALSE;
            }
          }
        else if (depth > 2)
          {
          strcat(strChildData, strTemp);
          }
        break;
      case XML_TEXT_NODE:
        if (depth > 1)
          {
          strcat(strChildData, (char *)valstr);
          if (isKey == CL_TRUE)
            {
            strcpy(keyVal, (char *)valstr);
            }
          }
        break;
      default:
        /* unused node type -- keep trying */
        break;
        }

      ret = xmlTextReaderRead(reader);
      }

    xmlFreeTextReader(reader);
    free(strChildData);
    return CL_TRUE;
    }
#endif
  void MgtObject::get(std::string *data, ClUint64T *datalen)
  {
    std::stringstream xmlString;
    if(data == NULL)
      return;
    toString(xmlString);
    logDebug("---","---","String: %s",xmlString.str().c_str());
    *datalen =  xmlString.str().length() + 1;
    data->assign(xmlString.str().c_str());
  }

#if 0
  ClBoolT MgtObject::isKeysMatch(std::map<std::string, std::string> *keys)
    {
    ClBoolT isMatch = CL_TRUE;
    ClUint32T i;
    for (i = 0; i< Keys.size(); i++)
      {
      MgtObject *itemKey = getChildObject(Keys[i]);

      map<string, string>::iterator mapIndex = keys->find(Keys[i]);
      if (mapIndex == keys->end())
        {
        isMatch = CL_FALSE;
        break;
        }

      if (itemKey)
        {
        string keyVal = static_cast<string>((*mapIndex).second);

        if (keyVal.compare(itemKey->strValue()) != 0)
          {
          isMatch = CL_FALSE;
          break;
          }
        }
      else
        {
        isMatch = CL_FALSE;
        break;
        }
      }
    return isMatch;
    }
#endif

  /* persistent db to database */
  ClRcT MgtObject::write(ClMgtDatabase* db)
  {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This function didn't support");
    return CL_ERR_NOT_EXIST;
  }

  /* unmashall db to object */
  ClRcT MgtObject::read(ClMgtDatabase* db)
  {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This function didn't support");
    return CL_OK;
  }


  /*
   * Dump Xpath structure
   */
  void MgtObject::dumpXpath()
    {

    }

  std::string MgtObject::getFullXpath()
    {
    std::string xpath = "";
    if (parent != NULL)
      {
      std::string parentXpath = parent->getFullXpath();
      if (parentXpath.length() > 0)
        {
        xpath = parentXpath.append("/").append(this->name);
        }
      }
    else
      {
      xpath.append("/").append(this->name);
      }

#if 0
    if (Keys.size() > 0)
      {
      MgtObject *itemKey = getChildObject(Keys[0]);
      xpath.append("[@").append(Keys[0]).append("='");
      if (itemKey)
        {
        xpath.append(itemKey->strValue()).append("'");
        }

      for(int i = 1; i< Keys.size(); i++)
        {
        itemKey = getChildObject(Keys[i]);
        xpath.append(",@").append(Keys[i]).append("=");
        if (itemKey)
          {
          xpath.append(itemKey->strValue()).append("'");
          }
        }
      xpath.append("]");
      }
#endif

    return xpath;

    }


  void MgtObject::dbgDumpChildren()
    {
      std::stringstream dumpStrStream;
      MgtObject::Iterator iter;
      MgtObject::Iterator endd = end();
      for (iter = begin(); iter != endd; iter++)
        {
          const std::string& name = iter->first;
          MgtObject* obj = iter->second;
          obj->toString(dumpStrStream);
        }
      logDebug("MGT","DUMP", "%s", dumpStrStream.str().c_str());
    }

  void deXMLize(const std::string& obj,MgtObject* context, bool& result)
    {
    if ((obj[0] == 't') || (obj[0] == 'T') || (obj[0] == '1')) { result=1; return; }
    if ((obj[0] == 'f') || (obj[0] == 'F') || (obj[0] == '0') || (obj[0] == 'n') || (obj[0] == 'N')) { result=0; return; }
    throw SerializationError("cannot deXMLize into a boolean");
    }

  void deXMLize(const std::string& obj,MgtObject* context, ClBoolT& result)
    {
    if ((obj[0] == 't') || (obj[0] == 'T') || (obj[0] == '1')) { result=1; return; }
    if ((obj[0] == 'f') || (obj[0] == 'F') || (obj[0] == '0') || (obj[0] == 'n') || (obj[0] == 'N')) { result=0; return; }
    throw SerializationError("cannot deXMLize into a boolean");
    }

  }
