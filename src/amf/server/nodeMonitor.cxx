#include <clFaultApi.hxx>
#include <clGroupApi.hxx>
#include <clGroupIpi.hxx>
#include <clCommon.hxx>
#include <amfRpc.hxx>
#include "nodeMonitor.hxx"
#include <SAFplusAmfModule.hxx>
#include <clNameApi.hxx>
#include <clFaultServerIpi.hxx>
#include <amfOperations.hxx>
#include <signal.h>

//#ifdef SAFPLUS_AMF_LOG_NODE_REPRESENTATIVE
#include "../../log/clLogIpi.hxx"
//#endif

#define CL_CPM_RESTART_FILE "safplus_restart"
using namespace SAFplus;
using namespace SAFplusI;

extern Group clusterGroup;
extern SAFplus::Fault gfault;
extern SAFplusAmf::SAFplusAmfModule cfg;
extern Handle myHandle;
extern Handle nodeHandle;
extern SAFplus::Fault gfault;
extern SAFplus::FaultServer fs;
extern bool isNodeRegistered;
extern AmfOperations *amfOpsMgmt;
extern volatile bool    quitting;  // Set to true to tell all threads to quit
extern SAFplusI::GroupServer gs;
extern ClRcT registerInstallInfo(bool active);

struct HeartbeatData
{
  enum
    {
      REQ      = 0xc101,
      REQ_OPP  = 0x01c1,
      RESP     = 0xc102,
      RESP_OPP = 0x02c1,
    };
  uint64_t now;
  uint16_t id;
};

const unsigned int InitialHbInterval = 3000;  // give a just started node an extra few seconds to come up

void NodeMonitor::initialize()
{
  lastHbHandle = INVALID_HDL;
  active = false;
  standby = false;
  safplusMsgServer.registerHandler(SAFplusI::HEARTBEAT_MSG_TYPE,this,0);
  for (int i=0;i<SAFplus::MaxNodes;i++)
    {
      lastHeard[i] = 0;
    }

  //maxSilentInterval = SAFplusI::NodeSilentInterval;
  quit = false;
  thread = boost::thread(boost::ref(*this));
}


void NodeMonitor::finalize()
{
  if (!quit)
    {
    quit = true;
    thread.join();
    }
}

void NodeMonitor::msgHandler(MsgServer* svr, Message* msg, ClPtrT cookie)
{
  ScopedLock<> lock(exclusion);
  if (quitting) return;
  if (msg->getLength() != sizeof(HeartbeatData))
    {
      // TODO: log error, notify fault mgr
      logError("HB","NOD","invalid msg received: msg received len [%d]; expected msg len [%d]", msg->getLength(), (int)sizeof(HeartbeatData));
      msg->free();
      return;
    }
  HeartbeatData hbbuf;
  HeartbeatData* hb = (HeartbeatData*) msg->flatten(&hbbuf,0, sizeof(HeartbeatData),0);
  assert(hb);
  if ((hb->id == HeartbeatData::REQ) || (hb->id == HeartbeatData::REQ_OPP))
    {
      hb->id = HeartbeatData::RESP;
      hb->now = nowMs();
      lastHbHandle = msg->getAddress();
      lastHbRequest = timerMs();
      logDebug("HB","NOD","Heartbeat request from [%d] handle [%" PRIx64 ":%" PRIx64 "] lastHbRequest [%" PRId64 " ms]",lastHbHandle.getNode(), lastHbHandle.id[0],lastHbHandle.id[1],lastHbRequest);
      if (hb == &hbbuf) 
        {
          logCritical("HB","NOD","Heartbeat response misalignment");
          assert(0); // TODO: copy it back, only needed if we used the temp buffer, which we should never do because this message is so small
        }
      svr->SendMsg(msg,SAFplusI::HEARTBEAT_MSG_TYPE);
    }
  else if ((hb->id == HeartbeatData::RESP) || (hb->id == HeartbeatData::RESP_OPP))
    {
      Handle hdl = msg->getAddress();
      if (hb->id == HeartbeatData::RESP_OPP) hb->now = __builtin_bswap64(hb->now);
      int64_t now = nowMs();
      int64_t timeDiff = now - hb->now;
      int node = hdl.getNode();
      lastHeard[node] = timerMs();
      logDebug("HB","NOD","Heartbeat response from [%d] handle [%" PRIx64 ":%" PRIx64 "] latency/time difference is [%" PRId64 " ms]; lastHeard[%d]=[%" PRId64 " ms]",node, hdl.id[0],hdl.id[1],timeDiff, node, lastHeard[node]);
    }
}

