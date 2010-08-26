from string import Template

scnodeTemplate = Template("""
	<object class="SaClmNode">
		<dn>safNode=${nodename},safCluster=myClmCluster</dn>
		<attr>
			<name>saClmNodeID</name>
			<value>0x00020${index}0f</value>
		</attr>
	</object>
	<object class="SaAmfNode">
		<dn>safAmfNode=${nodename},safAmfCluster=myAmfCluster</dn>
		<attr>
			<name>saAmfNodeSuFailoverMax</name>
			<value>2</value>
		</attr>
		<attr>
			<name>saAmfNodeSuFailOverProb</name>
			<value>1000</value>
		</attr>
		<attr>
			<name>saAmfNodeClmNode</name>
			<value>safNode=${nodename},safCluster=myClmCluster</value>
		</attr>
	</object>
	<object class="SaAmfNodeSwBundle">
		<dn>safInstalledSwBundle=safBundle=OpenSAF,safAmfNode=${nodename},safAmfCluster=myAmfCluster</dn>
		<attr>
			<name>saAmfNodeSwBundlePathPrefix</name>
			<value>/usr/local/lib/opensaf/</value>
		</attr>
	</object>
	<object class="SaAmfSU">
		<dn>safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfSUType</name>
			<value>safVersion=4.0.0,safSuType=OpenSafSuTypeServer</value>
		</attr>
		<attr>
			<name>saAmfSUHostNodeOrNodeGroup</name>
			<value>safAmfNode=${nodename},safAmfCluster=myAmfCluster</value>
		</attr>
		<attr>
			<name>saAmfSUFailover</name>
			<value>1</value>
		</attr>
	</object>
	<object class="SaAmfSU">
		<dn>safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfSUType</name>
			<value>safVersion=4.0.0,safSuType=OpenSafSuTypeND</value>
		</attr>
		<attr>
			<name>saAmfSUHostNodeOrNodeGroup</name>
			<value>safAmfNode=${nodename},safAmfCluster=myAmfCluster</value>
		</attr>
		<attr>
			<name>saAmfSUFailover</name>
			<value>1</value>
		</attr>
	</object>
	<object class="SaAmfSI">
		<dn>safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfSvcType</name>
			<value>safVersion=4.0.0,safSvcType=NoRed-OpenSAF</value>
		</attr>
		<attr>
			<name>saAmfSIProtectedbySG</name>
			<value>safSg=NoRed,safApp=OpenSAF</value>
		</attr>
		<attr>
			<name>saAmfSIRank</name>
			<value>${index}</value>
		</attr>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=RDE,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeRDE</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=RDE-OpenSAF,safComp=RDE,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=DTS,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeDTS</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=DTS-OpenSAF,safComp=DTS,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=FMS,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeFMS</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=FMS-OpenSAF,safComp=FMS,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=IMMD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeIMMD</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=IMMD-OpenSAF,safComp=IMMD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=LOG,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeLOG</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=LOG-OpenSAF,safComp=LOG,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=NTF,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeNTF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=NTF-OpenSAF,safComp=NTF,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=CPD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeCPD</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=CPD-OpenSAF,safComp=CPD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=EDS,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeEDS</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=EDS-OpenSAF,safComp=EDS,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=GLD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeGLD</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=GLD-OpenSAF,safComp=GLD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=IMMND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeIMMND</value>
		</attr>
	</object>
	<object class="SaAmfCSI">
		<dn>safCsi=IMMND,safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCSType</name>
			<value>safVersion=4.0.0,safCSType=IMMND-OpenSAF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=IMMND-OpenSAF,safComp=IMMND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=CPND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeCPND</value>
		</attr>
	</object>
	<object class="SaAmfCSI">
		<dn>safCsi=CPND,safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCSType</name>
			<value>safVersion=4.0.0,safCSType=CPND-OpenSAF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=CPND-OpenSAF,safComp=CPND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=MQD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeMQD</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=MQD-OpenSAF,safComp=MQD,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=MQND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeMQND</value>
		</attr>
	</object>
	<object class="SaAmfCSI">
		<dn>safCsi=MQND,safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCSType</name>
			<value>safVersion=4.0.0,safCSType=MQND-OpenSAF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=MQND-OpenSAF,safComp=MQND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=GLND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeGLND</value>
		</attr>
	</object>
	<object class="SaAmfCSI">
		<dn>safCsi=GLND,safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCSType</name>
			<value>safVersion=4.0.0,safCSType=GLND-OpenSAF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=GLND-OpenSAF,safComp=GLND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfHealthcheck">
		<dn>safHealthcheckKey=GLND,safComp=GLND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfHealthcheckPeriod</name>
			<value>10000000000</value>
		</attr>
		<attr>
			<name>saAmfHealthcheckMaxDuration</name>
			<value>5000000000</value>
		</attr>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=SMFND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeSMFND</value>
		</attr>
	</object>
	<object class="SaAmfCSI">
		<dn>safCsi=SMFND,safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCSType</name>
			<value>safVersion=4.0.0,safCSType=SMFND-OpenSAF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=SMFND-OpenSAF,safComp=SMFND,safSu=${nodename},safSg=NoRed,safApp=OpenSAF</dn>
	</object>
	<object class="SaAmfComp">
		<dn>safComp=SMF,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCompType</name>
			<value>safVersion=4.0.0,safCompType=OpenSafCompTypeSMF</value>
		</attr>
	</object>
	<object class="SaAmfCompCsType">
		<dn>safSupportedCsType=safVersion=4.0.0\,safCSType=SMF-OpenSAF,safComp=SMF,safSu=${nodename},safSg=2N,safApp=OpenSAF</dn>
	</object>
""")