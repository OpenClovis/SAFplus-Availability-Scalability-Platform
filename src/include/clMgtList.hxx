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
#include <clMgtBaseCommon.hxx>
#include "clLogIpi.hxx"
#include "clMgtObject.hxx"
#include "MgtFactory.hxx"

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
        typedef std::map<KEYTYPE, MgtObject*> Map;
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

        class HiddenFilterIterator:public MgtIteratorBase
        {
          public:
            std::string nameSpec;
            typename Map::iterator it;
            typename Map::iterator end;

            virtual bool next()
            {
              
              do
                {
                it++;                
                } while((it != end) && (!it->second->match(nameSpec)));

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
      std::string childXpath;
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
          if(curEntry->tag.compare(entry->tag) == 0)
          {
            return CL_TRUE;
          }
        }
#ifndef SAFplus7
        logDebug("MGT", "LIST", "Entry with name %s isn't exist",entry->tag.c_str());
#endif
        return CL_FALSE;
      }
      /**
       * API to add new entry into the list
       */
      virtual ClRcT addChildObject(MgtObject *mgtObject, KEYTYPE &objectKey = *(KEYTYPE *)nullptr)
      {
        ClRcT rc = CL_OK;
        assert(mgtObject);
        KEYTYPE *key = &objectKey;
        if(key == nullptr)
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
        if(mgtObject->parent == nullptr)
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
          mgtObject->parent = nullptr;
        }
        mgtObject->removeAllChildren();
        children.erase(objectKey);
        return CL_OK;
      }
      /**
       * API to delete entry from the list
       */
      virtual ClRcT deleteObj(const KEYTYPE &value)
      {
        if (nullptr != this->find(keyTypeToString(value)))
        {
          return this->removeChildObject(value);
        }
        return CL_ERR_NOT_EXIST;
      }
      /**
       * API to remove all entries from the list
       */
      void removeAllChildren()
      {
        for (typename Map::iterator it = children.begin(); it != children.end();)
        {
          if (nullptr != it->second)
          {
            it->second->removeAllChildren();
            if (it->second->isAllocated())
            {
              delete it->second;
              it->second = nullptr;
            }
          }
          it = children.erase(it);
        }
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

    //? iterate with selection criteria
    MgtObject::Iterator begin(const std::string& nameSpec)
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
          HiddenFilterIterator* h = new HiddenIterator();
          h->nameSpec = nameSpec;
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
    virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts=SerializeNoOptions)
      {
        typename Map::iterator iter;
        /* Name of this list */

        xmlString << '<' << tag;
        if (opts & MgtObject::SerializeNameAttribute)
          xmlString << " name=" << "\"" << getFullXpath(false) << "\"";
        if (opts & MgtObject::SerializePathAttribute)
          xmlString << " path=" << "\"" << getFullXpath(true) << "\"";
        xmlString << '>';

        MgtObject::SerializationOptions newopts = opts;
        if (opts & MgtObject::SerializeOnePath) newopts = (MgtObject::SerializationOptions) (newopts & ~MgtObject::SerializePathAttribute);

        if (depth) for (iter = children.begin(); iter != children.end(); iter++)
        {
          const KEYTYPE *k = &(iter->first);
          MgtObject *entry = iter->second;
          if (entry)
          {
              /*
               * Build fully tag with keys attribute
               * YumaNetconf parse does not support XML attribute
               * Example: <interface name="eth0" ipAddr="192.168.10.1">...</interface>
               * Just simple return:
               *     <interface>...</interface>
               */
              
            entry->toString(xmlString,depth-1,opts);
       
          }
        }
        xmlString << "</" << tag << '>';
      }

      MgtObject* lookUpMgtObject(const std::string & classType, const std::string &ref)
      {
        //std::string type = "P";
        //type.append((typeid(*this).name()));
        //if ( type == classType && this->tag == ref)
        if (this->tag == ref)
          {
            return this;
          }

        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
          {
            MgtObject *obj = iter->second;
            MgtObject *found = obj->lookUpMgtObject(classType, ref);
            if (found)
              return found;
          }
        return nullptr;
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
      virtual bool set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
      {
        keyMap::iterator iter;
        KEYTYPE entryKey;
        int ret, nodetyp, depth;

        xmlChar *valstr, *namestr;

        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        std::string strChildData;
        std::string lastnamestr;

        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, nullptr, nullptr, 0);
        if (!reader)
        {
#ifndef SAFplus7
          logError("MGT", "LIST", "Reader return null");
#endif
          return false;
        }

        /* Parse XM: */
        do
        {
           depth   = xmlTextReaderDepth(reader);
           nodetyp = xmlTextReaderNodeType(reader);
           namestr = (xmlChar *) xmlTextReaderConstName(reader);
           valstr  = (xmlChar *) xmlTextReaderValue(reader);
           switch (nodetyp)
           {
              /* Opening tag of a node */
              case XML_ELEMENT_NODE:
              {
                if(depth == 0)
                {
                   if(strcmp((const char *)namestr,this->tag.c_str()) != 0)
                   {
#ifndef SAFplus7
                     logError("MGT","LIST","The configuration [%s] isn't for this list [%s]",(const char *)namestr,this->tag.c_str());
#endif
                     return false;
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
                if(entry != nullptr)
                {
                  if(entry->set(strChildData.c_str(),strChildData.size(),t) == CL_FALSE)
                  {
#ifndef SAFplus7
                    logError("MGT", "LIST", "Setting for child failed");
#endif
                    xmlFreeTextReader(reader);
                    return false;
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

        return true;
      }

      //? Returns complete (from the root) XML Path of this list 
      std::string getFullXpath(bool includeParent = true)
      {
        std::string xpath;
        /* Parent X-Path will be add into the full xpath */
        if (parent != nullptr && includeParent) // this is the list parent
        {
          xpath = parent->getFullXpath();
        }
        xpath.append("/").append(this->tag);
        return xpath;
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
        std::string keypart = key.toXpath();

        /* Parent X-Path will be add into the full xpath */
        if (parent != nullptr && includeParent) // this is the list parent
        {
          parentXpath = parent->getFullXpath();
          if (parentXpath.length() > 0)
          {
            xpath.append(parentXpath);
          }
        }
        xpath.append("/").append(this->tag);
        /* Add key into xpath */
        /* ex: /ethernet/interfaces[@name="eth0" and @ipAddress="123"] */
        xpath.append(keypart);
        return xpath;
      }
      /**
       * Function to convert KEYTYPE to string
       */
      static std::string keyTypeToString(KEYTYPE key)
      {
        /*
         * GAS: convert to hash string to easy comparing
         */
        return key.str();
      }
      /**
       * Function to read from database
       */
      virtual ClRcT read(MgtDatabase *db=nullptr, std::string xpath = "")
      {
        ClRcT rc = CL_OK;

        if (!config)
          return rc;

        if ((db == nullptr) || (!db->isInitialized()))
          return rc;

        xpath.append("/");
        xpath.append(tag);

        std::vector<std::string> iters;
        db->iterate(xpath, iters);

        typedef std::map<std::string, keyMap> MultiKeyMap;
        MultiKeyMap multiKeyMap;
        std::vector<std::string>::iterator it = iters.begin();
        while( it != iters.end() )
        {
          if ((*it).find("[", xpath.length() + 1) != std::string::npos)
            continue;
          std::size_t found = (*it).find_last_of("/@");
          std::string dataXPath = (*it).substr(0, found - 1);
          std::string key = (*it).substr(found + 1, (*it).length() + 1 );
          if(keyList.find(key) != keyList.end())
            {
              std::string keyValue;
              db->getRecord(*it, keyValue);
              multiKeyMap[dataXPath][key] = keyValue;
            }
          it++;
        }

        for( MultiKeyMap::iterator it = multiKeyMap.begin(); it != multiKeyMap.end(); it++)
        {
          KEYTYPE *keyType = new KEYTYPE;
          keyType->build(it->second);

          MgtObject* object = MgtFactory::getInstance()->create(childXpath,"");
          if ( object )
          {
            addChildObject(object, *keyType);
            object->setChildObj(it->second);
            object->dataXPath = it->first;
          }
        }
        typename Map::iterator iter;
        for (iter = children.begin(); iter != children.end(); iter++)
        {
          MgtObject *obj = iter->second;
          rc = obj->read(db, xpath);
          if (CL_OK != rc)
          {
            // TODO log something
            //logInfo("MGT", "READ", "Load of some elements of [%s] failed with error [0x%x]", obj->tag.c_str(), rc);
          }
        }
        return rc;
      }

      ClRcT read(KEYTYPE key, MgtDatabase *db=nullptr, std::string xpath = "")
      {
        ClRcT rc = CL_OK;

        if (!config) return rc;

        MgtObject *obj = children[key];
        if(obj == nullptr)
        {
#ifndef SAFplus7
          logDebug("MGT","READ","Can't find children");
#endif
          return CL_ERR_INVALID_PARAMETER;
        }
        rc = obj->read(db, xpath);

        return rc;
      }
      /**
       * Function to write to database
       */
      virtual ClRcT write(MgtDatabase *db=nullptr, std::string xpath = "")
      {
        ClRcT rc = CL_OK;

        if (!config) return rc;

        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
        {
          MgtObject *obj = iter->second;
          rc = obj->write(db, xpath);
          if(CL_OK != rc)
            return rc;
        }
        return rc;
      }

    virtual ClRcT writeChanged(uint64_t firstBeat, uint64_t beat,SAFplus::MgtDatabase *db, std::string xpath)
      {
        ClRcT rc = CL_OK;

        if (!config) return rc;

        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
        {
          MgtObject *obj = iter->second;
          rc = obj->writeChanged(firstBeat,beat,db, xpath);
          if(CL_OK != rc)
            return rc;
        }
        return rc;
      }

      ClRcT write(KEYTYPE key, MgtDatabase *db=nullptr, std::string xpath = "")
      {
        ClRcT rc = CL_OK;

        if (!config) return rc;

        MgtObject *obj = children[key];
        if(obj == nullptr)
        {
#ifndef SAFplus7
          logDebug("MGT","READ","Can't find children");
#endif
          return CL_ERR_INVALID_PARAMETER;
        }
        rc = obj->write(db, xpath);

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
      typedef std::map<std::string, MgtObject*> Map;
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

      class HiddenFilterIterator:public MgtIteratorBase
        {
          public:
            std::string nameSpec;
            typename Map::iterator it;
            typename Map::iterator end;

            virtual bool next()
            {
              
              do
                {
                it++;                
                } while((it != end) && (!it->second->match(nameSpec)));

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
      std::string childXpath;
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
      virtual ~MgtList()
      {
        removeAllChildren();
      }
      /**
       * API to detect whether an entry is exist in the current list (based on its name)
       */
      ClBoolT isEntryExist(MgtObject* entry)
      {
        Map::iterator iter = children.begin();
        while(iter != children.end())
        {
          MgtObject* curEntry = iter->second;
          if(curEntry->tag.compare(entry->tag) == 0)
          {
#ifndef SAFplus7
            logDebug("MGT", "LIST", "Entry with name %s isn't exist",entry->tag.c_str());
#endif
            return CL_TRUE;
          }
        }
        return CL_FALSE;
      }
      /**
       * API to add new entry into the list
       */
      virtual ClRcT addChildObject(MgtObject *mgtObject, const std::string &objectKey = *(std::string *)nullptr)
      {
        ClRcT rc = CL_OK;
        assert(mgtObject);
        const std::string *key = &objectKey;
        if(key == nullptr)
        {
          key = &mgtObject->tag;
        }
        children[*key] = mgtObject;
#ifndef SAFplus7
        logDebug("MGT", "LIST", "Adding child object was successfully");
#endif
        //set tag for list item to display the item xpath with object name
        mgtObject->listTag.assign(this->tag);
        mgtObject->tag.assign(*key);

        if(mgtObject->parent == nullptr)
          mgtObject->parent = this;
        return rc;
      }
      /**
       * API to remove all entries from the list
       */
      void removeAllChildren()
      {
        for (typename Map::iterator it = children.begin(); it != children.end();)
        {
          if (nullptr != it->second)
          {
            it->second->removeAllChildren();
            if (it->second->isAllocated())
            {
              delete it->second;
              it->second = nullptr;
            }
          }
          it = children.erase(it);
        }
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

    //? iterate with selection criteria
    MgtObject::Iterator begin(const std::string& nameSpec)
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
          HiddenFilterIterator* h = new HiddenFilterIterator();
          h->nameSpec = nameSpec;
          h->it = bgn;
          h->end = end;
          while ((h->it != h->end) && (!h->it->second->match(nameSpec)))  // Move forward looking for the first match
            {
              h->it++;
            }
          if (h->it == h->end)
            {
                h->current.first = "";
                h->current.second = nullptr;
            }
          else
            {
            h->current.first = h->it->first;
            h->current.second = h->it->second;
            }
          ret.b  = h;
        }
        return ret;
      }

      /**
       * Shortcut to find an entry in the list
       */
      MgtObject* operator [](const std::string& objectKey)
      {
        typename Map::iterator it = children.find(objectKey);
        if (it == children.end()) return nullptr;
        return it->second;
      }

    virtual MgtObject* find(const std::string &name)
    {
      return (*this)[name];
    }

      //? Returns complete (from the root) XML Path of this list 
    std::string getFullXpath(bool includeParent = true)
      {
        std::string xpath;
        /* Parent X-Path will be add into the full xpath */
        if (parent != nullptr && includeParent) // this is the list parent
        {
          xpath = parent->getFullXpath();
        }
        xpath.append("/").append(this->tag);
        return xpath;
      }

      /**
       * API to get data of the list (called from netconf server)
       */
    virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth, SerializationOptions opts=SerializeNoOptions)
      {
        typename Map::iterator iter;
#if 0
        /* Name of this list */
        xmlString << '<' << tag;
        if (opts & MgtObject::SerializeNameAttribute)
          xmlString << " name=" << "\"" << getFullXpath(false) << "\"";
        if (opts & MgtObject::SerializePathAttribute)
          xmlString << " path=" << "\"" << getFullXpath(true) << "\"";
        xmlString << '>';

#endif
        MgtObject::SerializationOptions newopts = opts;
        if (opts & MgtObject::SerializeOnePath) newopts = (MgtObject::SerializationOptions) (newopts & ~MgtObject::SerializePathAttribute);

        // if (depth) Lists are "invisible" so depth is not reduced
        for (iter = children.begin(); iter != children.end(); iter++)
        {
          std::string k = iter->first;
          MgtObject *entry = iter->second;
          if (entry)
          {
             /*
              * Yuma parse does not support XML attribute
              * Example: <interface name="eth0" ipAddr="192.168.10.1">...</interface>
              * Just simple return:
              *     <interface>...</interface>
              */
            
            entry->toString(xmlString,depth, newopts); // Lists are "invisible" so depth is not reduced
          }
        }
#if 0
        xmlString << "</" << tag << '>';
#endif
      }

      MgtObject* lookUpMgtObject(const std::string & classType, const std::string &ref)
      {
        //std::string type = "P";
        //type.append((typeid(*this).name()));
        //if ( type == classType)
        if (1)
          { 
            if (this->tag == ref)
            {
              return this;
            }
          }
        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
          {
            MgtObject *obj = iter->second;
            MgtObject *found = obj->lookUpMgtObject(classType, ref);
            if (found)
            return found;
          }
        return nullptr;
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
      virtual bool set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
      {
        int ret, nodetyp, depth;

        xmlChar *valstr, *namestr;

        char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
        std::string strChildData;
        std::string lastnamestr;
        std::string keyValue;
#ifndef SAFplus7
        logDebug("MGT","LIST","Set data for list [%s]",(char *)pBuffer);
#endif
        /* Read XML from the buffer */
        xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, nullptr, nullptr, 0);
        if (!reader)
        {
#ifndef SAFplus7
          logError("MGT", "LIST", "Reader return null");
#endif
          return false;
        }

        /* Parse XM: */
        do
        {
           depth   = xmlTextReaderDepth(reader);
           nodetyp = xmlTextReaderNodeType(reader);
           namestr = (xmlChar *) xmlTextReaderConstName(reader);
           valstr  = (xmlChar *) xmlTextReaderValue(reader);
           switch (nodetyp)
           {
             /* Opening tag of a node */
             case XML_ELEMENT_NODE:
             {
               if(depth == 0)
               {
                  if(strcmp((const char *)namestr,this->tag.c_str()) != 0)
                  {
#ifndef SAFplus7
                    logError("MGT","LIST","The configuration [%s] isn't for this list [%s]",(const char *)namestr,this->tag.c_str());
#endif
                    return false;
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
                 return false;
               }
               MgtObject *entry = children[keyValue];
               if(entry != nullptr)
               {
                 if(entry->set(strChildData.c_str(),strChildData.size(),t) == CL_FALSE)
                 {
#ifndef SAFplus7
                   logError("MGT", "LIST", "Setting for child failed");
#endif
                   xmlFreeTextReader(reader);
                   return false;
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
        return true;
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
        keypart << "[@" << keyList << "=\"" << key <<"\"" << "]";
        /* Parent X-Path will be add into the full xpath */
        if (parent != nullptr && includeParent) // this is the list parent
        {
          parentXpath = parent->getFullXpath();
          if (parentXpath.length() > 0)
          {
            xpath.append(parentXpath);
          }
        }
        xpath.append("/").append(this->tag);
        xpath.append(keypart.str());
        return xpath;
      }

      virtual ClRcT read(MgtDatabase *db=nullptr, std::string xpath = "")
      {
        ClRcT rc = CL_OK;

        if (!config) return rc;

        if ((db == nullptr) || (!db->isInitialized()))
            return rc;

        xpath.append("/");
        xpath.append(tag);

        std::vector<std::string> iters;

        // Input xpath : /safplusAmf/ServiceUnit
        // Output iters: /safplusAmf/ServiceUnit[@name=su0], /safplusAmf/ServiceUnit[@name=su1] ...
        db->iterate(xpath, iters);

        for (std::vector<std::string>::iterator it = iters.begin() ; it != iters.end(); ++it)
          {
            // it = '/a/b[@key="su0"]'
            // xpath= '/a/b'
            if ((*it).find("[", xpath.length()) == std::string::npos )
                continue;

            // it = '/a/b[@key="su0"]/a'
            std::size_t posLastSlash = (*it).find_last_of("/");
            if (posLastSlash != std::string::npos && posLastSlash > xpath.length())
                continue;

            // *it      :'/a/b[@key="su0"]'
            // strKey   :[@key="su0"]
            // keyValue :su0
            std::string strKey = (*it).substr(xpath.length());
            std::size_t posEquals = strKey.find("=");
            std::string keyValue = strKey.substr(posEquals+2, strKey.length() - posEquals - 4);
            if (children.find(keyValue) != children.end())
            {
              //logDebug("MGMT","READ","object of [%s] was created, skip it", keyValue.c_str());
              continue;
            }

            dataXPath.assign(*it);

            MgtObject* object = MgtFactory::getInstance()->create(childXpath,keyValue);
            if (object)
              {
                addChildObject(object, keyValue);
                object->setChildObj(keyList, keyValue);
                object->dataXPath = dataXPath;
              }
          }

        typename Map::iterator iter;
        int count = 0;
        for(iter = children.begin(); iter != children.end(); iter++)
        {
          count++;
          MgtObject *obj = iter->second;
#ifdef MGTDBG
          logDebug("MGT", "READ", "read [%s]", obj->tag.c_str());
#endif
          rc = obj->read(db);
          if(CL_OK != rc)
            {
              // TODO log something
              //logInfo("MGT", "READ", "Load of some elements of [%s] failed with error [0x%x]", obj->tag.c_str(), rc);
            }
        }
//#ifdef MGTDBG
        logDebug("MGT", "READ", "read [%d] items in [%s]", count, xpath.c_str());
//#endif
        return rc;
      }

      virtual ClRcT write(MgtDatabase *db=nullptr, std::string xpath = "")
      {
          ClRcT rc = CL_OK;

          if (!config) return rc;

          typename Map::iterator iter;
          for(iter = children.begin(); iter != children.end(); iter++)
          {
            MgtObject *obj = iter->second;
            rc = obj->write(db, xpath);
            if(CL_OK != rc)
              return rc;
          }
          return rc;
      }

    virtual ClRcT writeChanged(uint64_t firstBeat, uint64_t beat,SAFplus::MgtDatabase *db, std::string xpath)
      {
        ClRcT rc = CL_OK;

        if (!config) return rc;

        typename Map::iterator iter;
        for(iter = children.begin(); iter != children.end(); iter++)
        {
          MgtObject *obj = iter->second;
          rc = obj->writeChanged(firstBeat,beat,db, xpath);
          if(CL_OK != rc)
            return rc;
        }
        return rc;
      }

     virtual void resolvePath(const char* path, std::vector<MgtObject*>* result)
       {
       if (path[0] == 0) // End of the path, this object is therefore a member
         {
          result->push_back(this);
          return;
         }
       if (strncmp(path,"./",2)==0) { this->resolvePath(path+2, result); return; }  // ./ refers to the current node
       if (strncmp(path,"../",3)==0) { this->parent->resolvePath(path+3, result); return; }  // ../ refers to the parent
       if (strncmp(path,"**/",3)==0) 
         {
#ifndef SAFplus7
           clDbgNotImplemented("DEEP search for the subsequent match");
#endif
         return; 
         }  

       std::string p(path);
       std::size_t idx = p.find("/");
       std::string childName;
       if (idx == std::string::npos) childName = p;
       else childName = p.substr(0,idx);
       if (childName.find_first_of("|") != std::string::npos)  // This means both, that is //root/foo|bar/child ->  //root/foo/child and //root/bar/child
         {
         std::vector<std::string> words;
         boost::split(words, childName, boost::is_any_of("|"), boost::token_compress_on);
         for (std::vector<std::string>::iterator i = words.begin(); i != words.end(); i++)
           {
             std::string tmp = *i;
             if (idx != std::string::npos) tmp.append(&path[idx]);
             resolvePath(tmp.c_str(),result);
           }
         }
       else if (childName.find_first_of("*[(])?") != std::string::npos)  // Its a complex lookup
         {  // Complex pattern lookup
           for (MgtObject::Iterator i= begin(childName);i!=end();i++)
             {
               MgtObject *child = i->second;
               if (idx == std::string::npos) result->push_back(child);
               else child->resolvePath(&path[idx+1], result);
             }
         }
       else  
         {  // Simple name lookup
           typename Map::iterator it = children.find(childName);
           if (it != children.end())
             {
               MgtObject *child = it->second;
               if (idx == std::string::npos) result->push_back(child);
               else child->resolvePath(&path[idx+1], result);
             }
         }
       }


      virtual MgtObject *findMgtObject(const std::string &xpath, std::size_t idx)
      {
        MgtObject *obj = nullptr;
        std::size_t nextIdx = xpath.find("/", idx + 1);

        if (nextIdx == std::string::npos)
          {
            std::string childName = xpath.substr(idx + 1, xpath.length() - idx -1);

            typename Map::iterator it = children.find(childName);
            if (it != children.end())
              obj = it->second;
          }
        else
          {
            std::string childName = xpath.substr(idx + 1, nextIdx - idx -1);

            typename Map::iterator it = children.find(childName);
            if (it != children.end())
              {
                MgtObject *child = it->second;
                obj = child->findMgtObject(xpath, nextIdx);
              }
          }

        return obj;
      }

      virtual ClRcT createObj(const std::string &value)
      {
        ClRcT ret = CL_OK;

        typename Map::iterator it = children.find(value);
        if (it != children.end())
          {
            ret = CL_ERR_ALREADY_EXIST;
            return ret;
          }

        MgtObject* object = MgtFactory::getInstance()->create(childXpath, value);

        // Build dataXpath to store into DB as format: /safplusAmf/Node[@name="node1"] and /safplusAmf/Node 
        if (object)
        {
          object->allocated = true;
          std::string xpath(getFullXpath(true));
          std::stringstream keypart;
          keypart << "[@" << keyList << "=\"" << value <<"\"" << "]";
          std::string dataXPath = "";
          dataXPath.append(xpath).append(keypart.str());

          addChildObject(object, value);
          object->setChildObj(keyList, value);
          object->dataXPath = dataXPath;

          MgtDatabase* db = getDb();
          if (db) // Update childs for list: i.e /safplusAmf/Node => "[@name='node0'], [@name='node1']"
          {
            // xapth: /safplusAmf/Component
            // keypart: [@name="Componentx"]
            std::string keyPath(xpath); ///safplusAmf/Component
            keyPath.append(keypart.str()); // /safplusAmf/Component[@name="Componentx"]
            db->setRecord(keyPath, "");
            logDebug("MGT","LIST", "Insert child node [%s]", keyPath.c_str());

            std::string pval;
            std::vector<std::string> childs;
            db->getRecord(xpath, pval, &childs);
            childs.push_back(keypart.str());
            db->setRecord(xpath, pval, &childs);
            logDebug("MGT","LIST", "Append child node [%s] into [%s]", keypart.str().c_str(), xpath.c_str());
          }

          if ((this->parent != nullptr)&&(db)) // Update childs for parent: i.e /safplusAmf => "Node[@name='node0'],Node[@name='node1']"
          {
            std::string parentDBKey = parent->getFullXpath();
            std::string childDBKey="";
            childDBKey.append(tag).append(keypart.str());

            // Update childs for parentDBKey
            std::string pval;
            std::vector<std::string> childs;
            db->getRecord(parentDBKey, pval, &childs);
            childs.push_back(childDBKey);
            db->setRecord(parentDBKey, pval, &childs);
            logDebug("MGT","LIST", "Append child node [%s] into [%s]", childDBKey.c_str(), parentDBKey.c_str());
          }
        }

        return ret;
      }

      virtual ClRcT deleteObj(const std::string &value)
      {
        ClRcT ret = CL_OK;

        typename Map::iterator it = children.find(value);
        if (it == children.end())
          {
            logDebug("MGT","LIST","entity [%s] doesn't exist in the map", value.c_str());
            ret = CL_ERR_DOESNT_EXIST;
            return ret;
          }

        MgtObject *child = it->second;
        if(nullptr == child) return ret;

        logDebug("MGT","LIST", "Deleting Object [%s]", child->dataXPath.c_str());

        std::string xpath(getFullXpath(true));
        std::stringstream keypart;
        keypart << "[@" << keyList << "=\"" << value <<"\"" << "]";

        MgtDatabase* db = getDb();
        if (db) /* Update childs for list: i.e /safplusAmf/Node => "[@name='node0'], [@name='node1']" */
        {
          std::string pval;
          std::vector<std::string> childs;
          logDebug("MGT","LIST","getting record of xpath [%s]", xpath.c_str());
          ret = db->getRecord(xpath, pval, &childs);
          if (ret == CL_OK)
          {
            std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), keypart.str());
            if (delIter != childs.end())
            {
              childs.erase(delIter);
              ret = db->setRecord(xpath, pval, &childs);
              logDebug("MGT","LIST", "Remove child node [%s] of [%s]", keypart.str().c_str(), xpath.c_str());
            }
            else
              logDebug("MGT","LIST", "Removing child node [%s] of [%s] failed", keypart.str().c_str(), xpath.c_str());
          }
          else
            logDebug("MGT","LIST","get record of xpath [%s] failed 0x%x", xpath.c_str(), ret);
        }
        else
          logDebug("MGT","LIST", "db object is null");
#if 0
        if (db &&(this->parent != nullptr)) /* Update childs for parent: i.e /safplusAmf => "Node[@name='node0'],Node[@name='node1']" */
        {
          std::string parentDBKey = parent->getFullXpath();
          std::string childDBKey="";
          childDBKey.append(tag).append(keypart.str());

          /* Update childs for parentDBKey */
          std::string pval;
          std::vector<std::string> childs;
          logDebug("MGT","LIST","getting record of xpath [%s]", parentDBKey.c_str());
          ret = db->getRecord(parentDBKey, pval, &childs);
          if (ret == CL_OK)
          {
            std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), childDBKey);
            if (delIter != childs.end())
            {
              childs.erase(delIter);
              ret = db->setRecord(parentDBKey, pval, &childs);
              logDebug("MGT","LIST", "Remove child node [%s] of [%s]", childDBKey.c_str(), parentDBKey.c_str());
            }
            else
              logDebug("MGT","LIST", "Removing child node [%s] of [%s] failed", childDBKey.c_str(), parentDBKey.c_str());
          }
          else
            logDebug("MGT","LIST","get record of xpath [%s] failed 0x%x", parentDBKey.c_str(), ret);
        }
        else
          logDebug("MGT","LIST", "db object is null");
#endif

        /* TODO: Remove the record out of database for its childs */

        /* Free-ed */
        child->removeAllChildren();
        if (child->isAllocated())
        {
          delete child;
          child = nullptr;
        }
        children.erase(value);
        return ret;
      }
      //  deleteObj("/safplusAmf/ComponentServiceInstance", csi.name(), "data", "val", "testKey")
      ClRcT deleteObj(const char* xpath, const std::string& entityName, const char* tagName, const char* tagName2, const std::string& value)
      {
        ClRcT ret = CL_OK;

        typename Map::iterator it = children.find(value);
        if (it == children.end())
          {
            logDebug("MGT","LIST","entity [%s] doesn't exist in the map", value.c_str());
            ret = CL_ERR_DOESNT_EXIST;
            return ret;
          }

        MgtObject *child = it->second;
        if(nullptr == child) return ret;

        logDebug("MGT","LIST", "Deleting Object [%s]", child->dataXPath.c_str());        
        std::string strXpath(xpath); // /safplusAmf/ComponentServiceInstance
        strXpath.append("[@name=\""); // /safplusAmf/ComponentServiceInstance[@name="
        strXpath.append(entityName); // /safplusAmf/ComponentServiceInstance[@name="csi
        strXpath.append("\"]"); // /safplusAmf/ComponentServiceInstance[@name="csi"]

        MgtDatabase* db = getDb();
        if (db) /* Update childs for list: i.e /safplusAmf/ComponentServiceInstance[@name="csi"]/data => "[@name='testKey'], [@name='testKey2']" */
        {
          std::stringstream keypart;
          keypart << "[@" << keyList << "=\"" << value <<"\"" << "]"; //[@name="testkey"]
          std::string strXpath1 = strXpath; // /safplusAmf/ComponentServiceInstance[@name="csi"]
          strXpath1.append(tagName); // /safplusAmf/ComponentServiceInstance[@name="csi"]data
          strXpath1.insert(strXpath.length(),"/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data
          std::string pval;
          std::vector<std::string> childs;
          logDebug("MGT","LIST","getting record of xpath [%s]", strXpath1.c_str());
          ret = db->getRecord(strXpath1, pval, &childs);
          if (ret == CL_OK)
          {
            std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), keypart.str());
            if (delIter != childs.end())
            {
              childs.erase(delIter);
              if (childs.size()>0)
              {
                 if ((ret = db->setRecord(strXpath1, pval, &childs))!=CL_OK) // /safplusAmf/ComponentServiceInstance[@name="csi"]/data --> val [] child []
                 {
                    logError("MGT","LIST", "set child node of [%s] num child [%d] failed with rc [0x%x]", strXpath1.c_str(), (int)childs.size(), ret);
                    return ret;
                 }
              }
              else
              {
                 if ((ret = db->deleteRecord(strXpath1))!=CL_OK)
                 {
                    logError("MGT","LIST", "delete child node [%s] of [%s] with rc [0x%x]", keypart.str().c_str(), strXpath1.c_str(), ret);
                    return ret;
                 }
              }
            }
            else
              logError("MGT","LIST", "Removing child node [%s] of [%s] failed", keypart.str().c_str(), strXpath1.c_str());
          }
          else
            logError("MGT","LIST","get record of xpath [%s] failed 0x%x", strXpath1.c_str(), ret);
        }
        else
          logCritical("MGT","LIST", "db object is null");
        
#if 0
        if (db &&(this->parent != nullptr)) /* Update childs for parent: i.e /safplusAmf/ComponentServiceInstance[@name='csi'] => "data[@name='testKey'], data[@name='testKey2']" */
        {
          std::stringstream keypart;
          keypart << tagName << "[@" << keyList << "=\"" << value <<"\"" << "]"; //[@name="testkey"]
          std::string strXpath1 = strXpath; // /safplusAmf/ComponentServiceInstance[@name="csi"]
          std::string pval;
          std::vector<std::string> childs;
          logDebug("MGT","LIST","getting record of xpath [%s]", strXpath1.c_str());
          ret = db->getRecord(strXpath1, pval, &childs);
          if (ret == CL_OK)
          {
            std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), keypart.str());
            if (delIter != childs.end())
            {
              childs.erase(delIter);
              ret = db->setRecord(strXpath1, pval, &childs);
              logDebug("MGT","LIST", "Remove child node [%s] of [%s]", keypart.str().c_str(), strXpath1.c_str());
            }
            else
              logDebug("MGT","LIST", "Removing child node [%s] of [%s] failed", keypart.str().c_str(), strXpath1.c_str());
          }
          else
            logDebug("MGT","LIST","get record of xpath [%s] failed 0x%x", strXpath1.c_str(), ret);
        }
#endif

        if (db &&(this->parent != nullptr)) /* Update childs for parent: i.e /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"] => "val" */
        {
          std::stringstream keypart;
          keypart << tagName << "[@" << keyList << "=\"" << value <<"\"" << "]";
          std::string strXpath1 = strXpath; // /safplusAmf/ComponentServiceInstance[@name="csi"]         
          strXpath1.append(keypart.str()); // /safplusAmf/ComponentServiceInstance[@name="csi"]data[@name="testKey"]
          strXpath1.insert(strXpath.length(),"/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]
          std::string pval;
          std::vector<std::string> childs;
          logDebug("MGT","LIST","getting record of xpath [%s]", strXpath1.c_str());
          ret = db->getRecord(strXpath1, pval, &childs);
          if (ret == CL_OK)
          {
            std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), std::string(tagName2));
            if (delIter != childs.end())
            {
              //childs.erase(delIter);
              //ret = db->setRecord(strXpath1, pval, &childs);
              if ((ret = db->deleteRecord(strXpath1))!= CL_OK)
              {
                 logError("MGT","LIST", "delete xpath [%s] failed with rc [0x%x]", strXpath1.c_str(), ret);
                 return ret;
              }
              
              std::string key(strXpath1); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]
              key.append("/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]/
              key.append(keyList); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]/name
              ret = db->deleteRecord(key);
              logDebug("MGT","LIST", "delete xpath [%s] returns rc [0x%x]", key.c_str(), ret);
 
              strXpath1.append("/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]/
              strXpath1.append(tagName2); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]/val
              if ((ret = db->deleteRecord(strXpath1)) != CL_OK)
              {
                 logError("MGT","LIST", "delete xpath [%s] failed with rc [0x%x]", strXpath1.c_str(), ret);
                 return ret;
              }
            }
            else
              logError("MGT","LIST", "Removing child node [%s] of [%s] failed", tagName2, strXpath1.c_str());
          }
          else
            logError("MGT","LIST","get record of xpath [%s] failed 0x%x", strXpath1.c_str(), ret);
        }        
        if (ret != CL_OK) return ret;
        /* Free-ed */
        child->removeAllChildren();
        if (child->isAllocated())
        {
          delete child;
          child = nullptr;
        }
        children.erase(value);
        return ret;
      }
  };

};

#endif /* CLMGTLIST_HXX_ */

/** \} */

