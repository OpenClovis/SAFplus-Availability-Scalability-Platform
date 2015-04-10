/* 
 * File EntityByName.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 
#include "SAFplusAmfCommon.hxx"

#include <string>
#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include "EntityByName.hxx"


namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(EntityByName, /SAFplusAmf/EntityByName)

    EntityByName::EntityByName(): SAFplus::MgtContainer("EntityByName"), name("name"), entity("entity")
    {
        this->config = false;
        this->addChildObject(&name, "name");
        name.config = false;
        this->addChildObject(&entity, "entity");
        entity.config = false;
    };

    EntityByName::EntityByName(std::string nameValue): SAFplus::MgtContainer("EntityByName"), name("name"), entity("entity")
    {
        this->name.value =  nameValue;
        this->config = false;
        this->addChildObject(&name, "name");
        name.config = false;
        this->addChildObject(&entity, "entity");
        entity.config = false;
    };

    std::vector<std::string> EntityByName::getKeys()
    {
        std::string keyNames[] = { "name" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* EntityByName::getChildNames()
    {
        std::string childNames[] = { "name", "entity" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/EntityByName/name
     */
    std::string EntityByName::getName()
    {
        return this->name.value;
    };

    /*
     * XPATH: /SAFplusAmf/EntityByName/name
     */
    void EntityByName::setName(std::string nameValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->name.value = nameValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(name.value),nameValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/EntityByName/entity
     */
    std::string EntityByName::getEntity()
    {
        return this->entity.value;
    };

    /*
     * XPATH: /SAFplusAmf/EntityByName/entity
     */
    void EntityByName::setEntity(std::string entityValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->entity.value = entityValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(entity.value),entityValue);
            t.addOperation(opt);
        }
    };

    EntityByName::~EntityByName()
    {
    };

}
/* namespace SAFplusAmf */