void NodeMonitor::becomeActive(void)
{
  standby = false;
  active = true;
}

void NodeMonitor::becomeStandby(void)
{
  active = false;
  standby = true;
  if (lastHbRequest == 0) lastHbRequest = timerMs();  // if we become standby without ever receiving a hb from the active I don't want the checking to trigger right away. 
}

NodeMonitor::~NodeMonitor()
{
  if (!quit) finalize(); // If quit is false, we never initialized this object and started the thread
}

template <size_t N>
void sendHB(const SAFplus::Handle& hdl, const bool (&ka)[N])
{
   int thisNode = hdl.getNode();
   assert(thisNode < SAFplus::MaxNodes);              
   if (thisNode != SAFplus::ASP_NODEADDR)  // No point in keepaliving myself
   {
      if (ka[thisNode] == false)
      {
         //Handle hdl = getProcessHandle(SAFplusI::AMF_IOC_PORT, thisNode);
         logInfo("HB","CLM"," heartbeat to [%d] handle [%" PRIx64 ":%" PRIx64 "] -- not in cluster group and not in fault manager",hdl.getNode(), hdl.id[0],hdl.id[1]);
         //if (lastHeard[thisNode]==0) lastHeard[thisNode] = timerMs() + InitialHbInterval;
         //assert(hdl == gi->id);  // key should = handle in the value, if not shared memory is corrupt 
         HeartbeatData hd;
         hd.id = HeartbeatData::REQ;
         hd.now = nowMs();
         safplusMsgServer.SendMsg(hdl, (void*) &hd, sizeof(HeartbeatData), SAFplusI::HEARTBEAT_MSG_TYPE);
       }
   }
}

