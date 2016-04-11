<section name="Availability Management">
  <example order="1" name="Basic AMF Tutorial">
    <html>
      <h2>Introduction</h2>
      <p>This example covers the basic use of the SAFplus Availability Management Framework (AMF).  When complete, you will have defined and created a redundant program on first one node and then 2 nodes.  These programs will be running in Active/Standby mode and will periodically print their role.  You will then kill one of the programs and watch the roles change and the program be restarted.</p>
      <p>Let's take a look at the code</p>
    </html>

    <html>
      <h2>Code</h2>
      <p>
        When a "normal" program is run, its "main" function starts doing the whatever the application was written to do.  However a SAF-aware application must wait until it gets a notification to become "active" before it really begins work.  Instead, there are 3 elements to the "main" of a SAF-aware application:
        <ul>
          <li> Connect to the AMF </li>
          <li> Dispatch any incoming AMF events, until its time to quit</li>
          <li> Shut down </li>
        </ul>
        This demo's main() function implements this (with the help of other function to be shown next):
      </p>
      <pre>
        <insert ref="AMFBasicMain" />
      </pre>

      <h3>Initialization</h3>
      <p>
        Connecting to the AMF is accomplished via the initializeAmf() helper function.  It is pretty simple.  First, it uses the saAmfInitialize() function to initialize the local AMF library and all its dependencies (messaging, logging, etc).  Callback functions are registered so that the AMF can tell the application to quit, become active/standby, or stop being active/standby.  Next, it registers this component with the AMF by name.
      </p>

      <p>Here is the relevant code:</p>
      <pre>
        <insert ref="AMFBasicInitializeAmf" />
      </pre>

      <p>There are actually 3 different startup modes you may be interested in:
        <ul>
          <li>Use SAFplus clustering services but do NOT put yourself under AMF control (very useful for debugging or applications like CLI)</li>
          <li>Start the application yourself but put it under AMF control </li>
          <li>Let the AMF fully manage the application (the most common case)</li>
        </ul>
        To implement the first case, just call <ref>saAmfInitialize()</ref> or the nonstandard <ref>safplusInitialize()</ref> function.  Do not call <ref>saAmfComponentNameGet</ref> or <ref>saAmfComponentRegister</ref>.  The second two cases are shown via the #if block.  The first technique allows the application to connect to the AMF whether or not the AMF started it.  It does this by first checking if the AMF has provided a component name using the <ref>saAmfComponentNameGet</ref> function.  If so, the AMF DID start this component, and the provided name is used.  If no name is provided, the application registers using a "well known" name.  This name must correspond to a component instance specified in the cluster model.
      </p>
      <p>The final technique simply passes NULL as the application name.  If the AMF started this application, the AMF-assigned component name will be used.  If the AMF did NOT start this application, an error will be returned.</p>

      <h3>Dispatching</h3>

      <p>After initialization your main() should go into a dispatch loop that handles AMF events.  This code is pretty boilerplate so will not be discussed in detail.  Basically, it just blocks on a set of file descriptors and if an event happens on that FD it calls the <ref>saAmfDispatch</ref> function to handle the event.  Ultimately, "handling" the event means calling one of the event handlers that you installed in the Initialization phase.  For the purposes of this demo, it also times out of the select periodically and prints a status line.
      </p>
      <pre>
        <insert ref="AMFBasicDispatchLoop" />
      </pre>

      <h3>Events</h3>

      The heart of the system is the work assignment.  It is here that you will react to active and standby assignments and have your application start doing its job:

      <pre>
        <insert ref="AMFBasicWorkAssignment" />
      </pre>
      <p>
        As you can see, this function is a pretty simple "switch" statement that checks what work assignment operation the AMF is requesting.  This simple demo just sets a global variable "running" to 1 or 2 depending on whether the application was assigned active or standby.  However, a more common useful technique is shown commented out "pthread_create(&amp;,thr,NULL,activeLoop,NULL);".  In other words, spawn a thread that executes your application's job.
      </p>
      <p>
        In fact, making an existing application "SAF-aware" can be as simple as copying this example code to the application, renaming the app's original "main" function to "activeLoop", and spawning a thread to run it as shown here.</p>
      <p>
        That is all that is required for basic high availability.  In the next sections we will compile, run and test this code.
      </p>
    </html>


    <html>
      <h2> The "Basic" Example: Compiling and Running </h2>
      <p>
        This example presumes that you have already installed SAFplus, built it from source.
      </p>
      <p>
        First, Download and detar the example, or cd to [safplus dir]/examples/eval/basic if you are using SAFplus source from Github.
        Next build:
      </p>
      <pre>
        make V=1
      </pre>
      <p>
        The program "basicApp" will be created.  If you installed SAFplus it will be created in this directory, but if you are using SAFplus from source, it will be located in the SAFplus target directory ([safplus]/target/[architecture]/bin).  This will be called the "bin" directory.
      </p><p>
        Next, we need to load a cluster model.  Two example models are provided in this sample:
      </p>
      <ul>
        <li>SAFplusAmf1Node1SG1Comp.xml: Run redundant copies of the app on a single node</li>
        <li>SAFplusAmf2Node1SG1Comp.xml: Run on two nodes</li>
      </ul>
      <p>
        Please choose which option you want, cd to the bin directory, and install the model:
      </p>
      <pre>
        cd SAFplus/target/i686-linux-gnu/bin
        ./safplus_db -x [example_source_dir]/SAFplusAmf1Node1SG1Comp.xml safplusAmf
      </pre>
      <p>
        This command reads the model XML in and writes it to a file called safplusAmf.db in the current directory.  The AMF will read this file to access the cluster model.

        Now set up environment variables:
      </p>
      <pre>
        cd SAFplus/target/i686-linux-gnu/bin
        source setup.basicApp 
      </pre>
      <p>
        You should open this script and familiarize yourself with what it is doing since you will need to modify it for your own applications.  Essentially it:
        <ul>
          <li> Sets up paths to SAFplus libraries (if needed)</li> 
          <li> Configures Linux to allocate more buffers for networking (optional but essential for applications that heavily use networking)</li> 
          <li> Sets up SAFplus environment variables, most importantly:</li> 
          <ul>
            <li> ASP_NODENAME: selects this node's name, which corresponds to a node definition in the XML model file.</li> 
            <li> SAFPLUS_BACKPLANE_INTERFACE: selects the intra-cluster networking interface</li> 
          </ul>
          <li> Sets up the cloud node identification table (only needed if using "cloud" based transports).</li> 
        </ul>
      </p>
      <p>
        Finally, run the SAFplus AMF to start up the model:
      </p>
      <pre>
        ./safplus_amf
      </pre>
      <p>
        You should see the SAFplus AMF start up, load your model from the database, start 2 copies of the "basicApp" application, and then assign them to active and standby roles.
