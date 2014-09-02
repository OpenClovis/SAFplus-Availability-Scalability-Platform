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
#ifndef SAFplus7
                logDebug("MGT", "LIST", "Reached end of the list");
#endif
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
#ifndef SAFplus7
        logDebug("MGT", "LIST", "Entry with name %s isn't exist",entry->name.c_str());
#endif
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
#ifndef SAFplus7
          logError("MGT", "LIST", "Key for child object is not defined");
#endif
          return CL_ERR_NULL_POINTER;
        }
        children[*key] = mgtObject;
#ifndef SAFplus7
        logDebug("MGT", "LIST", "Adding child object was successfully");
#endif
        if(mgtObject->parent == NULL)
          mgtObject->parent = this;
        return CL_OK;
      }
      /**
       * API to remove entry from the list
       */
      virtual ClRcT removeChildObject(const KEYTYPE objectKey)
      {
        MgtObject *mgtObject = children[objectKey];
        if(mgtObject && mgtObject->parent == this)
        {
          mgtObject->parent = NULL;
        }
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
        if (it == children.end())
        {
#ifndef SAFplus7
          logError("MGT", "LIST", "Can't find the object with given key");
#endif
          return nullptr;
        }
        return it->second;
      }
      /**
       * API to get data of the list (called from netconf server)
       */
      virtual void toString(std::stringstream& xmlString)
      {
        typename Map::iterator iter;
        /* Name of this list */
        for (iter = children.begin(); iter != children.end(); iter++)
        {
          const KEYTYPE *k = &(iter->first);
          MgtObject *entry = iter->second;
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
      virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
      {
        keyMap::iterator iter;
        KEYTYPE entryKey;
        int ret, nodetyp, depth;

        xmlChar *valstr, *namestr, *attrval;
        xmlNodePtr node;
        xmlAttr* attr;

        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        std::string strChildData;
        std::string lastnamestr;

        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
        if (!reader)
        {
#ifndef SAFplus7
          logError("MGT", "LIST", "Reader return null");
#endif
          return CL_FALSE;
        }

        /* Parse XM: */
        do
        {
           depth   = xmlTextReaderDepth(reader);
           nodetyp = xmlTextReaderNodeType(reader);
           namestr = (xmlChar *) xmlTextReaderConstName(reader);
           valstr  = (xmlChar *) xmlTextReaderValue(reader);
           node    = xmlTextReaderCurrentNode(reader);
           switch (nodetyp)
           {
              /* Opening tag of a node */
              case XML_ELEMENT_NODE:
              {
                if(depth == 0)
                {
                   if(strcmp((const char *)namestr,this->name.c_str()) != 0)
                   {
#ifndef SAFplus7
                     logError("MGT","LIST","The configuration [%s] isn't for this list [%s]",(const char *)namestr,this->name.c_str());
#endif
                     return CL_FALSE;
                   }
                }
                snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
                strChildData.append(strTemp);
                lastnamestr.assign((char *) namestr);
                break;
              }
               /* Closing tag of a node*/
              case XML_ELEMENT_DECL:
              {
                snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
                strChildData.append((char *)strTemp);
                if(depth != 0)
                {
                  break;
                }
                /* Forward configuration data to child objects */
                /* keyList should be assign when key was found */
                entryKey.build(keyList);
                MgtObject *entry = children[entryKey];
                if(entry != NULL)
                {
                  if(entry->set(strChildData.c_str(),strChildData.size(),t) == CL_FALSE)
                  {
#ifndef SAFplus7
                    logError("MGT", "LIST", "Setting for child failed");
#endif
                    xmlFreeTextReader(reader);
                    return CL_FALSE;
                  }
                }
                else
                {
#ifndef SAFplus7
                  logError("MGT", "LIST", "Can't find entry for key %s",entryKey.str().c_str());
#endif
                }
                /* Reset old data for new list entry */
                lastnamestr.assign("");
                strChildData.assign("");
                break;
              }
              /* Text value of node */
              case XML_TEXT_NODE:
              {
                strChildData.append((char *)valstr);
                iter = keyList.find(lastnamestr);
                if(iter != keyList.end())
                {
                  iter->second.assign((char *)valstr);
                }
                break;
              }
               /* Other type: don't care */
              default:
              {
                break;
              }
           }
           ret = xmlTextReaderRead(reader);
        } while(ret);

        return CL_TRUE;
      }
      /**
       * Provide full X-Path of this list *
       */
      std::string getFullXpath(KEYTYPE key,bool includeParent = true)
      {
        typename Map::iterator iter = children.find(key);
        std::string xpath = "",parentXpath;
        /* Check if expected entry is found */
        if(iter == children.end())
        {
#ifndef SAFplus7
          logError("MGT","XPT","Can't find object which belong to key");
#endif
          return xpath;
        }
        /* Build key */
        std::string keypart = key.toXmlString();

        /* Parent X-Path will be add into the full xpath */
        if (parent != NULL && includeParent) // this is the list parent
        {
          parentXpath = parent->getFullXpath();
          if (parentXpath.length() > 0)
          {
            xpath.append(parentXpath);
          }
        }
        xpath.append("/").append(this->name);
        /* Add key into xpath */
        /* ex: /ethernet/interfaces[name=eth0,ipAddress=123] */
        xpath.append(keypart);
        return xpath;
      }
      /**
       * Function to convert KEYTYPE to string
       */
      static std::string keyTypeToString(KEYTYPE key)
      {
        return key.str();
      }
      /**
       * Function to read from database
       */
      virtual ClRcT read(ClMgtDatabase *db=NULL)
      {
        ClRcT rc = CL_OK;
        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
        {
          MgtObject *obj = iter->second;
          rc = obj->read();
          if(CL_OK != rc)
            return rc;
        }
        return rc;
      }
      ClRcT read(KEYTYPE key,ClMgtDatabase *db=NULL)
      {
        ClRcT rc = CL_OK;
        MgtObject *obj = children[key];
        if(obj == NULL)
        {
#ifndef SAFplus7
          logDebug("MGT","READ","Can't find children");
#endif
          return CL_ERR_INVALID_PARAMETER;
        }
        rc = obj->read(getFullXpath(key));

        return rc;
      }
      /**
       * Function to write to database
       */
      virtual ClRcT write(ClMgtDatabase *db=NULL)
      {
        ClRcT rc = CL_OK;
        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
        {
          MgtObject *obj = iter->second;
          rc = obj->write();
          if(CL_OK != rc)
            return rc;
        }
        return rc;
      }
      ClRcT write(KEYTYPE key,ClMgtDatabase *db=NULL)
      {
        ClRcT rc = CL_OK;
        MgtObject *obj = children[key];
        if(obj == NULL)
        {
#ifndef SAFplus7
          logDebug("MGT","READ","Can't find children");
#endif
          return CL_ERR_INVALID_PARAMETER;
        }
        rc = obj->write(getFullXpath(key,false),db);

        return rc;
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
#ifndef SAFplus7
              logDebug("MGT", "LIST", "Reached end of the list");
#endif
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
#ifndef SAFplus7
            logDebug("MGT", "LIST", "Entry with name %s isn't exist",entry->name.c_str());
#endif
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
#ifndef SAFplus7
        logDebug("MGT", "LIST", "Adding child object was successfully");
#endif
        if(mgtObject->parent == NULL)
          mgtObject->parent = this;
        return CL_OK;
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
        typename Map::iterator iter;
        /* Name of this list */
        for (iter = children.begin(); iter != children.end(); iter++)
        {
          std::string k = iter->first;
          MgtObject *entry = iter->second;
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
      virtual ClBoolT set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
      {
        int ret, nodetyp, depth;

        xmlChar *valstr, *namestr, *attrval;
        xmlNodePtr node;
        xmlAttr* attr;

        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        std::string strChildData;
        std::string lastnamestr;
        std::string keyValue;
#ifndef SAFplus7
        logDebug("MGT","LIST","Set data for list [%s]",(char *)pBuffer);
#endif
        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
        if (!reader)
        {
#ifndef SAFplus7
          logError("MGT", "LIST", "Reader return null");
#endif
          return CL_FALSE;
        }

        /* Parse XM: */
        do
        {
           depth   = xmlTextReaderDepth(reader);
           nodetyp = xmlTextReaderNodeType(reader);
           namestr = (xmlChar *) xmlTextReaderConstName(reader);
           valstr  = (xmlChar *) xmlTextReaderValue(reader);
           node    = xmlTextReaderCurrentNode(reader);
           switch (nodetyp)
           {
             /* Opening tag of a node */
             case XML_ELEMENT_NODE:
             {
               if(depth == 0)
               {
                  if(strcmp((const char *)namestr,this->name.c_str()) != 0)
                  {
#ifndef SAFplus7
                    logError("MGT","LIST","The configuration [%s] isn't for this list [%s]",(const char *)namestr,this->name.c_str());
#endif
                    return CL_FALSE;
                  }
               }
               snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
               strChildData.append(strTemp);
               lastnamestr.assign((char *) namestr);
               break;
             }
             /* Closing tag of a node*/
             case XML_ELEMENT_DECL:
             {
               snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
               strChildData.append((char *)strTemp);
               if(depth != 0)
               {
                 break;
               }
               /* Forward configuration data to child objects */
               /* keyList should be assign when key was found */
               if(keyValue.size() == 0)
               {
#ifndef SAFplus7
                 logError("MGT","LIST","The configuration had error, no key found");
#endif
                 return CL_FALSE;
               }
               MgtObject *entry = children[keyValue];
               if(entry != NULL)
               {
                 if(entry->set(strChildData.c_str(),strChildData.size(),t) == CL_FALSE)
                 {
#ifndef SAFplus7
                   logError("MGT", "LIST", "Setting for child failed");
#endif
                   xmlFreeTextReader(reader);
                   return CL_FALSE;
                 }
               }
               /* Reset old data for new list entry */
               keyValue.assign("");
               lastnamestr.assign("");
               strChildData.assign("");
               break;
             }
             /* Text value of node */
             case XML_TEXT_NODE:
             {
               strChildData.append((char *)valstr);
               if(strcmp(keyList.c_str(),lastnamestr.c_str()) == 0)
               {
                 keyValue.assign((char *)valstr);
               }
               break;
             }
             /* Other type: don't care */
             default:
             {
               break;
             }
           }
           ret = xmlTextReaderRead(reader);
        } while(ret);

        /* Parent object (container) will have responsibility for commit or abort the transition */
        return CL_TRUE;
      }
      /**
       * Provide full X-Path of this list *
       */
      std::string getFullXpath(std::string key,bool includeParent = true)
      {
        typename Map::iterator iter = children.find(key);
        std::string xpath = "",parentXpath;
        /* Check if expected entry is found */
        if(iter == children.end())
        {
#ifndef SAFplus7
          logError("MGT","XPT","Can't find object which belong to key");
#endif
          return xpath;
        }
        std::stringstream keypart;
        keypart << "[" << keyList << "=" << key << "]";
        /* Parent X-Path will be add into the full xpath */
        if (parent != NULL && includeParent) // this is the list parent
        {
          parentXpath = parent->getFullXpath();
          if (parentXpath.length() > 0)
          {
            xpath.append(parentXpath);
          }
        }
        xpath.append("/").append(this->name);
        xpath.append(keypart.str());
        return xpath;
      }
  };

};

#endif /* CLMGTLIST_HXX_ */

/** \} */

