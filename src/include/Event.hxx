/*
 * Event.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENT_HXX_
#define EVENT_HXX_

namespace SAFplus
{

    class Event
    {
        public:
            Event();
            virtual
            ~Event();
            SAFplus::Handle eventClientHandle;
            Event(SAFplus::Handle handle)
            {
                eventClientHandle = handle;
            }
    };

} /* namespace SAFplus */
#endif /* EVENT_HXX_ */
