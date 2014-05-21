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

  MgtObject* MgtList::operator [](const char* name)
    {
    Map::iterator it = children.find(name);
    if (it == children.end()) return nullptr;
    return it->second;
    }

  void MgtList::removeAllChildren()
    {
    children.clear();
    }

  ClUint32T MgtList::getEntrySize()
    {
    return (ClUint32T) children.size();
    }

  void MgtList::toString(std::stringstream& xmlString)
    {
    Map::iterator i;
    Map::iterator end = children.end();
    for (i = children.begin(); i != end; i++)
      {
      MgtObject *entry = i->second;
      assert(entry);  // should be no NULL entries... erase don't nullptr them
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
    children.erase(objectName);
    }

  ClRcT MgtList::addChildObject(MgtObject *mgtObject, const std::string& objectName)
    {
    ClRcT rc = CL_OK;
    assert(mgtObject);

    const std::string* name = &objectName;
    if (!name) name = &mgtObject->name;
    children[*name] = mgtObject;
    }

  MgtObject::Iterator MgtList::begin(void)
    {
    MgtObject::Iterator ret;
    Map::iterator bgn = children.begin();
    Map::iterator end = children.end();
    if (bgn == end) // Handle the empty map case
      {
      ret.b = &mgtIterEnd;
      }
    else
      {
      HiddenIterator* h = new HiddenIterator();
      h->it = bgn;
      h->end = end;
      h->current.first = h->it->first;
      h->current.second = h->it->second;
      ret.b  = h;
      }

    return ret;
    }

  bool MgtList::HiddenIterator::next()
    {
    it++;
    if (it == end)
      {
      current.first = "";
      current.second = nullptr;
      return false;
      }
    else
      {
      current.first = it->first;
      current.second = it->second;
      return true;
      }
    }

  void MgtList::HiddenIterator::del()
    {
    delete this;
    }

  };
