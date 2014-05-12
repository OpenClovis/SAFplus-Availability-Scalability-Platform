

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
#if 0
    map<string, vector<MgtObject*>* >::iterator mapIndex;
    for(mapIndex = mChildren.begin(); mapIndex != mChildren.end(); ++mapIndex)
      {
        std::vector<MgtObject*> *objs = (vector<MgtObject*>*) (*mapIndex).second;
        //delete objs;  // GAS note: MUST be reference counted AND only if this object is new-ed.  Most objects will be members of other objects
      }
#endif
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

#if 0
    map<string, vector<MgtObject*>* >::iterator mapIndex = mChildren.find(objectName);

    /* Check if MGT module already exists in the database */
    if (mapIndex == mChildren.end())
      {
        return CL_ERR_NOT_EXIST;
      }

    std::vector<MgtObject*> *objs = (vector<MgtObject*>*) (*mapIndex).second;

    /* Remove MGT module out off the database */
    if (index >= objs->size())
      {
        return CL_ERR_INVALID_PARAMETER;
      }
    objs->erase (objs->begin() + index);
#endif

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
    if (name == nullptr) name = &mgtObject->name;

    // The first place you hook it in is the "main" one, the rest are sym links.
    if (!mgtObject->parent) mgtObject->parent = this;
    children[*name] = mgtObject;

#if 0
    const std::string* name = &objectName;
    if (name == nullptr) name = &mgtObject->Name;

    /* Check if MGT object already exists in the database */
    map<string, vector<MgtObject*>* >::iterator mapIndex = mChildren.find(*name);
    std::vector<MgtObject*> *objs;

    if (mapIndex != mChildren.end())
      {
        objs = (vector<MgtObject*>*) (*mapIndex).second;
      }
    else
      {
        objs = new vector<MgtObject*>;
        mChildren.insert(pair<string, vector<MgtObject *>* >(*name, objs));
      }

    /* Insert MGT object into the database */
    objs->push_back(mgtObject);
    mgtObject->Parent = this;
#endif

    return rc;
  }

  void MgtContainer::toString(std::stringstream& xmlString)
    {
    MgtObjectMap::iterator it;
    map<string, vector<MgtObject*>* >::iterator mapIndex;

    xmlString << '<' << name << '>';

    for (it = children.begin(); it != children.end(); ++it)
      {
      MgtObject* child = it->second;
      child->toString(xmlString);
      }

    xmlString << "</" << name << '>';
    }



  }
