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

/**
 *  \file
 *  \brief Header file of ClMgtList class which provides APIs to manage Yang lists
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTLIST_HXX_
#define CLMGTLIST_HXX_

#include <string>
#include <vector>

#include "clMgtObject.hxx"
#include "clMgtMsg.hxx"

#include <boost/container/map.hpp>

extern "C"
{
  #include <libxml/xmlreader.h>
  #include <libxml/xmlmemory.h>
  #include <libxml/parser.h>
  #include <libxml/xmlstring.h>
  #include <clCommon.h>
  #include <clCommonErrors.h>
} /* end extern 'C' */

namespace SAFplus
{
  /**
   *  MgtList class provides APIs to manage Yang lists
   *  A multiple key class must implement three mandatory functions:
   *  - Operator <: for comparing key instances
   *  - void build: for building an instance from string list
   *  - std::ostream& operator<<: for serializing key class to string
   */
  template<class KEYTYPE>
  class MgtList : public MgtObject
  {
    protected:
      /**
       * Store the list of entries which KEYTYPE class is the key of list
       */
        typedef boost::container::map<KEYTYPE, MgtObject*> Map;
        Map children;
        /**
         * An internal iterator
         */
        class HiddenIterator:public MgtIteratorBase
        {
          public:
            typename Map::iterator it;
            typename Map::iterator end;

