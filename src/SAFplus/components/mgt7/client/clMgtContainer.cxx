

extern "C"
  {
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
  } /* end extern 'C' */

#include <clMgtContainer.hxx>

using namespace std;

namespace SAFplus
  {


  MgtContainer::~MgtContainer()
    {
    }


  MgtObject* MgtContainer::deepFind(const std::string &s)
    {
    MgtObjectMap::iterator it;
    MgtObjectMap::iterator end = children.end();

    for (it = children.begin();it != end;it++)
      {
      MgtObject* obj = (MgtObject*) it->second;
      if (it->first == s) return obj;
      obj = obj->deepFind(s);
      if (obj) return obj;
      }  
    }


  MgtObject::Iterator MgtContainer::begin(void)
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

  bool MgtContainer::HiddenIterator::next()
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

  void MgtContainer::HiddenIterator::del()
    {
    delete this;
    }

  MgtObject* MgtContainer::deepMatch(const std::string &s)
    {
    MgtObjectMap::iterator it;
    MgtObjectMap::iterator end = children.end();

    for (it = children.begin();it != end;it++)
      {
      MgtObject* ret;
      MgtObject* obj = (MgtObject*) it->second;
      if (match(it->first,s)) return obj;            
      ret = obj->deepMatch(s);
      if (ret) return ret;
      }  
    }

  MgtObject* MgtContainer::find(const std::string &s)
    {
    MgtObjectMap::iterator it = children.find(s);
    if (it != children.end())
      {
      MgtObject* obj = (MgtObject*) it->second;
      return obj;
      }
    return NULL;
    }

  MgtObject::Iterator MgtContainer::multiFind(const std::string &nameSpec)
    {
    clDbgNotImplemented("multiFind");
    MgtObject::Iterator ret; 
    return ret;
    }

  MgtObject::Iterator MgtContainer::multiMatch(const std::string &nameSpec)
    {
    clDbgNotImplemented("");
    MgtObject::Iterator ret; 
    return ret;

    }

  ClRcT MgtContainer::removeChildObject(const std::string& objectName)
  {
    ClRcT rc = CL_OK;

    children.erase(objectName);

    return rc;
  }

  ClRcT MgtContainer::addChildObject(MgtObject *mgtObject, const char* objectName)
    {
    std::string name(objectName);
    return addChildObject(mgtObject,name);
    }

  ClRcT MgtContainer::addChildObject(MgtObject *mgtObject, const std::string& objectName)
  {
    ClRcT rc = CL_OK;
    assert(mgtObject);

    const std::string* name = &objectName;
    if (name == nullptr) name = &mgtObject->tag;

    // The first place you hook it in is the "main" one, the rest are sym links.
    if (!mgtObject->parent) mgtObject->parent = this;
    children[*name] = mgtObject;

    return rc;
  }

  void MgtContainer::toString(std::stringstream& xmlString)
    {
    MgtObjectMap::iterator it;
    map<string, vector<MgtObject*>* >::iterator mapIndex;

    //GAS: TAG already build at MgtList, hardcode to ignore
    if (!parent || !strstr(typeid(*parent).name(), "SAFplus7MgtList" ))
      {
        xmlString << '<' << tag << '>';
      }
    for (it = children.begin(); it != children.end(); ++it)
      {
        MgtObject *child = it->second;
        child->toString(xmlString);
      }
      if (!parent || !strstr(typeid(*parent).name(), "SAFplus7MgtList" ))
        {
          xmlString << "</" << tag << '>';
        }
    }

  ClRcT MgtContainer::write(MgtDatabase *db)
  {
    ClRcT rc = CL_OK;
    MgtObjectMap::iterator it;
    for (it = children.begin(); it != children.end(); ++it)
    {
      MgtObject* child = it->second;
      rc = child->write();
      if(CL_OK != rc)
        return rc;
    }
    return rc;
  }

  ClRcT MgtContainer::read(MgtDatabase *db)
  {
    ClRcT rc = CL_OK;
    MgtObjectMap::iterator it;

    std::string xp;
    if (dataXPath.size() > 0)
    {
      xp.assign(dataXPath);
    }
    else
      xp.assign(getFullXpath(true));

    for (it = children.begin(); it != children.end(); ++it)
    {
      MgtObject* child = it->second;
      rc = child->read(xp, db);
      if(CL_OK != rc)
        return rc;
    }
    return rc;
  }
  ClRcT MgtContainer::read(std::string parentXPath,MgtDatabase *db)
  {
    logDebug("MGT","SET","Read data, xpath %s",parentXPath.c_str());
    ClRcT rc = CL_OK;
    MgtObjectMap::iterator it;

    std::string xp;
    if (dataXPath.size() > 0)
    {
      xp.assign(dataXPath);
    }
    else if (parentXPath.size() > 0)
    {
      xp.assign(parentXPath);
      xp.append(getFullXpath(false));
    }
    else
      xp.assign(getFullXpath(true));

    for (it = children.begin(); it != children.end(); ++it)
    {
      MgtObject* child = it->second;
      rc = child->read(xp,db);
      if(CL_OK != rc)
      {
        logDebug("MGT","SET","Read data failed [%x] for child %s. Ignored",rc,child->tag.c_str());
        return rc;
      }
    }
    return rc;
  }
  ClRcT MgtContainer::write(std::string parentXPath,MgtDatabase *db)
  {
    ClRcT rc = CL_OK;
    MgtObjectMap::iterator it;
    std::string xp = getFullXpath();
    if(parentXPath.size() > 0)
    {
      xp = getFullXpath(false);
      parentXPath.append(xp);
      xp = parentXPath;
    }
    for (it = children.begin(); it != children.end(); ++it)
    {
      MgtObject* child = it->second;
      rc = child->write(xp,db);
      if(CL_OK != rc)
      {
        logDebug("MGT","SET","Write data failed [%x] for child %s. Ignored",rc,child->tag.c_str());
        return rc;
      }
    }
    return rc;
  }

  ClBoolT MgtContainer::set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
  {
    logDebug("MGT","SET","Set data for Container");
    SAFplus::MgtObjectMap::iterator iter;
    int ret, nodetyp, depth;
    xmlChar *valstr, *namestr, *attrval;
    xmlNodePtr node;
    xmlAttr* attr;

    char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
    string strChildData;

    xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, NULL, NULL, 0);
    if (!reader)
    {
      logError("MGT", "CONT", "Reader return null");
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
             if ((strcmp((const char *)namestr,this->tag.c_str()) != 0)
                && (strcmp((const char *)namestr,this->listTag.c_str()) != 0))
             {
               logError("MGT","SET","The configuration [%s] isn't for this container [%s]",(const char *)namestr,this->tag.c_str());
               return CL_FALSE;
             }
           }
           else
           {
             snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", namestr);
             strChildData.append(strTemp);
           }
           break;
         }
         /* Closing tag of a node*/
         case XML_ELEMENT_DECL:
         {
           if(depth == 1)
           {
             snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
             strChildData.append(strTemp);
             for(iter = children.begin();iter != children.end(); iter++)
             {
               MgtObject *child = iter->second;
               if(strcmp(child->tag.c_str(),(char *)namestr) == 0)
               {
                 if(child->set(strChildData.c_str(),strChildData.size(),t))
                 {
                   // Set success, reset childData for next child
                   strChildData.assign("");
                 }
                 else
                 {
                   // Set failed for a child, should abort the transaction
                   return CL_FALSE;
                 }
               }
             }
           }
           else if(depth > 1)
           {
             snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
             strChildData.append(strTemp);
           }
           break;
         }
         /* Text value of node */
         case XML_TEXT_NODE:
         {
           strChildData.append((char *)valstr);
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


  }
