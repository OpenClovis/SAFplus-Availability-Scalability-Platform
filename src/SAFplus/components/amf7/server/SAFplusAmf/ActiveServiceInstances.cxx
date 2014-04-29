/* 
 * File ActiveServiceInstances.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "clMgtObject.hxx"
#include "clMgtProv.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "clMgtProvList.hxx"
#include "ActiveServiceInstances.hxx"


namespace SAFplusAmf {

    /* Apply MGT object factory */
    REGISTERIMPL(ActiveServiceInstances, /SAFplusAmf/ServiceUnit/activeServiceInstances)

    ActiveServiceInstances::ActiveServiceInstances(): ClMgtObject("activeServiceInstances"), current("current"), history10sec("history10sec"), history1min("history1min"), history10min("history10min"), history1hour("history1hour"), history12hour("history12hour"), history1day("history1day"), history1week("history1week"), history1month("history1month")
    {
        this->addChildObject(&current, "current");
        this->addChildObject(&history10sec, "history10sec");
        this->addChildObject(&history1min, "history1min");
        this->addChildObject(&history10min, "history10min");
        this->addChildObject(&history1hour, "history1hour");
        this->addChildObject(&history12hour, "history12hour");
        this->addChildObject(&history1day, "history1day");
        this->addChildObject(&history1week, "history1week");
        this->addChildObject(&history1month, "history1month");
    };

    std::vector<std::string>* ActiveServiceInstances::getChildNames()
    {
        std::string childNames[] = { "current", "history10sec", "history1min", "history10min", "history1hour", "history12hour", "history1day", "history1week", "history1month" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/current
     */
    unsigned long int ActiveServiceInstances::getCurrent()
    {
        return this->current.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/current
     */
    void ActiveServiceInstances::setCurrent(unsigned long int currentValue)
    {
        this->current.Value = currentValue;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history10sec
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory10sec()
    {
        return this->history10sec.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history10sec
     */
    void ActiveServiceInstances::setHistory10sec(unsigned long int history10secValue)
    {
        this->history10sec.Value.push_back(history10secValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1min
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory1min()
    {
        return this->history1min.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1min
     */
    void ActiveServiceInstances::setHistory1min(unsigned long int history1minValue)
    {
        this->history1min.Value.push_back(history1minValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history10min
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory10min()
    {
        return this->history10min.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history10min
     */
    void ActiveServiceInstances::setHistory10min(unsigned long int history10minValue)
    {
        this->history10min.Value.push_back(history10minValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1hour
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory1hour()
    {
        return this->history1hour.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1hour
     */
    void ActiveServiceInstances::setHistory1hour(unsigned long int history1hourValue)
    {
        this->history1hour.Value.push_back(history1hourValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history12hour
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory12hour()
    {
        return this->history12hour.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history12hour
     */
    void ActiveServiceInstances::setHistory12hour(unsigned long int history12hourValue)
    {
        this->history12hour.Value.push_back(history12hourValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1day
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory1day()
    {
        return this->history1day.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1day
     */
    void ActiveServiceInstances::setHistory1day(unsigned long int history1dayValue)
    {
        this->history1day.Value.push_back(history1dayValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1week
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory1week()
    {
        return this->history1week.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1week
     */
    void ActiveServiceInstances::setHistory1week(unsigned long int history1weekValue)
    {
        this->history1week.Value.push_back(history1weekValue);
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1month
     */
    std::vector<unsigned long int> ActiveServiceInstances::getHistory1month()
    {
        return this->history1month.Value;
    };

    /*
     * XPATH: /SAFplusAmf/ServiceUnit/activeServiceInstances/history1month
     */
    void ActiveServiceInstances::setHistory1month(unsigned long int history1monthValue)
    {
        this->history1month.Value.push_back(history1monthValue);
    };

    ActiveServiceInstances::~ActiveServiceInstances()
    {
    };

}
/* namespace SAFplusAmf */