</p>
      <h2>Failovers</h2>
      <p>Next, let's kill the active application.  We should see the standby become active, and a new process be started that eventually becomes standby.  First, look at what is being logged:</p>
      <pre>
Mon Apr 11 15:29:31.717 2016 [main.cxx:370] (node0.25828.25828 : c0.APP.MAIN:00024 : INFO) Basic HA app: Active.  Hello World!
Mon Apr 11 15:29:31.728 2016 [main.cxx:371] (node0.25827.25827 : c1.APP.MAIN:00024 : INFO) Basic HA app: Standby.
      </pre>
      <p>You can find the pid of the logging processes in the log, right after the node name (in this case they are 25827 and 25828).  To be sure, run ps:</p>
<pre>
 # ps -efwww | grep basicApp
root     25827 25813  0 15:29 pts/10   00:00:00 ./basicApp c1
root     25828 25813  0 15:29 pts/10   00:00:00 ./basicApp c0
</pre>
      <p>Now kill the active process using "kill -9":</p>
<pre>
 # kill -9 25828
</pre>

You will see the prior standby become active
<pre>
Mon Apr 11 15:33:08.897 2016 [main.cxx:146] (node0.25827.25834 : c1.APP.MAIN:00242 : INFO) Component [c1] : PID [25827]. CSI Set Received

Mon Apr 11 15:33:08.897 2016 [main.cxx:405] (node0.25827.25834 : c1.APP.MAIN:00243 : INFO) CSI Flags : [Add One]
Mon Apr 11 15:33:08.897 2016 [main.cxx:409] (node0.25827.25834 : c1.APP.MAIN:00244 : INFO) CSI Name : [TODO]
Mon Apr 11 15:33:08.897 2016 [main.cxx:416] (node0.25827.25834 : c1.APP.MAIN:00245 : INFO) Name value pairs :
Mon Apr 11 15:33:08.897 2016 [main.cxx:419] (node0.25827.25834 : c1.APP.MAIN:00246 : INFO) Name : [testKey] Value : [testVal]
Mon Apr 11 15:33:08.897 2016 [amfOperations.cxx:240] (node0.25827.25813 : AMF. OP.CMP:02183 : INFO) Request component [c0] state from node [node0] returned [stopped]
Mon Apr 11 15:33:08.897 2016 [main.cxx:419] (node0.25827.25834 : c1.APP.MAIN:00247 : INFO) Name : [testKey2] Value : [testVal2]
Mon Apr 11 15:33:08.898 2016 [main.cxx:423] (node0.25827.25834 : c1.APP.MAIN:00248 : INFO) HA state : [Active]
Mon Apr 11 15:33:08.898 2016 [main.cxx:427] (node0.25827.25834 : c1.APP.MAIN:00249 : INFO) Active Descriptor :
Mon Apr 11 15:33:08.898 2016 [main.cxx:431] (node0.25827.25834 : c1.APP.MAIN:00250 : INFO) Transition Descriptor : [1]
Mon Apr 11 15:33:08.898 2016 [main.cxx:435] (node0.25827.25834 : c1.APP.MAIN:00251 : INFO) Active Component : []
Mon Apr 11 15:33:08.898 2016 [main.cxx:163] (node0.25827.25834 : c1.APP.MAIN:00252 : INFO) Basic HA app: ACTIVE state requested; activating service
</pre>

