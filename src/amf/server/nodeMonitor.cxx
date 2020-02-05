#include <clFaultApi.hxx>
#include <clGroupApi.hxx>
#include <clGroupIpi.hxx>
#include <clCommon.hxx>
#include <amfRpc.hxx>
#include "nodeMonitor.hxx"
#include <SAFplusAmfModule.hxx>
#include <signal.h>

#define CL_CPM_RESTART_FILE "safplus_restart"
using namespace SAFplus;
using namespace SAFplusI;

extern Group clusterGroup;
extern SAFplus::Fault gfault;
extern SAFplusAmf::SAFplusAmfModule cfg;
extern Handle myHandle;
extern Handle nodeHandle;
extern SAFplus::Fault gfault;
bool isNodeRegistered = false;

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
  if (msg->getLength() != sizeof(HeartbeatData))
    {
      // TODO: log error, notify fault mgr
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
      //logDebug("HB","NOD","Heartbeat response from [%d] handle [%" PRIx64 ":%" PRIx64 "] latency/time difference is [%" PRId64 " ms]",hdl.getNode(), hdl.id[0],hdl.id[1],timeDiff);
      lastHeard[hdl.getNode()] = timerMs();
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

void NodeMonitor::monitorThread(void)
{
  int loopCnt=-1;
  bool waitingForActive = true;
  //assert(maxSilentInterval);  // You didn't call init()

  Group::Iterator end = clusterGroup.end();

  while(!quit)
    {
      loopCnt++;
      int64_t now = timerMs();
      gfault.loadFaultPolicyEnv();
      if (active)
        {
          bool ka[SAFplus::MaxNodes];
	Handle currentHandle = getProcessHandle(SAFplusI::AMF_IOC_PORT,SAFplus::ASP_NODEADDR);
          Handle activeHandle = clusterGroup.getActive(ABORT);
          Handle standbyHandle = clusterGroup.getStandby(ABORT);
	Handle nodeHandle = getNodeHandle(SAFplus::ASP_NODEADDR);
	/*
	when the old active is disconnected, the new active must broadcast its role.
	*/
	if(gfault.getFaultState(standbyHandle) == FaultState::STATE_UNDEFINED)
	{
	   clusterGroup.broadcastRole(activeHandle,standbyHandle,true);
	}
	/*
	when the old active reconnect, it will find out that it is not active anymore.
	After it receive role message broadcasting from the new active, currentHandle (itself) does not resemble activeHandle.
	So it needs to send a message to rejoin group with standby role.
	*/
	if(currentHandle!= activeHandle)
	{
           clusterGroup.sendMemberReJoinMessage(GroupRoleNotifyTypeT::ROLE_STANDBY);
           gfault.registerEntity(nodeHandle, FaultState::STATE_UP);
           gfault.registerEntity(currentHandle, FaultState::STATE_UP);
	}
          if (1)
            {
              ScopedLock<> lock(exclusion);
              for (int i=0;i<SAFplus::MaxNodes;i++)
                {
                  if (lastHeard[i] != 0)
                    {
                      if ((cfg.safplusAmf.healthCheckMaxSilence!=0) && (now - lastHeard[i] > cfg.safplusAmf.healthCheckMaxSilence))
                        {
                          gfault.notify(getNodeHandle(i),AlarmState::ALARM_STATE_ASSERT,AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS,AlarmSeverity::ALARM_SEVERITY_MAJOR,AlarmProbableCause::ALARM_PROB_CAUSE_RECEIVER_FAILURE, gfault.getFaultPolicy());
                          lastHeard[i] = 0;  // after fault mgr notification, reset node as if its new.  If the fault mgr does not choose to kill the node, this will cause us to give the node another maxSilentInterval.
                          // DEBUG only trigger once boost::this_thread::sleep(boost::posix_time::milliseconds(1000000 + SAFplusI::NodeHeartbeatInterval)); 
                        }
                    }
                }
            }

          for (int i=1;i<SAFplus::MaxNodes;i++)  // Start Keepaliving any nodes that the fault manager thinks are up.
            {
              FaultState fs = gfault.getFaultState(getNodeHandle(i));
              if (i != SAFplus::ASP_NODEADDR)  // No point in keepaliving myself
                {
                  if (fs == FaultState::STATE_UP)
                    {
                      Handle hdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,i);
                      ka[i] = true;
                      if (lastHeard[i]==0) lastHeard[i] = timerMs() + InitialHbInterval;
                      HeartbeatData hd;
                      hd.id = HeartbeatData::REQ;
                      hd.now = nowMs();
                      //logDebug("HB","CLM","Heartbeat to handle [%" PRIx64 ":%" PRIx64 "]",hdl.id[0],hdl.id[1]);
                      safplusMsgServer.SendMsg(hdl, (void*) &hd, sizeof(HeartbeatData), SAFplusI::HEARTBEAT_MSG_TYPE);                
                    }
                  else
                    {
                      ka[i] = false;
                    }
                }              
            }

          // Keepalive any nodes in the cluster group.  Every node in this group SHOULD have registered with the fault manager and be UP so this should be a big NO-OP
          for (Group::Iterator it = clusterGroup.begin();it != end; it++)
            {
              Handle hdl = it->first;  // same as gi->id
              const GroupIdentity* gi = &it->second;
              int thisNode = hdl.getNode();
              assert(thisNode < SAFplus::MaxNodes);
              if (thisNode != SAFplus::ASP_NODEADDR)  // No point in keepaliving myself
                {
                  if (ka[thisNode] == false)
                    {
                      logInfo("HB","CLM"," heartbeat to [%d] handle [%" PRIx64 ":%" PRIx64 "] -- in cluster group but not in fault manager",hdl.getNode(), hdl.id[0],hdl.id[1]);
                      if (lastHeard[thisNode]==0) lastHeard[thisNode] = timerMs() + InitialHbInterval;
                      assert(hdl == gi->id);  // key should = handle in the value, if not shared memory is corrupt 
                      HeartbeatData hd;
                      hd.id = HeartbeatData::REQ;
                      hd.now = nowMs();
                      safplusMsgServer.SendMsg(hdl, (void*) &hd, sizeof(HeartbeatData), SAFplusI::HEARTBEAT_MSG_TYPE);
                    }
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
              Handle hdl;
              if (lastHbHandle == INVALID_HDL)  // We never received a HB from the active so fail whatever the cluster manager thinks is active
                {
                  lastHbHandle = clusterGroup.getActive(ABORT);
                }
              hdl = getNodeHandle(lastHbHandle);
              // I need to special case the fault reporting of the ACTIVE, since that fault server is probably dead.
              // TODO: It is more semantically correct to send this notification to the standby fault server by looking at the fault group.  However, it will end up pointing to this node...
              gfault.notifyLocal(hdl,AlarmState::ALARM_STATE_ASSERT,AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS,AlarmSeverity::ALARM_SEVERITY_MAJOR,AlarmProbableCause::ALARM_PROB_CAUSE_RECEIVER_FAILURE, gfault.getFaultPolicy());
            }
        }
      if (!active && !standby && !SAFplus::SYSTEM_CONTROLLER)  // Let the payload ensure that the active is alive
        {
          waitingForActive = true;

verify_active_alive:

          int64_t now = timerMs();
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
                  name.set(SAFplus::ASP_NODENAME,INVALID_HDL,NameRegistrar::MODE_NO_CHANGE);
                  FILE *fp = fopen(CL_CPM_RESTART_FILE, "w");
                  if (!fp)
                  {
                    logCritical("HB","CLM","AMF failure to trigger restart. Please restart safplus manually");
                  }
                  else
                  {
                    fclose(fp);
                  }
                  pid_t amfPid = getpid();
                  kill(amfPid, SIGKILL);
                }
              else
                {
                  logWarning("HB","CLM", "AMF payload blade waiting for AMF active to come up or node handle and amf handle to be valid...");
                }
            }
          else if (!isNodeRegistered)
          {
            EntityIdentifier activeHdl = clusterGroup.getActive();
            if (activeHdl != INVALID_HDL && nodeHandle != INVALID_HDL && myHandle != INVALID_HDL) 
            {
                logInfo("HB","NAM", "Registering this node [%s] as handle [%" PRIx64 ":%" PRIx64 "]", SAFplus::ASP_NODENAME, nodeHandle.id[0],nodeHandle.id[1]);
                name.set(SAFplus::ASP_NODENAME,nodeHandle,NameRegistrar::MODE_NO_CHANGE,true);
                do
                {  // Loop because active fault manager may not be chosen yet
                  gfault.registerEntity(nodeHandle, FaultState::STATE_UP);  // set this node as up
                  boost::this_thread::sleep(boost::posix_time::milliseconds(250));
                } while(gfault.getFaultState(nodeHandle) != FaultState::STATE_UP);

                do
                {
                  gfault.registerEntity(myHandle, FaultState::STATE_UP);    // set this AMF as up
                  boost::this_thread::sleep(boost::posix_time::milliseconds(250));
                } while(gfault.getFaultState(myHandle) != FaultState::STATE_UP);
                isNodeRegistered = true;
            }
            else
            {
                logInfo("HB","CLM","waiting for active SC up, my node handle and my amf handle valid, current values: active SC [%" PRIx64 ":%" PRIx64 "], my node handle [%" PRIx64 ":%" PRIx64 "], my amf handle [%" PRIx64 ":%" PRIx64 "]", activeHdl.id[0],activeHdl.id[1],nodeHandle.id[0],nodeHandle.id[1],myHandle.id[0],myHandle.id[1]);
            }
          }
        }

      uint64_t tmp = cfg.safplusAmf.healthCheckPeriod;
      if (tmp == 0) tmp = 1000; // if "off" loop every second anyway so we can detect when we get turned on.
      boost::this_thread::sleep(boost::posix_time::milliseconds(tmp)); 
    }
}
