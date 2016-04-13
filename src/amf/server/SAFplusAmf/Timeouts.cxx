/* 
 * File Timeouts.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "Timeouts.hxx"


namespace SAFplusAmf
  {

    Timeouts::Timeouts(): SAFplus::MgtContainer("timeouts"), quiescingComplete("quiescingComplete",SaTimeT(120000)), workRemoval("workRemoval",SaTimeT(120000)), workAssignment("workAssignment",SaTimeT(120000))
    {
        this->addChildObject(&quiescingComplete, "quiescingComplete");
        this->addChildObject(&workRemoval, "workRemoval");
        this->addChildObject(&workAssignment, "workAssignment");
        quiescingComplete = SaTimeT(120000);
        workRemoval = SaTimeT(120000);
        workAssignment = SaTimeT(120000);
    };

    std::vector<std::string>* Timeouts::getChildNames()
    {
        std::string childNames[] = { "quiescingComplete", "workRemoval", "workAssignment" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts/quiescingComplete
     */
    SaTimeT Timeouts::getQuiescingComplete()
    {
        return this->quiescingComplete.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts/quiescingComplete
     */
    void Timeouts::setQuiescingComplete(SaTimeT &quiescingCompleteValue, SAFplus::Transaction &txn)
    {
        this->quiescingComplete.set(quiescingCompleteValue,txn);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts/workRemoval
     */
    SaTimeT Timeouts::getWorkRemoval()
    {
        return this->workRemoval.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts/workRemoval
     */
    void Timeouts::setWorkRemoval(SaTimeT &workRemovalValue, SAFplus::Transaction &txn)
    {
        this->workRemoval.set(workRemovalValue,txn);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts/workAssignment
     */
    SaTimeT Timeouts::getWorkAssignment()
    {
        return this->workAssignment.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts/workAssignment
     */
    void Timeouts::setWorkAssignment(SaTimeT &workAssignmentValue, SAFplus::Transaction &txn)
    {
        this->workAssignment.set(workAssignmentValue,txn);
    };

    Timeouts::~Timeouts()
    {
    };

}
/* namespace ::SAFplusAmf */
