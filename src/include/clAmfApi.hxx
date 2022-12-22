#pragma once
//? <section name="Availability Management">

/*? <desc order="1"><html><p>The Availability Managment Framework (AMF) has the job of starting, monitoring, and assigning redundancy roles for all applications running on all nodes in the cluster.</p>
<p>Applications interface to the AMF via the SA-Forum defined AMF APIs specified in the <a href="http://help.openclovis.com/saf/SAI-AIS-AMF-B.04.01.AL/">SAI-AIS-AMF-B.04.01.AL</a> specification.  But the simplest way to learn the AMF application interface is through the examples provided by SAFplus.</p>
<p>The AMF uses a cluster description to decide what applications to run where.  This description contains information about all of the applications, and nodes, their redundancy relationships, and what should be done if one fails.  This description is defined by an XML file called the "cluster model".  Applications and operators can modifiy this description graphically though the SAFplus IDE, by hand by editing the raw XML, or dynamically (while SAFplus is running) via Management API calls or the SAFplus CLI.</p>
</html>
</desc>
*/

#include <clLogIpi.hxx>

namespace SAFplus
  {
  enum class AmfRedundancyPolicy
    {
    Undefined = 0,
    Custom = 1,
    NplusM = 2,
    };

#ifndef oper_str
#define oper_str(val) (val)?"enabled":"disabled"
#endif

#define CL_AMF_SET_O_STATE(entity, state)     do                \
{                                                               \
  if ((entity)->operState.value != state)                            \
  {                                                            \
    logNotice("POL","N+M","operState of entity [%s] change from [%s] to [%s]", (entity)->name.value.c_str(), oper_str((entity)->operState.value), oper_str(state));     \
    (entity)->operState = state;                                \
  }                                                            \
}                                                               \
while(0)

  };

//? </section>
