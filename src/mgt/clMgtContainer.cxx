
extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */

#include <boost/algorithm/string.hpp>
#include <clMgtContainer.hxx>
#include <clMgtRoot.hxx>
#include <clMgtProv.hxx>
using namespace std;

namespace SAFplus
{

  MgtContainer::~MgtContainer()
  {
  }

  MgtObject* MgtContainer::deepFind(const std::string &s)
  {
    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); it++)
      {
        MgtObject* obj = (MgtObject*) it->second;
        if (it->first == s)
          return obj;
        obj = obj->deepFind(s);
        if (obj)
          return obj;
      }
    return nullptr;
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
        ret.b = h;
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

  void MgtContainer::resolvePath(const char* path, std::vector<MgtObject*>* result)
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
        clDbgNotImplemented("DEEP search for the subsequent match");
        return; 
      }  

    std::string p(path);
    std::size_t idx = p.find("/");
    std::size_t idx2;
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
    else if ((idx2 = childName.find("[")) != std::string::npos)
      {  // First find the list object, then inside the list find the element indexed
        
        std::string listName = childName.substr(0,idx2);
        typename Map::iterator lst = children.find(listName);  // TODO check list name for wildcard
        if (lst != children.end())
          {
            MgtObject *child = lst->second;
            child->resolvePath(path+idx2, result);
          }
#if 0
            for (MgtObjectMap::iterator it = children.begin(); it != children.end(); it++)
        {
          if (it->second->match(childName))
            {
            MgtObject *child = it->second;
            if (idx == std::string::npos) result->push_back(child);
            else child->resolvePath(&path[idx+1], result);
            }
        }
#endif
      }
    else if (childName.find_first_of("*[(])?") != std::string::npos)
      {  // Complex pattern lookup
        for (MgtObjectMap::iterator it = children.begin(); it != children.end(); it++)
        {
          if (it->second->match(childName))
            {
            MgtObject *child = it->second;
            if (idx == std::string::npos) result->push_back(child);
            else child->resolvePath(&path[idx+1], result);
            }
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


  MgtObject* MgtContainer::deepMatch(const std::string &s)
  {
    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); it++)
      {
        MgtObject* ret;
        MgtObject* obj = (MgtObject*) it->second;
        if (match(it->first, s))
          return obj;
        ret = obj->deepMatch(s);
        if (ret)
          return ret;
      }
    return nullptr;
  }

  MgtObject* MgtContainer::find(const std::string &s)
  {
    MgtObjectMap::iterator it = children.find(s);
    if (it != children.end())
      {
        MgtObject* obj = (MgtObject*) it->second;
        return obj;
      }
    return nullptr;
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
    return addChildObject(mgtObject, name);
  }

  ClRcT MgtContainer::addChildObject(MgtObject *mgtObject, const std::string& objectName)
  {
    ClRcT rc = CL_OK;
    assert(mgtObject);

    const std::string* name = &objectName;
    if (name == nullptr)
      name = &mgtObject->tag;
    if (mgtObject->tag.size() == 0)  // Unnamed: so assign it the passed name
      {
        mgtObject->tag = objectName;
      }

    // The first place you hook it in is the "main" one, the rest are sym links.
    if (!mgtObject->parent)
      mgtObject->parent = this;
    children[*name] = mgtObject;

    return rc;
  }

  void MgtContainer::toString(std::stringstream& xmlString,int depth, MgtObject::SerializationOptions opts)
  {
    bool openTagList = false;
    std::string closer;
    if (1) // !parent || !strstr(typeid(*parent).name(), "SAFplus7MgtList"))
      {
        //if (!listTag.empty() )
        //if (parent && typeid(*parent)==typeid(SAFplus::MgtList))
        if (parent && strstr(typeid(*parent).name(), "MgtList"))  // If my parent is a list I need to use a different tag format 
          {
            xmlString << '<' << parent->tag << " listkey=\"" << tag << "\"";
            closer = parent->tag;
          }
        else
          {
          xmlString << '<' << tag;
          closer = tag;
          }
        if (opts & MgtObject::SerializeNameAttribute)
          xmlString << " name=" << "\"" << getFullXpath(false) << "\"";
        if (opts & MgtObject::SerializePathAttribute)
          xmlString << " path=" << "\"" << getFullXpath() << "\"";
        xmlString << '>';
        openTagList = true;
      }
    MgtObject::SerializationOptions newopts = opts;
    if (opts & MgtObject::SerializeOnePath) newopts = (MgtObject::SerializationOptions) (newopts & ~MgtObject::SerializePathAttribute);
    if (depth) for (MgtObjectMap::iterator it = children.begin(); it != children.end(); ++it)
      {
        MgtObject *child = it->second;
        child->toString(xmlString,depth-1, newopts);
      }
    if (openTagList)
      {
        xmlString << "</" << closer << '>';
      }
  }

  void MgtContainer::get(std::string *data)
  {
    std::stringstream xmlString;
    if (data == nullptr)
      return;

    xmlString << '<' << tag << '>';
    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); ++it)
      {
        MgtObject *child = it->second;
        child->toString(xmlString);
      }
    xmlString << "</" << tag << '>';

    data->assign(xmlString.str().c_str());
  }

  ClRcT MgtContainer::read(MgtDatabase *db, std::string parentXPath)
  {
    ClRcT rc = CL_OK;

    if (!config)
      return rc;

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

    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); ++it)
      {
        MgtObject* child = it->second;
        if (child->config)
          {
            rc = child->read(db, xp);
            if (CL_OK != rc)
              {
                logWarning("MGT", "READ", "Read data failed error [0x%x] for child [%s] of [%s]. Ignored", rc, child->tag.c_str(), xp.c_str());
                // TODO: Attempt to initialize the MgtObject to its configured default.  If that cannot happen, remember this error and raise an exception at the end.
              }
            else
              {
                logInfo("MGT", "READ", "Read [%s/%s] OK", xp.c_str(),child->tag.c_str());
              }
          }
        else
          {
            logInfo("MGT", "READ", "skipping nonconfig object [%s/%s]", xp.c_str(),child->tag.c_str());
            // TODO: Initialize the MgtObject to its configured default.           
          }
      }

    if (!this->parent)
      {
        MgtRoot *mgtRoot = MgtRoot::getInstance();
        mgtRoot->updateReference();
      }
    return rc;
  }

  ClRcT MgtContainer::write(MgtDatabase *db, std::string parentXPath)
  {
    ClRcT rc = CL_OK;

    if (!config)
      return rc;

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

    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); ++it)
      {
        MgtObject* child = it->second;
        rc = child->write(db, xp);
        if (CL_OK != rc)
          {
            logDebug("MGT", "SET", "Write data failed [%x] for child [%s]. Ignored", rc, child->tag.c_str());
            return rc;
          }
      }
    return rc;
  }

  ClBoolT MgtContainer::set(const void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
  {
    logDebug("MGT", "SET", "Set data [%s] for container [%s]", (const char*) pBuffer, getFullXpath(true).c_str());
    SAFplus::MgtObjectMap::iterator iter;
    int ret, nodetyp, depth;
    xmlChar *valstr, *namestr;

    char strTemp[CL_MAX_NAME_LENGTH] = { 0 };
    string strChildData;

    xmlTextReaderPtr reader = xmlReaderForMemory((const char*) pBuffer, buffLen, nullptr, nullptr, 0);
    if (!reader)
      {
        logError("MGT", "CONT", "Reader return null");
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
        /* Opening tag of a node */
        case XML_ELEMENT_NODE:
          {
            if (depth == 0)
              {
                if ((strcmp((const char *) namestr, this->tag.c_str()) != 0)
                    && (strcmp((const char *) namestr, this->listTag.c_str()) != 0))
                  {
                    logError("MGT", "SET", "The configuration [%s] isn't for this container [%s]", (const char *)namestr, this->tag.c_str());
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
            if (depth == 1)
              {
                snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
                strChildData.append(strTemp);
                for (iter = children.begin(); iter != children.end(); iter++)
                  {
                    MgtObject *child = iter->second;
                    if (strcmp(child->tag.c_str(), (char *) namestr) == 0)
                      {
                        if (child->set(strChildData.c_str(), strChildData.size(), t))
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
            else if (depth > 1)
              {
                snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", namestr);
                strChildData.append(strTemp);
              }
            break;
          }
          /* Text value of node */
        case XML_TEXT_NODE:
          {
            strChildData.append((char *) valstr);
            break;
          }
          /* Other type: don't care */
        default:
          {
            break;
          }
          }
        ret = xmlTextReaderRead(reader);
      }
    while (ret);

    return true;
  }

  MgtObject *MgtContainer::findMgtObject(const std::string &xpath, std::size_t idx)
  {
    MgtObject *obj = nullptr;
    size_t nextIdx = xpath.find("/", idx + 1);

    if (nextIdx == std::string::npos)
      {
        std::string childName = xpath.substr(idx + 1, xpath.length() - idx - 1);

        typename Map::iterator it = children.find(childName);
        if (it != children.end())
          obj = it->second;
      }
    else
      {
        std::string childName = xpath.substr(idx + 1, nextIdx - idx - 1);

        typename Map::iterator it = children.find(childName);
        if (it != children.end())
          {
            MgtObject *child = it->second;
            obj = child->findMgtObject(xpath, nextIdx);
          }
      }

    return obj;
  }

  ClRcT MgtContainer::setChildObj(const std::string &childName, const std::string &value)
  {
    ClRcT ret = CL_OK;

    typename Map::iterator it = children.find(childName);
    if (it == children.end())
      {
        ret = CL_ERR_DOESNT_EXIST;
        return ret;
      }

    MgtObject *child = it->second;
    child->setObj(value);
    child->loadDb = false;

    return ret;
  }

  ClRcT MgtContainer::setChildObj(const std::map<std::string,std::string> &keyList)
  {
    for (std::map<std::string,std::string>::const_iterator it = keyList.begin(); it != keyList.end(); it++)
      {
        Map::iterator keyChild = children.find(it->first);
        if( keyChild != children.end() )
          {
            MgtObject *child = keyChild->second;
            child->setObj(it->second);
            child->loadDb = false;
          }
      }
    return CL_OK;
  }

  MgtObject* MgtContainer::lookUpMgtObject(const std::string & classType, const std::string &ref)
  {
    std::string type = "P";
    type.append((typeid(*this).name()));
    if ( type == classType)
      { 
        if (this->tag == ref)
          {
            return this;
          }
        typename Map::iterator obj = children.find("name");
        if (obj != children.end())
          {
            SAFplus::MgtProv<std::string>* node = dynamic_cast<SAFplus::MgtProv<std::string>*>(obj->second);
            if (node && node->value == ref)
              {
                return this;
              }
          }
      }
 

    typename Map::iterator iter;
    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); it++)
      {
        MgtObject* obj = (MgtObject*) it->second;
        MgtObject*found = obj->lookUpMgtObject(classType, ref);
        if (found)
          return found;
      }
    return nullptr;
  }

}
