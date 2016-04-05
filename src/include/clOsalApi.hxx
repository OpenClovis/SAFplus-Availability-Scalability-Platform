
// OSAL: Operating System Adaption Layer 
// This API group covers all operating system interaction functionality not covered by standard libraries like boost, std or the C libs.

#include <clThreadApi.hxx>
#include <clProcessApi.hxx>

namespace SAFplus
{
  //? Return the current monotonically increasing time in milliseconds
  uint64_t nowMs();
}