void NodeMonitor::monitorThread(void)
{
  int loopCnt=-1;
  bool waitingForActive = true;
  bool activeHdlExist = false;
  //assert(maxSilentInterval);  // You didn't call init()

  Group::Iterator end = clusterGroup.end();

  while(!quit && !quitting)
    {
      loopCnt++;
      int64_t now = timerMs();
      gfault.loadFaultPolicyEnv();
      if (active)
        {
          bool ka[SAFplus::MaxNodes];
	  //Handle currentHandle = getProcessHandle(SAFplusI::AMF_IOC_PORT,SAFplus::ASP_NODEADDR);
          //Handle activeHandle = clusterGroup.getActive(ABORT);
          Handle standbyHandle = clusterGroup.getStandby(ABORT);
	  //Handle nodeHdl = getNodeHandle(SAFplus::ASP_NODEADDR);
          //logInfo("HB","CLM","active handle [%" PRIx64 ":%" PRIx64 "]; standby handle [%" PRIx64 ":%" PRIx64 "]", currentHandle.id[0],currentHandle.id[1], activeHandle.id[0],activeHandle.id[1], standbyHandle.id[0],standbyHandle.id[1]);
          if (currentActive == standbyHandle)
          {
             logNotice("---","---","Active became standby, this node will restart now");
             quitting = true;
             //exit(0);
          }
	/*
	when the old active is disconnected, the new active must broadcast its role.
	*/
#if 0
	if(gfault.getFaultState(standbyHandle) == FaultState::STATE_UNDEFINED)
	{
	   clusterGroup.broadcastRole(activeHandle,standbyHandle,true);
	}
#endif
	/*
	when the old active reconnect, it will find out that it is not active anymore.
	After it receive role message broadcasting from the new active, currentHandle (itself) does not resemble activeHandle.
	So it needs to send a message to rejoin group with standby role.
	*/
#if 0
	if(currentHandle!= activeHandle)
	{
           clusterGroup.sendMemberReJoinMessage(GroupRoleNotifyTypeT::ROLE_STANDBY);
           logInfo("HB","CLM","Registering handle [%" PRIx64 ":%" PRIx64 "] as fault state UP",nodeHandle.id[0],nodeHandle.id[1]);
           gfault.registerEntity(nodeHandle, FaultState::STATE_UP);
           logInfo("HB","CLM","Registering handle [%" PRIx64 ":%" PRIx64 "] as fault state UP",currentHandle.id[0],currentHandle.id[1]);
           gfault.registerEntity(currentHandle, FaultState::STATE_UP);
	}
#endif
          if (1)
            {
              ScopedLock<> lock(exclusion);
              for (int i=0;i<SAFplus::MaxNodes;i++)
                {
                  if (lastHeard[i] != 0)
                    {
                      if ((cfg.safplusAmf.healthCheckMaxSilence!=0) && (now - lastHeard[i] > cfg.safplusAmf.healthCheckMaxSilence))
                        {
                          logInfo("HB","CLM","active: not heard from node %d",i);
                          Handle faultHdl = getNodeHandle(i);
                          try {
                            name.handleFailure(NameRegistrar::FailureType::FAILURE_NODE,faultHdl);
                          }catch (Error& e) {
                             logError("HB","CLM","active: error message [%s]", e.errStr);
                          }
                           
                          fs.registerRelatedEntities(gfault, faultHdl, FaultState::STATE_DOWN);
                          Handle amfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,i);
                          bool skip = false;
                          for (auto it = members.cbegin(); it != members.cend(); it++)
                          {
                            const GroupIdentity* gi = dynamic_cast<const GroupIdentity*>(*it);
                            if (gi->id == amfHdl)
                            {
                                skip = true;
                                break;
                            }
                          }
                          if (!skip)
                          {
                            const GroupIdentity* gi = clusterGroup.getMember(amfHdl);
                            if (gi)
                            {
                              logDebug("HB","CLM","active: store member with id [%" PRIx64 ":%" PRIx64 "]",amfHdl.id[0], amfHdl.id[1]);
                              gs.m_reelect = true;
                              GroupIdentity* pgi = new GroupIdentity;
                              memcpy(pgi, gi, sizeof(GroupIdentity));
                              members.push_back(pgi);
                              //clusterGroup.deregister(amfHdl);
                            }
                            else
                            {
                              logWarning("HB","CLM","active: there is no member with id [%" PRIx64 ":%" PRIx64 "]",amfHdl.id[0], amfHdl.id[1]);
                            }
                            //syslog(LOG_INFO,"active: node %d not heard",i);
                            Handle faultHdl = getNodeHandle(i);
                            logInfo("HB","CLM","active: notifying fault entity [%" PRIx64 ":%" PRIx64 "]", faultHdl.id[0],faultHdl.id[1]);
                            gfault.notify(faultHdl,AlarmState::ALARM_STATE_ASSERT,AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS,AlarmSeverity::ALARM_SEVERITY_MAJOR,AlarmProbableCause::ALARM_PROB_CAUSE_RECEIVER_FAILURE, gfault.getFaultPolicy());
                            //lastHeard[i] = 0;  // after fault mgr notification, reset node as if its new.  If the fault mgr does not choose to kill the node, this will cause us to give the node another maxSilentInterval.
                            // DEBUG only trigger once boost::this_thread::sleep(boost::posix_time::milliseconds(1000000 + SAFplusI::NodeHeartbeatInterval));
                          }
                          lastHeard[i] = 0;
                        }
                      else
                        {
                          logInfo("HB","CLM","active: heard from node %d",i);
                          Handle amfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,i);
                          for (auto it = members.cbegin(); it != members.cend(); it++)
                          {
                             const GroupIdentity* gi = dynamic_cast<const GroupIdentity*>(*it);
                             if (gi->id == amfHdl)
                             {
                                 if (gi->credentials|SC_ELECTION_BIT) //Only SystemContoler node can be taken into account
                                 {
                                    FaultState fstate = gfault.getFaultState(amfHdl);
                                    bool isMember = clusterGroup.isMember(amfHdl);
                                    logInfo("HB","CLM","Amf Handle [%" PRIx64 ":%" PRIx64 "] Fault: [%s] Member: [%c], reelect [%c]",amfHdl.id[0],amfHdl.id[1],c_str(fstate), isMember?'Y':'N',gs.m_reelect?'Y':'N');
                                    // Might TODO: sync data (checkpoint data after the dead node up again???
                                    if (fstate != FaultState::STATE_UP)
                                    {
                                        gfault.registerEntity(amfHdl, FaultState::STATE_UP);
                                        gfault.registerEntity(getNodeHandle(i), FaultState::STATE_UP);
                                    }
                                    if (!isMember)
                                    {
                                       gs.registerEntityEx(clusterGroup.handle,gi->id, gi->credentials,NULL,0, gi->capabilities,false);
                                       gs.m_reelect = false;
                                       amfOpsMgmt->splitbrainInProgress = true;
                                    }
                                    isMember = clusterGroup.isMember(amfHdl);
                                    //fs = gfault.getFaultState(amfHdl);
                                    logInfo("HB","CLM","Amf Handle [%" PRIx64 ":%" PRIx64 "] Fault: [%s] Member: [%c], reelect [%c]",amfHdl.id[0],amfHdl.id[1],c_str(fstate), isMember ? 'Y':'N', gs.m_reelect?'Y':'N');        
                                    if (gs.m_reelect && fstate == FaultState::STATE_UP && isMember)
                                    {
                                       logDebug("HB","CLM","active: deregister myself and the register again to the cluster group to trigger re-election");
                                       GroupIdentity me;
                                       memcpy(&me, &clusterGroup.myInformation, sizeof(GroupIdentity));
                                       clusterGroup.deregister();
                                       clusterGroup.registerEntity(myHandle, me.credentials, me.capabilities);
                                       gs.m_reelect = false;
                                       amfOpsMgmt->splitbrainInProgress = true;
                                    }
                                    else
                                    {
                                       logDebug("HB","CLM","active: no need to trigger re-election");
                                    }
                                 }
                                 delete gi;
                                 members.erase(it);
                                 logInfo("HB","CLM","active: register myself [%s] as handle [%" PRIx64 ":%" PRIx64 "]", ASP_NODENAME, nodeHandle.id[0], nodeHandle.id[1]);
                                 name.set(ASP_NODENAME, nodeHandle, NameRegistrar::MODE_NO_CHANGE, true);
                                 break;
                             }
                             else
                             {
                                 logWarning("HB","CLM","active: member with id [%" PRIx64 ":%" PRIx64 "] not found from the nodeMonitor",amfHdl.id[0], amfHdl.id[1]);
                             }
                          }
                        }
                    }
                }
            }

          for (int i=1;i<SAFplus::MaxNodes;i++)  // Start Keepaliving any nodes that the fault manager thinks are up.
            {
              FaultState fstate = gfault.getFaultState(getNodeHandle(i));
              if (i != SAFplus::ASP_NODEADDR)  // No point in keepaliving myself
                {
                  if (fstate == FaultState::STATE_UP)
                    {
                      Handle hdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,i);
                      ka[i] = true;
                      //if (lastHeard[i]==0) lastHeard[i] = timerMs() + InitialHbInterval;
                      HeartbeatData hd;
                      hd.id = HeartbeatData::REQ;
                      hd.now = nowMs();
                      logDebug("HB","CLM","Heartbeat to handle [%" PRIx64 ":%" PRIx64 "]; lastHeart[%d]=%" PRId64 "ms",hdl.id[0],hdl.id[1], i, lastHeard[i]);
                      safplusMsgServer.SendMsg(hdl, (void*) &hd, sizeof(HeartbeatData), SAFplusI::HEARTBEAT_MSG_TYPE);                
                    }
                  else
                    {
                      ka[i] = false;
                    }
                }              
            }

          // Keepalive any nodes in the cluster group.  Every node in this group SHOULD have registered with the fault manager and be UP so this should be a big NO-OP
          //for (int thisNode=1;thisNode<SAFplus::MaxNodes;thisNode++)
          //{
