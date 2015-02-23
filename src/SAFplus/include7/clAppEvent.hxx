#include <saAis.h>
#include <saAmf.h>

namespace SAFplus
{

class AppEvent
{
public:
    /// APPLICATION CALLBACKS
  AppEvent();

  virtual ~AppEvent();

  /** Start yourself (and keep this thread until termination)
   */
  virtual void Start();

  /** Stop yourself, all threads you have created, and return from the "Start" function.
   */
  virtual void Stop();

  /** Pause yourself
   */
  virtual void Suspend();

  /** Ok, you can keep going...
   */
  virtual void Resume();

  /** Override (optional) and return SA_AIS_OK if your app is ok, otherwise return any error
   */
  virtual SaAisErrorT HealthCheck();

  
  /// WORK ASSIGNMENT CALLBACKS

  /** Override and activate the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void ActivateWorkAssignment (const SaAmfCSIDescriptorT& workDescriptor);

  /** Override and set to standby the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void StandbyWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor);

  /** Override and cleanly stop the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void QuiesceWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor);

  /** Override and halt the work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void AbortWorkAssignment    (const SaAmfCSIDescriptorT& workDescriptor);


  /** Override and remove the passed work (SAForum CSI) assignment from your component.
   *  GAS TO DO: Will CSI be stopped before this is called?
   * @param workName The name of the work item (previously passed in an Activate call)
   * @param all      If true, remove ALL matching work items; otherwise, just remove one.
   */
  virtual void RemoveWorkAssignment (const SaNameT* workName, int all);
  
};
  
}
