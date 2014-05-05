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

#include "clMgtRoot.hxx"
#include "clMgtObject.hxx"
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <clDebugApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;

namespace SAFplus
{
  ClMgtObject::ClMgtObject(const char* name)
  {
    Name.assign(name);
    Parent = NULL;
  }

  ClMgtObject::~ClMgtObject()
  {
    map<string, vector<ClMgtObject*>* >::iterator mapIndex;
    for(mapIndex = mChildren.begin(); mapIndex != mChildren.end(); ++mapIndex)
      {
        std::vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) (*mapIndex).second;
        //delete objs;  // GAS note: MUST be reference counted AND only if this object is new-ed.  Most objects will be members of other objects
      }
  }

  ClMgtObject* ClMgtObject::root(void)
  {
    ClMgtObject* ret = this;
    while(ret->Parent) ret = ret->Parent;
    return ret;
  }

  ClMgtObject* ClMgtObject::deepFind(const std::string &s)
  {
    ClMgtObject* ret = find(s);
    if (ret) return ret;
  
    ClMgtObjectMap::iterator it;
    ClMgtObjectMap::iterator end = mChildren.end();

    for (it = mChildren.begin();it != end;it++)
      {
        vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) it->second;
        int nObjs = objs->size();
        for(int i = 0; i < nObjs; i++)
          {
            ClMgtObject* obj = (ClMgtObject*) (*objs)[i];
            ret = obj->deepFind(s);
            if (ret) return ret;
          }
      }  
  }

  ClMgtObject* ClMgtObject::find(const std::string &s)
  {
    ClMgtObjectMap::iterator it = mChildren.find(s);
    if (it != mChildren.end())
      {
        vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) it->second;
        int nObjs = objs->size();
        assert(nObjs == 1);
        ClMgtObject* obj = (ClMgtObject*) (*objs)[0];
        return obj;
      }
    return NULL;
  }


  ClRcT ClMgtObject::bindNetconf(const std::string module, const std::string route)
  {
    return ClMgtRoot::getInstance()->bindMgtObject(CL_NETCONF_BIND_TYPE, this, module, route);
  }

  ClRcT ClMgtObject::bindSnmp(const std::string module, const std::string route)
  {
    return ClMgtRoot::getInstance()->bindMgtObject(CL_SNMP_BIND_TYPE, this, module, route);
  }

  ClRcT ClMgtObject::addKey(std::string key)
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

  ClRcT ClMgtObject::removeKey(std::string key)
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

  ClRcT ClMgtObject::addChildName(std::string name)
  {
    ClRcT rc = CL_OK;

    if (mChildren.find(name) != mChildren.end())
      {
        return CL_ERR_ALREADY_EXIST;
      }

    std::vector<ClMgtObject*> *objs = new (std::vector<ClMgtObject*>);

    mChildren.insert(pair<string, vector<ClMgtObject *>* >(name, objs));

    return rc;
  }

  ClRcT ClMgtObject::removeChildName(std::string name)
  {
    ClRcT rc = CL_ERR_NOT_EXIST;
    map<string, vector<ClMgtObject*>* >::iterator mapIndex = mChildren.find(name);

    /* Check if MGT module already exists in the database */
    if (mapIndex == mChildren.end())
      {
        return CL_ERR_NOT_EXIST;
      }

    std::vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) (*mapIndex).second;
    /* Remove MGT module out off the database */
    mChildren.erase(name);
    delete objs;

    return rc;
  }


  ClRcT ClMgtObject::addChildObject(ClMgtObject *mgtObject, const char* objectName)
    {
    std::string name(objectName);
    return addChildObject(mgtObject,name);
    }


  ClRcT ClMgtObject::addChildObject(ClMgtObject *mgtObject, const std::string& objectName)
  {
    ClRcT rc = CL_OK;
    if (mgtObject == NULL)
      {
        return CL_ERR_NULL_POINTER;
      }

    const std::string* name = &objectName;    
    if (name == nullptr) name = &mgtObject->Name;

    /* Check if MGT object already exists in the database */
    map<string, vector<ClMgtObject*>* >::iterator mapIndex = mChildren.find(*name);
    std::vector<ClMgtObject*> *objs;

    if (mapIndex != mChildren.end())
      {
        objs = (vector<ClMgtObject*>*) (*mapIndex).second;
      }
    else
      {
        objs = new vector<ClMgtObject*>;
        mChildren.insert(pair<string, vector<ClMgtObject *>* >(*name, objs));
      }

    /* Insert MGT object into the database */
    objs->push_back(mgtObject);
    mgtObject->Parent = this;

    return rc;
  }

  ClRcT ClMgtObject::removeChildObject(const std::string objectName, ClUint32T index)
  {
    ClRcT rc = CL_OK;

    map<string, vector<ClMgtObject*>* >::iterator mapIndex = mChildren.find(objectName);

    /* Check if MGT module already exists in the database */
    if (mapIndex == mChildren.end())
      {
        return CL_ERR_NOT_EXIST;
      }

    std::vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) (*mapIndex).second;

    /* Remove MGT module out off the database */
    if (index >= objs->size())
      {
        return CL_ERR_INVALID_PARAMETER;
      }
    objs->erase (objs->begin() + index);

    return rc;
  }

  ClMgtObject *ClMgtObject::getChildObject(const std::string objectName, ClUint32T index)
  {
    map<string, vector<ClMgtObject*>* >::iterator mapIndex = mChildren.find(objectName);

    /* Check if MGT module already exists in the database */
    if (mapIndex != mChildren.end())
      {
        vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) (*mapIndex).second;
        if (index < objs->size())
          {
            return (*objs)[index];
          }
      }

    return NULL;
  }

  /*
   * Virtual function called from netconf server to validate object data
   */
  ClBoolT ClMgtObject::set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
  {
    xmlChar                             *valstr, *namestr;
    int                                 ret, nodetyp, depth;

    char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN);
    if (!strChildData)
      {
        return CL_FALSE;
      }

    char                                keyVal[MGT_MAX_ATTR_STR_LEN];

    char                                strTemp[CL_MAX_NAME_LENGTH];
    std::vector<ClMgtObject*>           *objs = NULL;
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
    if (strcmp((char *)namestr, Name.c_str()))
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
              map<string, vector<ClMgtObject*>* >::iterator mapIndex = mChildren.find((char *)namestr);

              if (mapIndex == mChildren.end())
                {
                  xmlFreeTextReader(reader);
                  free(strChildData);
                  return CL_FALSE;
                }

              objs = (vector<ClMgtObject*>*) (*mapIndex).second;

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
                  ClMgtObject* mgtObject = (*objs)[i];

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

  void ClMgtObject::toString(std::stringstream& xmlString)
  {
    ClUint32T i;

    map<string, vector<ClMgtObject*>* >::iterator mapIndex;

    xmlString << "<" << Name << ">";

    for (mapIndex = mChildren.begin(); mapIndex != mChildren.end(); ++mapIndex)
      {
        vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) (*mapIndex).second;

        for(i = 0; i< objs->size(); i++)
          {
            ClMgtObject* mgtChildObject = (*objs)[i];
            mgtChildObject->toString(xmlString);
          }
      }

    xmlString << "</" << Name << ">";
  }

  void ClMgtObject::get(void **ppBuffer, ClUint64T *pBuffLen)
  {
    std::stringstream xmlString;

    toString(xmlString);

    *pBuffLen =  xmlString.str().length() + 1;

    if (*ppBuffer == NULL)
      {
        *ppBuffer = (void *) calloc(*pBuffLen, sizeof(char));
      }

    strncat((char *)*ppBuffer, xmlString.str().c_str(), *pBuffLen - 1 );
  }

  ClBoolT ClMgtObject::isKeysMatch(std::map<std::string, std::string> *keys)
  {
    ClBoolT isMatch = CL_TRUE;
    ClUint32T i;
    for (i = 0; i< Keys.size(); i++)
      {
        ClMgtObject *itemKey = getChildObject(Keys[i]);

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

  vector<string> *ClMgtObject::getChildNames()
  {
    return new vector<string>();
  }

  /* persistent db to database */
  ClRcT ClMgtObject::write()
  {

    return CL_OK;
  }

  /* unmashall db to object */
  ClRcT ClMgtObject::read()
  {

    return CL_OK;
  }

  /* iterator db key and bind to object */
  ClRcT ClMgtObject::iterator()
  {
    return CL_OK;
  }

  /*
   * Dump Xpath structure
   */
  void ClMgtObject::dumpXpath()
  {

  }

  std::string ClMgtObject::getFullXpath()
  {
    std::string xpath = "";
    if (Parent != NULL)
      {
        std::string parentXpath = Parent->getFullXpath();
        if (parentXpath.length() > 0)
          {
            xpath = parentXpath.append("/").append(this->Name);
          }
      }
    else
      {
        xpath.append("/").append(this->Name);
      }

    if (Keys.size() > 0)
      {
        ClMgtObject *itemKey = getChildObject(Keys[0]);
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

    return xpath;

  }

  void ClMgtObject::load()
  {

  }

  void ClMgtObject::dbgDumpChildren()
  {   
    ClMgtObjectMap::iterator iter;
    ClMgtObjectMap::iterator end = mChildren.end();
    for (iter = mChildren.begin(); iter != end; iter++)
      {
        std::string name = iter->first;
        vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) iter->second;
        int temp = objs->size();
        for(int i = 0; i < temp; i++)
          {
            ClMgtObject* s = (*objs)[i];
            printf("%s location: %p\n", name.c_str(), s);
          }      
      }
      
  }

  void deXMLize(const std::string& obj,ClMgtObject* context, bool& result)
  {
    if ((obj[0] == 't') || (obj[0] == 'T') || (obj[0] == '1')) { result=1; return; }
    if ((obj[0] == 'f') || (obj[0] == 'F') || (obj[0] == '0') || (obj[0] == 'n') || (obj[0] == 'N')) { result=0; return; }
    throw SerializationError("cannot deXMLize into a boolean");
  }

  void deXMLize(const std::string& obj,ClMgtObject* context, ClBoolT& result)
  {
    if ((obj[0] == 't') || (obj[0] == 'T') || (obj[0] == '1')) { result=1; return; }
    if ((obj[0] == 'f') || (obj[0] == 'F') || (obj[0] == '0') || (obj[0] == 'n') || (obj[0] == 'N')) { result=0; return; }
    throw SerializationError("cannot deXMLize into a boolean");
  }

}
