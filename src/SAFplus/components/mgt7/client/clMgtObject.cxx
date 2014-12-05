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
#include <iostream>

#include <clCommon.hxx>
#include <clMgtRoot.hxx>
#include <clMgtObject.hxx>
#include <clMgtDatabase.hxx>

extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */



using namespace std;

namespace SAFplus
  {
  MgtObject::MgtObject(const char* name)
    {
    assert(name);
    tag.assign(name);
    dataXPath.assign("");
    parent = NULL;
    headRev = 1;
    }

  MgtObject::~MgtObject()
    {
    }

  MgtObject* MgtObject::root(void)
    {
    MgtObject* ret = this;
    while(ret->parent) ret = ret->parent;
    return ret;
    }



  ClRcT MgtObject::bind(Handle handle, const std::string module, const std::string route)
    {
    return MgtRoot::getInstance()->bindMgtObject(handle, this, module, route);
    }

  bool MgtObject::match( const std::string &name, const std::string &spec)
    {
    return (name == spec);  // TODO, add wildcard matching (* and ?)
    }

  MgtObject::Iterator MgtObject::begin(void) { return MgtObject::Iterator(); }
  MgtObject::Iterator MgtObject::end(void) { return MgtObject::Iterator(); }
  MgtObject::Iterator MgtObject::multiFind(const std::string &nameSpec) { return MgtObject::Iterator(); }
  MgtObject::Iterator MgtObject::multiMatch(const std::string &nameSpec) { return MgtObject::Iterator(); }

  MgtObject::Iterator::Iterator(const Iterator& i):b(i.b) 
    { 
    i.b->refs++;  
    }  // b must ALWAYS be valid

  MgtObject::Iterator& MgtObject::Iterator::operator=(const Iterator& i)
    { 
    b = i.b;
    b->refs++;  
    }  // b must ALWAYS be valid

  bool MgtIteratorBase::next() { clDbgCodeError(1, "base implementation should never be called");  }
  void MgtIteratorBase::del()  { if (this != &mgtIterEnd) clDbgCodeError(1, "base implementation should never be called"); }

  MgtIteratorBase mgtIterEnd;

  MgtObject* MgtObject::find(const std::string &name) { return nullptr; }
  MgtObject* MgtObject::deepFind(const std::string &name) { return nullptr; }
  MgtObject* MgtObject::deepMatch(const std::string &name) { return nullptr; }

  bool MgtObject::Iterator::operator !=(const MgtObject::Iterator& e) const
    {
    MgtIteratorBase* me = b;
    MgtIteratorBase* him = e.b;

    // in the case of end() e.value will be nullptr, triggering
    // this quick compare
    if (me->current.second != him->current.second) return true;  
    if (him->current.second == nullptr)
      {
      if (me->current.second == nullptr) return false;
      else return true; // one is null other is not; must be !=
      }
    else if (me->current.second == nullptr) return true;  // one is null other is not; must be !=
        
    // ok if this is "real" comparison of two iterators, check the
    // names also
    if (me->current.first != him->current.first) return true;

    // The iterators are pointing at the same object so we'll
    // define that as =, even tho the iterators themselves may not
    // be equivalent.
    return false;
    }

  ClRcT MgtObject::removeChildObject(const std::string& objectName)
    {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This node does not support children");
    return CL_ERR_NOT_EXIST;
    }

  ClRcT MgtObject::addChildObject(MgtObject *mgtObject, std::string const& objectName)
    {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This node does not support children");
    return CL_ERR_BAD_OPERATION;
    }

  ClRcT MgtObject::addChildObject(MgtObject *mgtObject, const char* objectName)
    {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This node does not support children");
    return CL_ERR_BAD_OPERATION;
    }

  void MgtObject::removeAllChildren() {}  // Nothing to do, base class has no children

  void MgtObject::get(std::string *data, ClUint64T *datalen)
  {
    std::stringstream xmlString;
    if(data == NULL)
      return;
    toString(xmlString);
    //logDebug("---","---","String: %s",xmlString.str().c_str());
    *datalen =  xmlString.str().length() + 1;
    data->assign(xmlString.str().c_str());
  }

  /* persistent db to database */
  ClRcT MgtObject::write(MgtDatabase* db)
  {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This function didn't support");
    return CL_ERR_NOT_EXIST;
  }
  ClRcT MgtObject::write(std::string xpt,MgtDatabase* db)
    {
      clDbgCodeError(CL_ERR_BAD_OPERATION,"This function didn't support");
      return CL_ERR_NOT_EXIST;
    }

  /* unmashall db to object */
  ClRcT MgtObject::read(MgtDatabase* db)
  {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This function didn't support");
    return CL_OK;
  }
  ClRcT MgtObject::read(std::string xpt,MgtDatabase *db)
  {
    clDbgCodeError(CL_ERR_BAD_OPERATION,"This function didn't support");
    return CL_OK;
  }

  /*
   * Dump Xpath structure
   */
  void MgtObject::dumpXpath()
    {

    }

  std::string MgtObject::getFullXpath(bool includeParent)
  {
    std::string xpath = "";
    if (parent != NULL && includeParent)
    {
      std::string parentXpath = parent->getFullXpath();
      if (parentXpath.length() > 0)
      {
        xpath.append(parentXpath);
      }
    }
    xpath.append("/").append(this->tag);
    return xpath;
  }


  void MgtObject::dbgDumpChildren()
    {
      std::stringstream dumpStrStream;
      MgtObject::Iterator iter;
      MgtObject::Iterator endd = end();
      for (iter = begin(); iter != endd; iter++)
        {
          const std::string& name = iter->first;
          MgtObject* obj = iter->second;
          obj->toString(dumpStrStream);
        }
      logDebug("MGT","DUMP", "%s", dumpStrStream.str().c_str());
    }

  void deXMLize(const std::string& obj,MgtObject* context, bool& result)
    {
    if ((obj[0] == 't') || (obj[0] == 'T') || (obj[0] == '1')) { result=1; return; }
    if ((obj[0] == 'f') || (obj[0] == 'F') || (obj[0] == '0') || (obj[0] == 'n') || (obj[0] == 'N')) { result=0; return; }
    throw SerializationError("cannot deXMLize into a boolean");
    }

  void deXMLize(const std::string& obj,MgtObject* context, ClBoolT& result)
    {
    if ((obj[0] == 't') || (obj[0] == 'T') || (obj[0] == '1')) { result=1; return; }
    if ((obj[0] == 'f') || (obj[0] == 'F') || (obj[0] == '0') || (obj[0] == 'n') || (obj[0] == 'N')) { result=0; return; }
    throw SerializationError("cannot deXMLize into a boolean");
    }

  }
