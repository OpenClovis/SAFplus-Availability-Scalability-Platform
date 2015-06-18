
extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */

#include <clMgtContainer.hxx>
#include "clMgtRoot.hxx"
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

    // The first place you hook it in is the "main" one, the rest are sym links.
    if (!mgtObject->parent)
      mgtObject->parent = this;
    children[*name] = mgtObject;

    return rc;
  }

  void MgtContainer::toString(std::stringstream& xmlString)
  {
    //GAS: TAG already build at MgtList, hardcode to ignore
    if (!parent || !strstr(typeid(*parent).name(), "SAFplus7MgtList"))
      {
        xmlString << '<' << tag << '>';
      }
    for (MgtObjectMap::iterator it = children.begin(); it != children.end(); ++it)
      {
        MgtObject *child = it->second;
        child->toString(xmlString);
      }
    if (!parent || !strstr(typeid(*parent).name(), "SAFplus7MgtList"))
      {
        xmlString << "</" << tag << '>';
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
        rc = child->read(db, xp);
        if (CL_OK != rc)
          {
            logWarning("MGT", "READ", "Read data failed [%x] for child [%s] of [%s]. Ignored", rc, child->tag.c_str(), xp.c_str());
            // TODO: Attempt to initialize the MgtObject to its configured default.  If that cannot happen, remember this error and raise an exception at the end.
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

    return ret;
  }

  MgtObject* MgtContainer::lookUpMgtObject(const std::string & classType, const std::string &ref)
  {
    std::string type = "P";
    type.append((typeid(*this).name()));
    if ( type == classType && this->tag == ref)
      {
        return this;
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
