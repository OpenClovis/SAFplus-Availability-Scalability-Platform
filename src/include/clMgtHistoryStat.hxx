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

//#include <vector>
#include <boost/circular_buffer.hpp>
#include <clMgtContainer.hxx>
#include <clMgtStat.hxx>
#include <clThreadApi.hxx>
#include <clCommon.hxx>


//#define CL_MAX_HISTORY_ARRAY 1000

namespace SAFplus
{

  inline std::string stringify(float f)
    {
      char s[64];
      sprintf(s,"%1.2f",f);
      return std::string(s);
    }

  inline std::string stringify(int i)
    {
      return std::to_string(i);
    }


  template <class T> class MgtCircularBuffer:public MgtObject
  {
  public:
    boost::circular_buffer<T> value;
    SAFplus::Mutex lock;
    MgtCircularBuffer():MgtObject(""),value(SAFplusI::MgtHistoryArrayLen)
    {}

    void push_back(const T& item) { SAFplus::ScopedLock<> l(lock); value.push_back(item); }
    void push_front(const T& item) { SAFplus::ScopedLock<> l(lock); value.push_front(item); }
    void pop_front() { SAFplus::ScopedLock<> l(lock); value.pop_front(); }
    void pop_back() { SAFplus::ScopedLock<> l(lock); value.pop_back(); }   
    

    void initialize(const char* name)
    {
      tag = name;
    }

    MgtCircularBuffer(char* name,int size): MgtObject(name), value(size)
    {}

    virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth,SerializationOptions opts=SerializeNoOptions)
    {
      SAFplus::ScopedLock<> l(lock);
      xmlString << "<" << tag;
      if (opts & MgtObject::SerializeNameAttribute)
        xmlString << " name=" << "\"" << getFullXpath(false) << "\"";
      if (opts & MgtObject::SerializePathAttribute)
        xmlString << " path=" << "\"" << getFullXpath() << "\"";

      xmlString << ">";
      auto i = value.begin();
      if (i!=value.end())
        {
        xmlString << std::to_string(*i);
        i++;
        for (; i != value.end(); i++)
          {
          xmlString << "," << stringify(*i);
          }
        }
      xmlString << "</" << tag << ">";

    }
  };
  

  static const char* historyNames[] = { "history10sec", "history1min", "history10min","history1hour", "history1day", "history1week", "history4weeks"};
  static int historyMultiples[] = { 6,10,60,24,7,28,0 };
  static const int NumHistoryGroups = sizeof(historyMultiples)/sizeof(int);

    enum class HistoryOperation
    {
      SUM, MAX, MIN, AVE
    };

/**
 *  MgtHistoryStat class provides APIs to manage MGT historical data
 */
template <class T>
class MgtHistoryStat : public MgtContainer
{
private:
  // ClTimerHandleT   mTimerHandle;
#if 0
    T m10SecTrack;
    T m1MinTrack;
    T m10MinTrack;
    T m1HourTrack;
  //T m12HourTrack;
    T m1DayTrack;
    T m1WeekTrack;
#endif
protected:

    /*
     * Store the list of historical values
     */
    MgtCircularBuffer<T> mHistory[7];  //
    uint mCounts[7];

public:
    MgtStat<T> current;
    HistoryOperation op;

    //static ClRcT clTstTimerCallback(void *pCookie);
    MgtHistoryStat();
    MgtHistoryStat(const char* name);
    void initialize(void);

    virtual ~MgtHistoryStat();

    ClRcT startTimer();

    void stopTimer();

#if 0
    void sample(T value)
      {
        current = value;
        roll();
      }

    void roll(void)
      {
        mHistory10Sec.append(current);
      }
#endif
    /**
     * \brief	Function to set value of the MgtHistoryStat object
     * \param	value					Value to be set
     * \return	void
     */
    void setValue(T value);

    void operator = (T value)
    {
      setValue(value);
    }
  // virtual void toString(std::stringstream& xmlString, int depth=SAFplusI::MgtToStringRecursionDepth,SerializationOptions opts=SerializeNoOptions);

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
    MgtHistoryStat<T> *thisObj = static_cast<MgtHistoryStat<T> *>(pCookie);
    T currentVal = thisObj->calculateCurrentValue();
    thisObj->setValue(currentVal);
    return CL_OK;
}

template <class T>
void MgtHistoryStat<T>::initialize(void)
{
  op=HistoryOperation::SUM;
  addChildObject(&current,current.tag);
  for (int i=0;i<NumHistoryGroups;i++)
    {
      mHistory[i].initialize(historyNames[i]);
      mCounts[i] = 0;
      addChildObject(&mHistory[i],historyNames[i]);
    }

}

template <class T>
MgtHistoryStat<T>::MgtHistoryStat(const char* name) : MgtContainer(name),current("current")
{
  initialize();
}

template <class T>
MgtHistoryStat<T>::MgtHistoryStat() : MgtContainer(""),current("current")
{
  initialize();
}

template <class T>
ClRcT MgtHistoryStat<T>::startTimer()
{
#if 0
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
#endif
}

template <class T>
void MgtHistoryStat<T>::stopTimer()
{
#if 0
    if (mTimerHandle)
    {
        clTimerStop(mTimerHandle);
        clTimerDelete(&mTimerHandle);
    }
#endif
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
void MgtHistoryStat<T>::setValue(T value)
{
    T sum;

    current.value = value;

    mHistory[0].push_back(current.value);
    mCounts[0]++;
    for (int j=0;j<NumHistoryGroups-1;j++)  // -1 because we never roll the last group
      {
        if (mCounts[j] >= historyMultiples[j])
          {
            auto i = mHistory[j].value.rbegin();
            if (i==mHistory[j].value.rend()) break;
            T sum = *i;
            i++;
            for(int amt=1; amt<historyMultiples[j];amt++,i++)
              {
                if ((op == HistoryOperation::SUM)||(op== HistoryOperation::AVE))
                  {
                    sum += *i;
                  }
                else if (op == HistoryOperation::MIN)
                  {
                    sum = std::min(sum,*i);
                  }  
                else if (op == HistoryOperation::MAX)
                  {
                    sum = std::max(sum,*i);
                  }        
              }
            if (op== HistoryOperation::AVE)
              {
                sum=sum/historyMultiples[j];
              }
            mCounts[j]=0;
            mHistory[j+1].push_back(sum);
            mCounts[j+1]++;
          }
        else break;  // if this one didn't add to the next one, the next ones can't possibly be ready to roll over so exit out of the for loop
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

