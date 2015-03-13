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
 *  \brief Header file of the MgtHistoryStat class which provides APIs to manage MGT historical data
 *  \ingroup mgt
 */

/**
 *  \addtogroup mgt
 *  \{
 */
#pragma once
#ifndef CLMGTHISTORYSTAT_HXX_
#define CLMGTHISTORYSTAT_HXX_

#include <vector>
#include "clMgtObject.hxx"


extern "C"
{
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clDebugApi.h>
} /* end extern 'C' */


#define CL_MAX_HISTORY_ARRAY 60

namespace SAFplus
{
/**
 *  MgtHistoryStat class provides APIs to manage MGT historical data
 */
template <class T>
class MgtHistoryStat : public MgtObject
{
private:
    ClTimerHandleT   mTimerHandle;
    T m10SecTrack;
    T m1MinTrack;
    T m10MinTrack;
    T m1HourTrack;
    T m12HourTrack;
    T m1DayTrack;
    T m1WeekTrack;
protected:

    /*
     * Store the list of historical values
     */
    T mCurrent;
    std::vector<T> mHistory10Sec;
    std::vector<T> mHistory1Min;
    std::vector<T> mHistory10Min;
    std::vector<T> mHistory1Hour;
    std::vector<T> mHistory12Hour;
    std::vector<T> mHistory1Day;
    std::vector<T> mHistory1Week;
    std::vector<T> mHistory1Month;

public:
    //static ClRcT clTstTimerCallback(void *pCookie);

    MgtHistoryStat(const char* name);
    virtual ~MgtHistoryStat();

    ClRcT startTimer();

    void stopTimer();

    /**
     * \brief	Function to set value of the MgtHistoryStat object
     * \param	value					Value to be set
     * \return	void
     */
    void setValue(T value);

    virtual void toString(std::stringstream& xmlString);

    virtual ClRcT write(MgtDatabase* db, std::string xpt = "")
    {
      return CL_OK;
    }
    virtual ClRcT read(MgtDatabase *db, std::string xpt = "")
    {
      return CL_OK;
    }

    /**
     * \brief   Virtual function to validate object data
     */
    virtual ClBoolT validate(void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t);

    /**
     * \brief	Virtual function to calculate current value of the clMgtHistoryStat on timer
     */
    virtual T calculateCurrentValue();
};

// timer callback which will be invoked on timer
template <class T>
ClRcT clTstTimerCallback(void *pCookie)
{
    //MgtHistoryStat<T> *thisObj = (MgtHistoryStat<T> *)pCookie;
	MgtHistoryStat<T> *thisObj = static_cast<MgtHistoryStat<T> *>(pCookie);
    T currentVal = thisObj->calculateCurrentValue();
    thisObj->setValue(currentVal);
    return CL_OK;
}

template <class T>
MgtHistoryStat<T>::MgtHistoryStat(const char* name) : MgtObject(name)
{
    m10SecTrack = 0;
    m1MinTrack = 0;
    m10MinTrack = 0;
    m1HourTrack = 0;
    m12HourTrack = 0;
    m1DayTrack = 0;
    m1WeekTrack = 0;
}

template <class T>
ClRcT MgtHistoryStat<T>::startTimer()
{
    ClRcT rc = CL_OK;

    ClTimerTimeOutT  timeout = {10,0};
    ClTimerTypeT     timerType    = CL_TIMER_REPETITIVE;
    ClTimerContextT  timerContext = CL_TIMER_SEPARATE_CONTEXT;
    mTimerHandle = CL_HANDLE_INVALID_VALUE;

    rc = clTimerCreate(timeout, timerType, timerContext,
                        clTstTimerCallback<T>, static_cast<void *>(this),
                        &mTimerHandle);
    if( CL_OK != rc )
    {
        //Error occured, proper action should be taken
        return rc;
    }

    rc  = clTimerStart(mTimerHandle);
    if( CL_OK != rc )
    {
        // Error occcured, proper cleanup and action should be taken
        return rc;
    }

    return rc;
}

template <class T>
void MgtHistoryStat<T>::stopTimer()
{
    if (mTimerHandle)
    {
        clTimerStop(mTimerHandle);
        clTimerDelete(&mTimerHandle);
    }
}

template <class T>
MgtHistoryStat<T>::~MgtHistoryStat()
{
    stopTimer();
}

template <class T>
ClBoolT MgtHistoryStat<T>::validate( void *pBuffer, ClUint64T buffLen, SAFplus::Transaction& t)
{
    return CL_FALSE;
}

template <class T>
void MgtHistoryStat<T>::toString(std::stringstream& xmlString)
{
    ClUint32T i;

    xmlString << "<" << tag << ">";

    xmlString << "<current>" << mCurrent << "</current>";

    for(i = 0; i< mHistory10Sec.size(); i++)
    {
        xmlString << "<history10sec>" << mHistory10Sec[i] << "</history10sec>";
    }

    for(i = 0; i< mHistory1Min.size(); i++)
    {
        xmlString << "<history1min>" << mHistory1Min[i] << "</history1min>";
    }

    for(i = 0; i< mHistory10Min.size(); i++)
    {
        xmlString << "<history10min>" << mHistory10Min[i] << "</history10min>";
    }

    for(i = 0; i< mHistory1Hour.size(); i++)
    {
        xmlString << "<history1hour>" << mHistory1Hour[i] << "</history1hour>";
    }

    for(i = 0; i< mHistory12Hour.size(); i++)
    {
        xmlString << "<history12hour>" << mHistory12Hour[i] << "</history12hour>";
    }

    for(i = 0; i< mHistory1Day.size(); i++)
    {
        xmlString << "<history1day>" << mHistory1Day[i] << "</history1day>";
    }

    for(i = 0; i< mHistory1Week.size(); i++)
    {
        xmlString << "<history1week>" << mHistory1Week[i] << "</history1week>";
    }

    for(i = 0; i< mHistory1Month.size(); i++)
    {
        xmlString << "<history1month>" << mHistory1Month[i] << "</history1month>";
    }

    xmlString << "</" << tag << ">";
}

template <class T>
void MgtHistoryStat<T>::setValue(T value)
{
    ClBoolT isAccum;
    ClUint32T i;
    T sum;

    mCurrent = value;

    mHistory10Sec.push_back(mCurrent);
    if (mHistory10Sec.size() > CL_MAX_HISTORY_ARRAY)
    {
        m10SecTrack--;
        mHistory10Sec.erase (mHistory10Sec.begin());
    }

    isAccum = CL_FALSE;
    if (mHistory10Sec.size() - m10SecTrack == 6)
    {
        isAccum = CL_TRUE;
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m10SecTrack; i < mHistory10Sec.size(); i++)
        {
            sum += mHistory10Sec[i];
        }
        m10SecTrack = mHistory10Sec.size();

        mHistory1Min.push_back(sum);
        if (mHistory1Min.size() > CL_MAX_HISTORY_ARRAY)
        {
            m1MinTrack--;
            mHistory1Min.erase (mHistory1Min.begin());
        }

        isAccum = CL_FALSE;
        if (mHistory1Min.size() - m1MinTrack == 10)
        {
            isAccum = CL_TRUE;
        }
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m1MinTrack; i < mHistory1Min.size(); i++)
        {
            sum += mHistory1Min[i];
        }
        m1MinTrack = mHistory1Min.size();

