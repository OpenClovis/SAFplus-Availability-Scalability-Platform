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

#include "clMgtList.hxx"
#include "clLogApi.hxx"

extern "C"
  {
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

#include <clCommonErrors.h>
//#include <clDebugApi.h>
  } /* end extern 'C' */

using namespace std;

namespace SAFplus
  {
  MgtList::MgtList(const char* name) :
    MgtObject(name)
    {
    }

  MgtList::~MgtList()
    {
    }

#if 0
  ClRcT MgtList::addKey(std::string key)
    {
    ClRcT rc = CL_OK;

    ClUint32T i;
    for (i = 0; i < mKeys.size(); i++)
      {
      if (mKeys[i].compare(key) == 0)
        {
        rc = CL_ERR_ALREADY_EXIST;
        logWarning("MGT", "LIST", "Key [%s] already exists", key.c_str());
        return rc;
        }
      }
    mKeys.push_back(key);
    return rc;
    }

  ClRcT MgtList::removeKey(std::string key)
    {
    ClRcT rc = CL_ERR_NOT_EXIST;
    ClUint32T i;

    for (i = 0; i < mKeys.size(); i++)
      {
      if (mKeys[i].compare(key) == 0)
        {
        mKeys.erase(mKeys.begin() + i);
        rc = CL_OK;
        break;
        }
      }
    return rc;
    }

  void MgtList::removeAllKeys()
    {
    mKeys.clear();
    }
#endif


  ClBoolT MgtList::isEntryExist(MgtObject* entry)
    {
    std::vector<MgtObject*>::iterator it;
    std::vector<MgtObject*>::iterator end = mEntries.end();
    for (it=mEntries.begin();it!=end; it++)
      {
      if (*it && ((*it)->name == entry->name)) return true;
      }
    return false;

#if 0
    ClBoolT isExist = CL_FALSE;

    ClUint32T i, j;

    for (i = 0; i < mEntries.size(); i++)
      {

      ClBoolT itemFound = CL_TRUE;
      for (j = 0; j < mKeys.size(); j++)
        {
        MgtObject *inputKey = entry->getChildObject(mKeys[j]);
        MgtObject *itemKey = mEntries[i]->getChildObject(mKeys[j]);

        if ((inputKey != NULL) && (itemKey))
          {
          if (inputKey->strValue().compare(itemKey->strValue()) != 0)
            {
            itemFound = CL_FALSE;
            break;
            }
          }
        else
          {
          itemFound = CL_FALSE;
          break;
          }
        }
      if (itemFound)
        {
        isExist = CL_TRUE;
        break;
        }
      }

    return isExist;
#endif
    }


#if 0
  MgtObject* MgtList::findEntryByKeys(std::map<std::string, std::string> *keys)
    {
    ClUint32T i, j;

    for (i = 0; i < mEntries.size(); i++)
      {
      ClBoolT itemFound = CL_TRUE;
      for (j = 0; j < mKeys.size(); j++)
        {
        MgtObject *itemKey = mEntries[i]->getChildObject(mKeys[j]);

        map<string, string>::iterator mapIndex = keys->find(mKeys[j]);
        if (mapIndex == keys->end())
          {
          itemFound = CL_FALSE;
          break;
          }

        if (itemKey)
          {
          string keyVal = static_cast<string>((*mapIndex).second);

          if (keyVal.compare(itemKey->strValue()) != 0)
            {
            itemFound = CL_FALSE;
            break;
            }
          }
        else
          {
          itemFound = CL_FALSE;
          break;
          }
        }
      if (itemFound)
        {
        return mEntries[i];
        }
      }
    return NULL;
    }
#endif


  ClRcT MgtList::removeEntry(ClUint32T index)
    {
    ClRcT rc = CL_ERR_NOT_EXIST;

    if ((0 <= index) && (index < mEntries.size()))
      {
      mEntries.erase(mEntries.begin() + index);
      rc = CL_OK;
      }

    return rc;
    }

  void MgtList::removeAllChildren()
    {
    mEntries.clear();
    }

  MgtObject* MgtList::getEntry(ClUint32T index)
    {
    if ((0 <= index) && (index < mEntries.size()))
      {
      MgtObject *entry = mEntries[index];
      return entry;
      }

    return NULL;
    }

  ClUint32T MgtList::getEntrySize()
    {
    return (ClUint32T) mEntries.size();
    }

  void MgtList::toString(std::stringstream& xmlString)
    {
    ClUint32T i;

    for (i = 0; i < mEntries.size(); i++)
      {
      MgtObject *entry = mEntries[i];
      if (entry)
        entry->toString(xmlString);
      }

    }

  ClBoolT MgtList::set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
    {
    xmlChar *valstr, *namestr;
    int ret, nodetyp, depth;

    char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN);
    if (!strChildData)
      {
      return CL_FALSE;
      }

    char keyVal[MGT_MAX_ATTR_STR_LEN];

    char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
    ClBoolT isKey = CL_FALSE;
    std::map < std::string, std::string > keys;
    ClUint32T i;

    xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen,
      NULL, NULL, 0);
    if (!reader)
      {
      free(strChildData);
      return CL_FALSE;
      }

    do
      {
      depth = xmlTextReaderDepth(reader);
      nodetyp = xmlTextReaderNodeType(reader);
      namestr = (xmlChar *) xmlTextReaderConstName(reader);
      valstr = (xmlChar *) xmlTextReaderValue(reader);

      switch (nodetyp)
        {
        case XML_ELEMENT_NODE:
          /* classify element as empty or start */
          snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
          if (depth == 0)
            {
            strcpy(strChildData, strTemp);
            keys.clear();
            }
          else if (depth == 1)
            {
            strcat(strChildData, strTemp);
#if 0 // TODO
            for (i = 0; i < mKeys.size(); i++)
              {
              if (mKeys[i].compare((char *) namestr) == 0)
                {
                isKey = CL_TRUE;
                break;
                }
              }
#endif
            }
          else
            {
            strcat(strChildData, strTemp);
            }
          break;
        case XML_ELEMENT_DECL:
          snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
          if (depth == 0)
            {
            strcat(strChildData, strTemp);
            MgtObject* entry = NULL;  // TODO: was findEntryByKeys(&keys);
            if (entry != NULL)
              {
              if (entry->set(strChildData, strlen(strChildData), t) == CL_FALSE)
                {
                xmlFreeTextReader(reader);
                free(strChildData);
                return CL_FALSE;
                }
              }
            }
          else if (depth == 1)
            {
            strcat(strChildData, strTemp);
            if (isKey == CL_TRUE)
              {
              keys.insert(pair<string, string>((char *) namestr, keyVal));
              isKey = CL_FALSE;
              }
            }
          else
            {
            strcat(strChildData, strTemp);
            }
          break;
        case XML_TEXT_NODE:
          if (depth > 0)
            {
            strcat(strChildData, (char *) valstr);
            if (isKey == CL_TRUE)
              {
              strcpy(keyVal, (char *) valstr);
              }
            }
          break;
        default:
          /* unused node type -- keep trying */
          break;
        }

      ret = xmlTextReaderRead(reader);
      } while (ret);

    xmlFreeTextReader(reader);
    free(strChildData);
    return CL_TRUE;
    }

  ClRcT MgtList::removeChildObject(const std::string& objectName)
    {
    for (int i = 0; i < mEntries.size(); i++)
      {
      MgtObject *entry = mEntries[i];
      if ((entry)&&(entry->name == objectName)) { mEntries[i] = nullptr; return CL_OK; }
      }
    return CL_ERR_NOT_EXIST;
    }

  ClRcT MgtList::addChildObject(MgtObject *mgtObject, const std::string& objectName)
    {
    ClRcT rc = CL_OK;
    assert(mgtObject);
    assert(&objectName == nullptr);  // lists do not have named elements

    assert(isEntryExist(mgtObject)==false);
    mEntries.push_back(mgtObject);
    }
  };
