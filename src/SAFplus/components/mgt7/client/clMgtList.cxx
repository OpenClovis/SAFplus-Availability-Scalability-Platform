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

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors.h>
#include <clDebugApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;

#ifdef SAFplus7
#define clLog(...)
#endif

namespace SAFplus
{
  ClMgtList::ClMgtList(const char* name) :
    ClMgtObject(name)
  {
  }

  ClMgtList::~ClMgtList()
  {
  }

  ClRcT ClMgtList::addKey(std::string key)
  {
    ClRcT rc = CL_OK;

    ClUint32T i;
    for (i = 0; i < mKeys.size(); i++)
      {
        if (mKeys[i].compare(key) == 0)
          {
            rc = CL_ERR_ALREADY_EXIST;
            clLogWarning("MGT", "LIST", "Key [%s] already exists", key.c_str());
            return rc;
          }
      }
    mKeys.push_back(key);
    return rc;
  }

  ClRcT ClMgtList::removeKey(std::string key)
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

  void ClMgtList::removeAllKeys()
  {
    mKeys.clear();
  }

  ClBoolT ClMgtList::isEntryExist(ClMgtObject* entry)
  {
    ClBoolT isExist = CL_FALSE;

    ClUint32T i, j;

    for (i = 0; i < mEntries.size(); i++)
      {
        ClBoolT itemFound = CL_TRUE;
        for (j = 0; j < mKeys.size(); j++)
          {
            ClMgtObject *inputKey = entry->getChildObject(mKeys[j]);
            ClMgtObject *itemKey = mEntries[i]->getChildObject(mKeys[j]);

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
  }

  ClMgtObject* ClMgtList::findEntryByKeys(
                                          std::map<std::string, std::string> *keys)
  {
    ClUint32T i, j;

    for (i = 0; i < mEntries.size(); i++)
      {
        ClBoolT itemFound = CL_TRUE;
        for (j = 0; j < mKeys.size(); j++)
          {
            ClMgtObject *itemKey = mEntries[i]->getChildObject(mKeys[j]);

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

  ClRcT ClMgtList::addEntry(ClMgtObject* entry)
  {
    ClRcT rc = CL_OK;

    if (entry == NULL)
      {
        clLogError("MGT", "LIST", "Null pointer input");
        return CL_ERR_NULL_POINTER;
      }

    if (isEntryExist(entry) == CL_TRUE)
      {
        clLogWarning("MGT", "LIST", "Entry [%s] already exists",
                     entry->Name.c_str());
        return CL_ERR_ALREADY_EXIST;
      }

    mEntries.push_back(entry);
    addChildObject(entry, entry->Name);
    return rc;
  }

  ClRcT ClMgtList::removeEntry(ClUint32T index)
  {
    ClRcT rc = CL_ERR_NOT_EXIST;

    if ((0 <= index) && (index < mEntries.size()))
      {
        removeChildObject(mEntries[index]->Name);
        mEntries.erase(mEntries.begin() + index);
        rc = CL_OK;
      }

    return rc;
  }

  void ClMgtList::removeAllEntries()
  {
    mEntries.clear();
  }

  ClMgtObject* ClMgtList::getEntry(ClUint32T index)
  {
    if ((0 <= index) && (index < mEntries.size()))
      {
        ClMgtObject *entry = mEntries[index];
        return entry;
      }

    return NULL;
  }

  ClUint32T ClMgtList::getEntrySize()
  {
    return (ClUint32T) mEntries.size();
  }
  ClUint32T ClMgtList::getKeySize()
  {
    return (ClUint32T) mKeys.size();
  }

  void ClMgtList::toString(std::stringstream& xmlString)
  {
    ClUint32T i;

    for (i = 0; i < mEntries.size(); i++)
      {
        ClMgtObject *entry = mEntries[i];

        entry->toString(xmlString);
      }

  }

  ClBoolT ClMgtList::set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
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
                for (i = 0; i < mKeys.size(); i++)
                  {
                    if (mKeys[i].compare((char *) namestr) == 0)
                      {
                        isKey = CL_TRUE;
                        break;
                      }
                  }
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
                ClMgtObject* entry = findEntryByKeys(&keys);
                if (entry != NULL)
                  {
                    if (entry->set(strChildData, strlen(strChildData), t)
                        == CL_FALSE)
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


};