        mHistory10Min.push_back(sum);
        if (mHistory10Min.size() > CL_MAX_HISTORY_ARRAY)
        {
            m10MinTrack--;
            mHistory10Min.erase (mHistory10Min.begin());
        }

        isAccum = CL_FALSE;
        if (mHistory10Min.size() - m10MinTrack == 6)
        {
            isAccum = CL_TRUE;
        }
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m10MinTrack; i < mHistory10Min.size(); i++)
        {
            sum += mHistory10Min[i];
        }
        m10MinTrack = mHistory10Min.size();

        mHistory1Hour.push_back(sum);
        if (mHistory1Hour.size() > CL_MAX_HISTORY_ARRAY)
        {
            m1HourTrack--;
            mHistory1Hour.erase (mHistory1Hour.begin());
        }

        isAccum = CL_FALSE;
        if (mHistory1Hour.size() - m1HourTrack == 12)
        {
            isAccum = CL_TRUE;
        }
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m1HourTrack; i < mHistory1Hour.size(); i++)
        {
            sum += mHistory1Hour[i];
        }
        m1HourTrack = mHistory1Hour.size();

        mHistory12Hour.push_back(sum);
        if (mHistory12Hour.size() > CL_MAX_HISTORY_ARRAY)
        {
            m12HourTrack--;
            mHistory12Hour.erase (mHistory12Hour.begin());
        }

        isAccum = CL_FALSE;
        if (mHistory12Hour.size() - m12HourTrack == 2)
        {
            isAccum = CL_TRUE;
        }
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m12HourTrack; i < mHistory12Hour.size(); i++)
        {
            sum += mHistory12Hour[i];
        }
        m12HourTrack = mHistory12Hour.size();

        mHistory1Day.push_back(sum);
        if (mHistory1Day.size() > CL_MAX_HISTORY_ARRAY)
        {
            m1DayTrack--;
            mHistory1Day.erase (mHistory1Day.begin());
        }

        isAccum = CL_FALSE;
        if (mHistory1Day.size() - m1DayTrack == 7)
        {
            isAccum = CL_TRUE;
        }
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m1DayTrack; i < mHistory1Day.size(); i++)
        {
            sum += mHistory1Day[i];
        }
        m1DayTrack = mHistory1Day.size();

        mHistory1Week.push_back(sum);
        if (mHistory1Week.size() > CL_MAX_HISTORY_ARRAY)
        {
            m1WeekTrack--;
            mHistory1Week.erase (mHistory1Week.begin());
        }

        isAccum = CL_FALSE;
        if (mHistory1Week.size() - m1WeekTrack == 4)
        {
            isAccum = CL_TRUE;
        }
    }

    if (isAccum)
    {
        sum = 0;
        for (i = m1WeekTrack; i < mHistory1Week.size(); i++)
        {
            sum += mHistory1Week[i];
        }
        m1WeekTrack = mHistory1Week.size();

        mHistory1Month.push_back(sum);
        if (mHistory1Month.size() > CL_MAX_HISTORY_ARRAY)
        {
            mHistory1Month.erase (mHistory1Month.begin());
        }
    }
}

template <class T>
T MgtHistoryStat<T>::calculateCurrentValue()
{
    return 0;
}

};

#endif /* CLMGTHISTORYSTAT_HXX_ */

/** \} */

