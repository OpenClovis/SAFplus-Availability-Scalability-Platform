// summary of alarm on storage data

#ifndef SUMMARY_HXX_HEADER_INCLUDED_A6DEE18F
#define SUMMARY_HXX_HEADER_INCLUDED_A6DEE18F
#include <AlarmSeverity.hxx>
using namespace SAFplusAlarm;


namespace SAFplus
{

// summary of alarm on storage data
class Summary
{
  public:
	Summary();
	Summary(const AlarmSeverity aseverity);
	std::string toString() const;
    // severity of alarm
    AlarmSeverity severity;
    // total alarms
    int intTotal;
    // total cleared alarm
    int intCleared;
    // total alarm is cleared but not closed
    int intClearedNotClosed;
    // total cleared and closed
    int intClearedClosed;
    // total alarm not cleared and closed
    int intNotClearedClosed;
    // total alarm not cleared and not closed
    int intNotClearedNotClosed;
};
}


#endif /* SUMMARY_HXX_HEADER_INCLUDED_A6DEE18F */