            virtual bool next()
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
                current.first = keyTypeToString(it->first);
                current.second = it->second;
                return true;
              }
            }
            virtual void del()
            {
              delete this;
            }
        };

    public:
      /**
       * Store the list of key names
       */
      typedef std::map<std::string,std::string> keyMap;
      keyMap keyList;
      /**
       * API to specify key names for the list
       */
      void setListKey(std::string keyName)
      {
        keyList.insert(std::make_pair(keyName,""));
      }
      /**
       * MgtList constructor
       */
      MgtList(std::string objectKey):MgtObject(objectKey.c_str()){}
      /**
       * MgtList destructor
       */
      virtual ~MgtList() {}
      /**
       * API to detect whether an entry is exist in the current list (based on its name)
       */
      ClBoolT isEntryExist(MgtObject* entry)
      {
        typename Map::iterator iter = children.begin();
        while(iter != children.end())
        {
          MgtObject* curEntry = iter->second;
          if(curEntry->name.compare(entry->name) == 0)
          {
            return CL_TRUE;
          }
        }
        return CL_FALSE;
      }
      /**
       * API to add new entry into the list
       */
      virtual ClRcT addChildObject(MgtObject *mgtObject, KEYTYPE &objectKey = *(KEYTYPE *)NULL)
      {
        ClRcT rc = CL_OK;
        assert(mgtObject);
        KEYTYPE *key = &objectKey;
        if(key == NULL)
        {
          return CL_ERR_NULL_POINTER;
        }
        children[*key] = mgtObject;
        return CL_OK;
      }
      /**
       * API to remove entry from the list
       */
      virtual ClRcT removeChildObject(const KEYTYPE objectKey)
      {
        children.erase(objectKey);
      }
      /**
       * API to remove all entries from the list
       */
      void removeAllChildren()
      {
        children.clear();
      }
      /**
       * API to iterate thought objects in the list
       */
      MgtObject::Iterator begin(void)
      {
        MgtObject::Iterator ret;
        typename Map::iterator bgn = children.begin();
        typename Map::iterator end = children.end();
        if (bgn == end) // Handle the empty map case
        {
          ret.b = &mgtIterEnd;
        }
        else
        {
          HiddenIterator* h = new HiddenIterator();
          h->it = bgn;
          h->end = end;
          h->current.first = keyTypeToString(h->it->first);
          h->current.second = h->it->second;
          ret.b  = h;
        }
        return ret;
      }
      /**
       * Shortcut to find an entry in the list
       */
      MgtObject* operator [](const KEYTYPE objectKey)
      {
        typename Map::iterator it = children.find(objectKey);
        if (it == children.end()) return nullptr;
        return it->second;
      }
      /**
       * API to get data of the list (called from netconf server)
       */
      virtual void toString(std::stringstream& xmlString)
      {
        typename Map::iterator i;
        typename Map::iterator end = children.end();
        for (i = children.begin(); i != end; i++)
        {
          MgtObject *entry = i->second;
          assert(entry);  // should be no NULL entries... erase don't nullptr them
          if (entry)
          {
            entry->toString(xmlString);
          }
        }
      }
      /**
       * API to get number of entries in the list
       */
      ClUint32T getEntrySize()
      {
        return (ClUint32T) children.size();
      }
      /**
       * API to set list data from netconf server (XML format)
       */
      virtual ClBoolT set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
      {
        keyMap::iterator iter;
        KEYTYPE entryKey;
        xmlChar *valstr, *namestr;
        int ret, nodetyp, depth;
        ClBoolT isKey = CL_FALSE;
        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN); /* Store the XML for child */
        if (!strChildData)
        {
          return CL_FALSE;
        }
        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
        if (!reader)
        {
          free(strChildData);
          return CL_FALSE;
        }
        /* Parse XM: */
        do
        {
           depth = xmlTextReaderDepth(reader);
           nodetyp = xmlTextReaderNodeType(reader);
           namestr = (xmlChar *) xmlTextReaderConstName(reader);
           valstr = (xmlChar *) xmlTextReaderValue(reader);
           switch (nodetyp)
           {
             case XML_ELEMENT_NODE: /* Node name */
               snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
               if (depth == 0) /* Highest level node name */
               {
                 strcpy(strChildData, strTemp);
               }
               else /* Lower level node name */
               {
                 strcat(strChildData, strTemp);
                 iter = keyList.find(std::string((const char *)namestr));
                 if(iter != keyList.end())
                 {
                   isKey = CL_TRUE;
                 }

               }
               break;
             case XML_ELEMENT_DECL: /* Closing tag of node name */
               snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
               strcat(strChildData, strTemp);
               if(depth == 0) /* Closing of an fully entry */
               {
                 /* Build the entryKey from name/val of set keys */
                 // FIXME: entryKey.build(keyList);
                 MgtObject *entry = children[entryKey];
                 if(entry != NULL)
                 {
                   if(entry->set(strChildData,strlen(strChildData),t) == CL_FALSE)
                   {
                     xmlFreeTextReader(reader);
                     free(strChildData);
                     return CL_FALSE;
                   }
                 }
               }
               break;
             case XML_TEXT_NODE:
               strcat(strChildData, (char *) valstr);
               if(isKey)
               {
                 isKey = CL_FALSE;
                 iter->second.assign(std::string((const char *)valstr));
               }
               break;
             default:
               break;
           }
           ret = xmlTextReaderRead(reader);
        } while(ret);

        return CL_TRUE;
      }
      /**
       * Function to convert KEYTYPE to string
       */
      static std::string keyTypeToString(KEYTYPE key)
      {
        std::stringstream stream;
        stream << key;
        return stream.str();
      }
  };
  /**
   * For backward compatible with current version of MgtList which key is std::string
   */
  template<>
  class MgtList<std::string> : public MgtObject
  {
    protected:
      typedef boost::container::map<std::string, MgtObject*> Map;
      Map children;
      /**
       * An internal iterator
       */
      class HiddenIterator:public MgtIteratorBase
      {
        public:
          typename Map::iterator it;
          typename Map::iterator end;

          virtual bool next()
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
          virtual void del()
          {
            delete this;
          }
      };
    public:
      /**
       * Store the key name
       */
      std::string keyList;
      /**
       * API to specify key names for the list
       */
      void setListKey(std::string keyName)
      {
        keyList.assign(keyName);
      }
      /**
       * MgtList constructor
       */
      MgtList(std::string objectKey):MgtObject(objectKey.c_str()){}
      /**
       * MgtList destructor
       */
      virtual ~MgtList(){ }
      /**
       * API to detect whether an entry is exist in the current list (based on its name)
       */
      ClBoolT isEntryExist(MgtObject* entry)
      {
        Map::iterator iter = children.begin();
        while(iter != children.end())
        {
          MgtObject* curEntry = iter->second;
          if(curEntry->name.compare(entry->name) == 0)
          {
            return CL_TRUE;
          }
        }
        return CL_FALSE;
      }
      /**
       * API to add new entry into the list
       */
      virtual ClRcT addChildObject(MgtObject *mgtObject, std::string &objectKey = *(std::string *)NULL)
      {
        ClRcT rc = CL_OK;
        assert(mgtObject);
        std::string *key = &objectKey;
        if(key == NULL)
        {
          key = &mgtObject->name;
        }
        children[*key] = mgtObject;
        return CL_OK;
      }

      virtual ClRcT addChildObject(MgtObject *mgtObject, const char* key)
      {
        ClRcT rc = CL_OK;
        assert(mgtObject);
        children[key] = mgtObject;
        return CL_OK;
      }

      /**
       * API to remove an entry from the list
       */
      virtual ClRcT removeChildObject(const std::string objectKey)
      {
        children.erase(objectKey);
      }
      /**
       * API to remove all entries from the list
       */
      void removeAllChildren()
      {
        children.clear();
      }
      /**
       * API to iterate thought objects in the list
       */
      MgtObject::Iterator begin(void)
      {
        MgtObject::Iterator ret;
        typename Map::iterator bgn = children.begin();
        typename Map::iterator end = children.end();
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
      /**
       * Shortcut to find an entry in the list
       */
      MgtObject* operator [](const std::string objectKey)
      {
        typename Map::iterator it = children.find(objectKey);
        if (it == children.end()) return nullptr;
        return it->second;
      }
      /**
       * API to get data of the list (called from netconf server)
       */
      virtual void toString(std::stringstream& xmlString)
      {
        typename Map::iterator i;
        typename Map::iterator end = children.end();
        for (i = children.begin(); i != end; i++)
        {
          MgtObject *entry = i->second;
          assert(entry);  // should be no NULL entries... erase don't nullptr them
          if (entry)
          {
            entry->toString(xmlString);
          }
        }
      }
      /**
       * API to get number of entries in the list
       */
      ClUint32T getEntrySize()
      {
        return (ClUint32T) children.size();
      }
      /**
       * API to set list data from netconf server (XML format)
       */
      virtual ClBoolT set(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
      {
        Map::iterator iter;
        std::string entryKey;

        xmlChar *valstr, *namestr;
        int ret, nodetyp, depth;
        ClBoolT isKey = CL_FALSE;
        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN);
        if (!strChildData)
        {
          return CL_FALSE;
        }
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
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
               snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
               if (depth == 0)
               {
                 strcpy(strChildData, strTemp);
               }
               else
               {
                 strcat(strChildData, strTemp);
                 if(keyList.compare(std::string((const char *)namestr)) == 0)
                 {
                   isKey = CL_TRUE;
                 }
               }
               break;
             case XML_ELEMENT_DECL:
               snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
               strcat(strChildData, strTemp);
               if(depth == 0)
               {
                 MgtObject *entry = children[entryKey];
                 if(entry != NULL)
                 {
                   if(entry->set(strChildData,strlen(strChildData),t) == CL_FALSE)
                   {
                     xmlFreeTextReader(reader);
                     free(strChildData);
                     return CL_FALSE;
                   }
                 }
               }
               break;
             case XML_TEXT_NODE:
               strcat(strChildData, (char *) valstr);
               if(isKey == CL_TRUE)
               {
                 isKey = CL_FALSE;
                 entryKey.assign(std::string((const char *)valstr));
               }
               break;
             default:
               break;
           }
           ret = xmlTextReaderRead(reader);
        } while(ret);

        return CL_TRUE;
      }
  };

};

#endif /* CLMGTLIST_HXX_ */

/** \} */