Next you will see the killed component get restarted:

<small><small>
<pre>
Mon Apr 11 15:33:08.898 2016 [amfOperations.cxx:685] (node0.25827.25813 : AMF.OPS.SRT:02184 : INFO) Launching Component [c0] as [./basicApp c0] locally with process id [25862], recommended port [134]
</pre>
</small></small>

And finally you will see the status printouts from the two applications, first as Active/Idle and then Active/Standby:

<pre>
Mon Apr 11 15:33:08.985 2016 [main.cxx:370] (node0.25827.25827 : c1.APP.MAIN:00253 : INFO) Basic HA app: Active.  Hello World!
Mon Apr 11 15:33:09.912 2016 [main.cxx:374] (node0.25862.25862 : c0.APP.MAIN:00007 : INFO) Basic HA app: idle
.
.
.
Mon Apr 11 15:33:13.915 2016 [main.cxx:371] (node0.25862.25862 : c0.APP.MAIN:00023 : INFO) Basic HA app: Standby.
Mon Apr 11 15:33:13.990 2016 [main.cxx:370] (node0.25827.25827 : c1.APP.MAIN:00258 : INFO) Basic HA app: Active.  Hello World!
</pre>

      <h2>Troubleshooting</h2>

<ul>
      <li><strong>SAFplus AMF never starts any applications.  It just says:</strong></li>
      <small><small>
      <pre>
Fri Apr  8 14:07:07.388 2016 [nPlusmAmfPolicy.cxx:510] (.0.7671 : AMF.N+M.AUDIT:00996 : INFO) Service Instance [si] should be fully assigned but is [unassigned]. Current active assignments [0], targeting [1]
Fri Apr  8 14:07:07.388 2016 [nPlusmAmfPolicy.cxx:527] (.0.7671 : AMF.N+M.AUDIT:00997 : INFO) Service Instance [si] cannot be assigned 0th active.  No available service units.
Fri Apr  8 14:07:07.388 2016 [customAmfPolicy.cxx:36] (.0.7671 : AMF.POL.CUSTOM:00998 : INFO) Active audit
Fri Apr  8 14:07:07.388 2016 [customAmfPolicy.cxx:47] (.0.7671 : AMF.CUSTOM.AUDIT:00999 : INFO) Auditing service group sg0
      </pre>
      </small></small>
      <p>
        Did you forget to set a node name?  The node names in the model do not match the nodes running. 
      </p>

      <li><strong>SAFplus AMF never starts any applications.  It just says:</strong></li>
      <small><small>
      <pre>
Mon Apr 11 15:44:00.786 2016 [main.cxx:322] (node0.25929.25929 : AMF.AUD.ACT:00000 : DEBUG) Active Audit
Mon Apr 11 15:44:00.786 2016 [main.cxx:325] (node0.25929.25929 : AMF.AUD.ACT:00000 : DEBUG) Active Audit -- Nodes
Mon Apr 11 15:44:00.786 2016 [main.cxx:332] (node0.25929.25929 : AMF.AUD.NOD:00000 : INFO) Node handle [2fff009f00000001:0], [2fff009f00000001:0], 
Mon Apr 11 15:44:00.786 2016 [main.cxx:346] (node0.25929.25929 : AMF.AUD.ACT:00000 : DEBUG) Active Audit -- Applications
Mon Apr 11 15:44:00.786 2016 [nPlusmAmfPolicy.cxx:717] (node0.25929.25929 : AMF.POL.N+M:00000 : INFO) Active audit: Discovery phase
Mon Apr 11 15:44:00.786 2016 [nPlusmAmfPolicy.cxx:347] (node0.25929.25929 : AMF.POL.N+M:00000 : INFO) Active audit: Operation phase
Mon Apr 11 15:44:00.786 2016 [customAmfPolicy.cxx:36] (node0.25929.25929 : AMF.POL.CUSTOM:00000 : INFO) Active audit
      </pre>
      </small></small>
      <p>
        Did you forget load your XML model into the database?  As you can see, the audit does not show any Service Groups, Service Units or Components.
      </p>

      <li><strong>./safplus_amf: error while loading shared libraries: libAmfPolicyEnv.so: cannot open shared object file: No such file or directory</strong></li>
      <p>
        Did you properly set up your LD_LIBRARY_PATH and PYTHON_PATH?  run "source ./setup.basicApp" and look at its contents to see what it does.
      </p>

      <li><strong>terminate called after throwing an instance of 'SAFplus::Error' what():  Address already in use</strong></li>
      <p>
        Is another copy of SAFplus AMF already running?  To kill all running instances of SAFplus and application programs, run "safplus_cleanup".
      </p>
</ul>
    </html>
  </example>
</section>
