#include <clAppEvent.hxx>


namespace SAFplus
{

  AppEvent::AppEvent()
  {
    /* GAS TODO: Install this event receiver in a list */
  }

  AppEvent::~AppEvent()
  {}

  /** Start yourself (and keep this thread until termination)
   */
  void AppEvent::Start()
  {}

  /** Stop yourself, all threads you have created, and return from the "Start" function.
   */
  void AppEvent::Stop()
  {}

  /** Pause yourself
   */
  void AppEvent::Suspend()
  {}

  /** Ok, you can keep going...
   */
  void AppEvent::Resume()
  {}

  /** Override (optional) and return CL_OK if your app is ok, otherwise return any error
   */
  SaAisErrorT AppEvent::HealthCheck()
  {
    return SA_AIS_OK;
  }

  
  /// WORK ASSIGNMENT CALLBACKS

  /** Override and activate the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  void AppEvent::ActivateWorkAssignment (const SaAmfCSIDescriptorT& workDescriptor)
  {}

  /** Override and set to standby the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  void AppEvent::StandbyWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor)
  {}

  /** Override and cleanly stop the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  void AppEvent::QuiesceWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor)
  {}

  /** Override and halt the work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  void AppEvent::AbortWorkAssignment    (const SaAmfCSIDescriptorT& workDescriptor)
  {}


  /** Override and remove the passed work (SAForum CSI) assignment from your component.
   *  GAS TO DO: Will CSI be stopped before this is called?
   * @param workName The name of the work item (previously passed in an Activate call)
   * @param all      If true, remove ALL matching work items; otherwise, just remove one.
   */
  void AppEvent::RemoveWorkAssignment (const SaNameT* workName, int all)
  {}
  
}
