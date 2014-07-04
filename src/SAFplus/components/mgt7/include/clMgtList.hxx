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
                logDebug("MGT", "LIST", "Reached end of the list");
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
        logDebug("MGT", "LIST", "Entry with name %s isn't exist",entry->name.c_str());
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
          logError("MGT", "LIST", "Key for child object is not defined");
          return CL_ERR_NULL_POINTER;
        }
        children[*key] = mgtObject;
        logDebug("MGT", "LIST", "Adding child object was successfully");
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
        if (it == children.end())
        {
          logError("MGT", "LIST", "Can't find the object with given key");
          return nullptr;
        }
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
        int ret, nodetyp, depth;

        xmlChar *valstr, *namestr, *attrval;
        xmlNodePtr node;
        xmlAttr* attr;

        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN); /* Store the XML for child */
        if (!strChildData)
        {
          logError("MGT", "LIST", "Can't allocate buffer for child data");
          return CL_FALSE;
        }

        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
        if (!reader)
        {
          logError("MGT", "LIST", "Reader return null");
          free(strChildData);
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
               switch(depth)
               {
                 case 0: /* Open of list container */
                 {
                   /* Verify if configuration is belong to this list by list name */
                   if(strcmp((const char *)namestr,this->name.c_str()) != 0)
                   {
                     logError("The configuration [%s] isn't for this list [%s]",(const char *)namestr,this->name.c_str());
                     free(strChildData);
                     return CL_FALSE;
                   }
                   break;
                 }
                 case 1: /* Open of list item */
                 {
                   snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s", namestr);
                   strcat(strChildData, strTemp);
                   attr = node->properties;
                   while(attr && attr->name && attr->children)
                   {
                     attrval = xmlNodeListGetString(node->doc, attr->children, 1);
                     iter = keyList.find(std::string((const char *)attr->name));
                     if(iter != keyList.end())
                     {
                       iter->second.assign(std::string((const char *)attrval));
                     }
                     snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, " %s=\"%s\"", attr->name,attrval);
                     strcat(strChildData, strTemp);
                     xmlFree(attrval);
                     attr = attr->next;
                    }
                    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, ">", namestr);
                    strcat(strChildData, strTemp);
                    break;
                 }
                 default: /* Open of other nodes, just copy data */
                 {
                   snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
                   strcpy(strChildData, strTemp);
                   break;
                 }
               }
               break;
             }
             /* Closing tag of a node*/
             case XML_ELEMENT_DECL:
             {
               switch(depth)
               {
                 case 0: /* Close of list container */
                 {
                   break;
                 }
                 case 1: /* Close of list item, should forward configurations to object */
                 {
                   snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
                   strcat(strChildData, strTemp);
                   /* Build the entryKey from name/val of set keys */
                   entryKey.build(keyList);
                   MgtObject *entry = children[entryKey];
                   if(entry != NULL)
                   {
                     if(entry->set(strChildData,strlen(strChildData),t) == CL_FALSE)
                     {
                       logError("MGT", "LIST", "Setting for child failed");
                       xmlFreeTextReader(reader);
                       free(strChildData);
                       return CL_FALSE;
                     }
                   }
                   /* Prepare strChildData for the new entry */
                   memset(strChildData,0,MGT_MAX_DATA_LEN);
                   break;
                 }
                 default:
                 {
                   break;
                 }
               }
               break;
             }
             /* Text value of node */
             case XML_TEXT_NODE:
             {
               strcat(strChildData, (char *) valstr);
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

        /* Safely free the allocated memory */
        if(strChildData)
        {
          free(strChildData);
        }
        return CL_TRUE;
      }
      /**
       * Provide full X-Path of this list *
       */
      std::string getFullXpath(KEYTYPE key)
      {
        typename Map::iterator iter = children.find(key);
        std::string xpath = "",parentXpath,childXpath;
        /* Check if expected entry is found */
        if(iter == children.end())
        {
          logError("MGT","XPT","Can't find object which belong to key");
          return xpath;
        }
        /* Parent X-Path will be add into the full xpath */
        if (parent != NULL) // this is the list parent
        {
          parentXpath = parent->getFullXpath();
          if (parentXpath.length() > 0)
          {
            xpath = parentXpath.append("/").append(this->name);
          }
          else
          {
            xpath.append("/").append(this->name);
          }
        }
        else
        {
          xpath.append("/").append(this->name);
        }
        /* Append entry X-Path */
        childXpath = iter->second->getFullXpath();
        if(childXpath.length() > 0)
        {
          xpath.append(childXpath);
        }
        return xpath;
      }
      /**
       * Function to convert KEYTYPE to string
       */
      static std::string keyTypeToString(KEYTYPE key)
      {
        return key.str();
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
              logDebug("MGT", "LIST", "Reached end of the list");
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
            logDebug("MGT", "LIST", "Entry with name %s isn't exist",entry->name.c_str());
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
        logDebug("MGT", "LIST", "Adding child object was successfully");
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
        int ret, nodetyp, depth;

        xmlChar *valstr, *namestr, *attrval;
        xmlNodePtr node;
        xmlAttr* attr;

        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        char *strChildData = (char *) malloc(MGT_MAX_DATA_LEN); /* Store the XML for child */
        if (!strChildData)
        {
          logError("MGT", "LIST", "Can't allocate buffer for child data");
          return CL_FALSE;
        }

        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
        if (!reader)
        {
          logError("MGT", "LIST", "Reader return null");
          free(strChildData);
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
               switch(depth)
               {
                 case 0: /* Open of list container */
                 {
                   /* Verify if configuration is belong to this list by list name */
                   if(strcmp((const char *)namestr,this->name.c_str()) != 0)
                   {
                     logError("The configuration [%s] isn't for this list [%s]",(const char *)namestr,this->name.c_str());
                     free(strChildData);
                     return CL_FALSE;
                   }
                   break;
                 }
                 case 1: /* Open of list item */
                 {
                   snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s", namestr);
                   strcat(strChildData, strTemp);
                   attr = node->properties;
                   while(attr && attr->name && attr->children)
                   {
                     attrval = xmlNodeListGetString(node->doc, attr->children, 1);
                     if(strcmp(keyList.c_str(),(const char *)attr->name) == 0)
                     {
                       keyList.assign(std::string((const char *)attrval));
                     }
                     snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, " %s=\"%s\"", attr->name,attrval);
                     strcat(strChildData, strTemp);
                     xmlFree(attrval);
                     attr = attr->next;
                    }
                    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, ">", namestr);
                    strcat(strChildData, strTemp);
                    break;
                 }
                 default: /* Open of other nodes, just copy data */
                 {
                   snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
                   strcpy(strChildData, strTemp);
                   break;
                 }
               }
               break;
             }
             /* Closing tag of a node*/
             case XML_ELEMENT_DECL:
             {
               switch(depth)
               {
                 case 0: /* Close of list container */
                 {
                   break;
                 }
                 case 1: /* Close of list item, should forward configurations to object */
                 {
                   snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
                   strcat(strChildData, strTemp);
                   /* Get the list item based on key */
                   MgtObject *entry = children[keyList];
                   if(entry != NULL)
                   {
                     if(entry->set(strChildData,strlen(strChildData),t) == CL_FALSE)
                     {
                       logError("MGT", "LIST", "Setting for child failed");
                       xmlFreeTextReader(reader);
                       free(strChildData);
                       return CL_FALSE;
                     }
                   }
                   /* Prepare strChildData for the new entry */
                   memset(strChildData,0,MGT_MAX_DATA_LEN);
                   keyList.assign("");
                   break;
                 }
                 default:
                 {
                   break;
                 }
               }
               break;
             }
             /* Text value of node */
             case XML_TEXT_NODE:
             {
               strcat(strChildData, (char *) valstr);
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

        /* Safely free the allocated memory */
        if(strChildData)
        {
          free(strChildData);
        }
        return CL_TRUE;
      }
      /**
       * Provide full X-Path of this list *
       */
      std::string getFullXpath(std::string key)
      {
        typename Map::iterator iter = children.find(key);
        std::string xpath = "",parentXpath,childXpath;
        /* Check if expected entry is found */
        if(iter == children.end())
        {
          logError("MGT","XPT","Can't find object which belong to key");
          return xpath;
        }
        /* Parent X-Path will be add into the full xpath */
        if (parent != NULL) // this is the list parent
        {
          parentXpath = parent->getFullXpath();
          if (parentXpath.length() > 0)
          {
            xpath = parentXpath.append("/").append(this->name);
          }
          else
          {
            xpath.append("/").append(this->name);
          }
        }
        else
        {
          xpath.append("/").append(this->name);
        }
        /* Append entry X-Path */
        childXpath = iter->second->getFullXpath();
        if(childXpath.length() > 0)
        {
          xpath.append(childXpath);
        }
        return xpath;
      }
  };

};

#endif /* CLMGTLIST_HXX_ */

/** \} */