#if 0
          for (Group::Iterator it = clusterGroup.begin();it != end; it++)
            {
              Handle hdl = it->first;  // same as gi->id
              const GroupIdentity* gi = &it->second;
              int thisNode = hdl.getNode();
              assert(thisNode < SAFplus::MaxNodes);
#endif
          for (auto it = members.cbegin(); it != members.cend(); it++)
            {
              const GroupIdentity* gi = dynamic_cast<const GroupIdentity*>(*it);
              sendHB(gi->id, ka);
#if 0
              const Handle& hdl = gi->id;              
              int thisNode = hdl.getNode();
              assert(thisNode < SAFplus::MaxNodes);              
              if (thisNode != SAFplus::ASP_NODEADDR)  // No point in keepaliving myself
                {
                  if (ka[thisNode] == false)
                    {
                      logInfo("HB","CLM"," heartbeat to [%d] handle [%" PRIx64 ":%" PRIx64 "] -- not in cluster group and not in fault manager",hdl.getNode(), hdl.id[0],hdl.id[1]);
                      //if (lastHeard[thisNode]==0) lastHeard[thisNode] = timerMs() + InitialHbInterval;
                      //assert(hdl == gi->id);  // key should = handle in the value, if not shared memory is corrupt 
                      HeartbeatData hd;
                      hd.id = HeartbeatData::REQ;
                      hd.now = nowMs();
                      safplusMsgServer.SendMsg(hdl, (void*) &hd, sizeof(HeartbeatData), SAFplusI::HEARTBEAT_MSG_TYPE);
                    }
                }
#endif
            }

          for (Group::Iterator it = clusterGroup.begin();it != end; it++)
            {
              Handle hdl = it->first;
              const GroupIdentity* gi = &it->second;
              int count = 0;
              for (auto it2 = members.cbegin(); it2 != members.cend(); it2++)
                 {
                    const GroupIdentity* gi2 = dynamic_cast<const GroupIdentity*>(*it2);
                    if (gi->id != gi2->id)
                    {
                       count++;
                    }
                 }
              if (count == members.size() || count==0)
              {
                 sendHB(gi->id, ka);
              } 
            }

        }
      if (standby)  // Let the standby ensure that the active is alive
        {
          int64_t now = timerMs();
          if (loopCnt&31==0) 
            {
            cfg.safplusAmf.healthCheckMaxSilence.read();  // Periodically reload from the database to get any changes.  TODO: checkpoint should notify us
            cfg.safplusAmf.healthCheckPeriod.read();
            }

          if ((cfg.safplusAmf.healthCheckMaxSilence.value>0)&&((now > lastHbRequest)&&(now - lastHbRequest > cfg.safplusAmf.healthCheckMaxSilence)))
            {
              // TODO: Split brain avoidance.  The standby should send HB reqs to all the other nodes and count how many it can talk to.  If it is half or greater, then become active.  Otherwise, print a split brain critical message and exit              
              if (lastHbHandle == INVALID_HDL)  // We never received a HB from the active so fail whatever the cluster manager thinks is active
              {
                 lastHbHandle = clusterGroup.getActive(ABORT);
              }
              Handle& amfHdl = lastHbHandle;              
              logInfo("HB","CLM","standby: not heard from active [%" PRIx64 ":%" PRIx64 "]",amfHdl.id[0], amfHdl.id[1]);
              Handle hdl = getNodeHandle(amfHdl.getNode());
              try {
                name.handleFailure(NameRegistrar::FailureType::FAILURE_NODE,hdl);
              }catch (Error& e) {
                logError("HB","CLM","stanby: error message [%s]", e.errStr);
              }
              fs.registerRelatedEntities(gfault, hdl, FaultState::STATE_DOWN);
              bool skip = false;
              for (auto it = members.cbegin(); it != members.cend(); it++)
              {
                 const GroupIdentity* gi = dynamic_cast<const GroupIdentity*>(*it);
                 if (gi->id == amfHdl)
                 {
                    skip = true;
                    break;
                 }
              }
              if (!skip)
              {
                 const GroupIdentity* gi = clusterGroup.getMember(amfHdl);
                 if (gi)
                 {
                    logDebug("HB","CLM","standby: store member with id [%" PRIx64 ":%" PRIx64 "]",amfHdl.id[0], amfHdl.id[1]);
                    gs.m_reelect = true;
                    GroupIdentity* pgi = new GroupIdentity;
                    memcpy(pgi, gi, sizeof(GroupIdentity));
                    members.push_back(pgi);
                    clusterGroup.deregister(amfHdl);
                 }
                 else
                 {
                     logWarning("HB","CLM","standby: there is no member with id [%" PRIx64 ":%" PRIx64 "]",amfHdl.id[0], amfHdl.id[1]);
                 }
                 //Handle hdl;
                 /*if (lastHbHandle == INVALID_HDL)  // We never received a HB from the active so fail whatever the cluster manager thinks is active
                 {
                    lastHbHandle = clusterGroup.getActive(ABORT);
                 }*/
                 //hdl = getNodeHandle(lastHbHandle);
                 // I need to special case the fault reporting of the ACTIVE, since that fault server is probably dead.
                 // TODO: It is more semantically correct to send this notification to the standby fault server by looking at the fault group.  However, it will end up pointing to this node...
                 gfault.notifyLocal(hdl,AlarmState::ALARM_STATE_ASSERT,AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS,AlarmSeverity::ALARM_SEVERITY_MAJOR,AlarmProbableCause::ALARM_PROB_CAUSE_RECEIVER_FAILURE, gfault.getFaultPolicy());
               }
            }
            else
            {
               if (lastHbHandle == INVALID_HDL)  // We never received a HB from the active so fail whatever the cluster manager thinks is active
               {
                   lastHbHandle = clusterGroup.getActive(ABORT);
               }
               const FaultState& fltState = gfault.getFaultState(lastHbHandle);
               logInfo("HB","CLM","standby: heard from [%" PRIx64 ":%" PRIx64 "]; fault state [%s]", lastHbHandle.id[0], lastHbHandle.id[1], c_str(fltState));
               Handle& amfHdl = lastHbHandle;
               for (auto it = members.cbegin(); it != members.cend(); it++)
               {
                   const GroupIdentity* gi = dynamic_cast<const GroupIdentity*>(*it);
                   if (gi->id == amfHdl)
                   {
                       delete gi;
                       members.erase(it);                       
                       logInfo("HB","CLM","standby: register myself [%s] as handle [%" PRIx64 ":%" PRIx64 "]", ASP_NODENAME, nodeHandle.id[0], nodeHandle.id[1]);
                       name.set(ASP_NODENAME, nodeHandle, NameRegistrar::MODE_NO_CHANGE, true);
                       break;
                   }
                   else
                   {
                       logWarning("HB","CLM","standby: member with id [%" PRIx64 ":%" PRIx64 "] not found from the nodeMonitor",amfHdl.id[0], amfHdl.id[1]);
                   }
                }
            }
        }
      if (!active && !standby && !SAFplus::SYSTEM_CONTROLLER)  // Let the payload ensure that the active is alive
        {
          waitingForActive = true;
          Handle activeHandle;
          int64_t now;
verify_active_alive:
          if (!activeHdlExist)
          {
             activeHandle = clusterGroup.getActive(ABORT);
             logDebug("HB","CLM","payload: current active [%" PRIx64 ":%" PRIx64 "]", activeHandle.id[0], activeHandle.id[1]);         
             if (activeHandle != INVALID_HDL)
             {
                activeHdlExist = true;
                goto active_exists;
             }
          }
          now = timerMs();
          if (loopCnt&31==0) 
            {
              cfg.safplusAmf.healthCheckMaxSilence.read();  // Periodically reload from the database to get any changes.  TODO: checkpoint should notify us
              cfg.safplusAmf.healthCheckPeriod.read();
            }
          if ((cfg.safplusAmf.healthCheckMaxSilence.value>0)&&((now > lastHbRequest)&&(now - lastHbRequest > cfg.safplusAmf.healthCheckMaxSilence)))
            {
              // Not found active, failover may happen so wating for active in 5s
              if (waitingForActive)
                {
                  waitingForActive = false;
                  if (isNodeRegistered)
                  {                    
                    logAlert("HB","CLM", "No leader in the cluster, this node will be restarted in 5 seconds");
                  }
                  boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
                  goto verify_active_alive;
                }
              else if (isNodeRegistered)
                {
                  // Deregister my node handle from the name service before I restart
                  logInfo("HB","NAM", "Deregistering this node [%s], handle [%" PRIx64 ":%" PRIx64 "] from the name service", SAFplus::ASP_NODENAME, nodeHandle.id[0],nodeHandle.id[1]);
                  name.remove(SAFplus::ASP_NODENAME);
                  char *runDir = getenv("ASP_RUNDIR");
                  char fileName[512];
                  strncpy(fileName, runDir, 511);
                  strncat(fileName, "/", 511);
                  strncat(fileName, CL_CPM_RESTART_FILE, 511);
                  FILE *fp = fopen(fileName, "w");
                  if (!fp)
                  {
                    logCritical("HB","CLM","AMF failure to trigger restart. Please restart safplus manually");
                  }
                  else
                  {
                    fclose(fp);
                  }
                  pid_t amfPid = getpid();
                  kill(amfPid, SIGINT);
                }
              else
                {
                  logWarning("HB","CLM", "AMF payload blade waiting for AMF active to come up or node handle and amf handle to be valid...");
                }
            }
          else 
          {
active_exists:
            if (!isNodeRegistered) 
            {
                EntityIdentifier activeHdl = clusterGroup.getActive();
                if (activeHdl != INVALID_HDL && nodeHandle != INVALID_HDL && myHandle != INVALID_HDL)
                {
                    logInfo("HB","NAM", "Registering this node [%s] as handle [%" PRIx64 ":%" PRIx64 "]", SAFplus::ASP_NODENAME, nodeHandle.id[0],nodeHandle.id[1]);
                    name.set(SAFplus::ASP_NODENAME,nodeHandle,NameRegistrar::MODE_NO_CHANGE);
                    do
                    {  // Loop because active fault manager may not be chosen yet
                      logInfo("HB","CLM","Registering handle [%" PRIx64 ":%" PRIx64 "] as fault state UP",nodeHandle.id[0],nodeHandle.id[1]);
                      gfault.registerEntity(nodeHandle, FaultState::STATE_UP);  // set this node as up
                      boost::this_thread::sleep(boost::posix_time::milliseconds(250));
                    } while(gfault.getFaultState(nodeHandle) != FaultState::STATE_UP);

                    do
                    {
                      logInfo("HB","CLM","Registering handle [%" PRIx64 ":%" PRIx64 "] as fault state UP",myHandle.id[0],myHandle.id[1]);
                      gfault.registerEntity(myHandle, FaultState::STATE_UP);    // set this AMF as up
                      boost::this_thread::sleep(boost::posix_time::milliseconds(250));
                    } while(gfault.getFaultState(myHandle) != FaultState::STATE_UP);
                    isNodeRegistered = true;
                    registerInstallInfo(false);
                }
                else
                {
                    logInfo("HB","CLM","waiting for active SC up, my node handle and my amf handle valid, current values: active SC [%" PRIx64 ":%" PRIx64 "], my node handle [%" PRIx64 ":%" PRIx64 "], my amf handle [%" PRIx64 ":%" PRIx64 "]", activeHdl.id[0],activeHdl.id[1],nodeHandle.id[0],nodeHandle.id[1],myHandle.id[0],myHandle.id[1]);
                }
            }            
          }
        }

      uint64_t tmp = cfg.safplusAmf.healthCheckPeriod;
      if (tmp == 0) tmp = 1000; // if "off" loop every second anyway so we can detect when we get turned on.      
      boost::this_thread::sleep(boost::posix_time::milliseconds(tmp));
    }
}
