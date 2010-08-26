from string import Template

plnodeTemplate = Template("""
	<object class="SaClmNode">
		<dn>safNode=${nodename},safCluster=myClmCluster</dn>
		<attr>
			<name>saClmNodeID</name>
			<value>0x20${index}0f</value>
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
	<object class="SaAmfCSIAttribute">
		<dn>safCsiAttr=Attr1,safCsi=GLND,safSi=NoRed${index},safApp=OpenSAF</dn>
		<attr>
			<name>saAmfCSIAttriValue</name>
			<value>val1</value>
			<value>val2</value>
		</attr>
	</object>
	<object class="SaAmfCSIAttribute">
		<dn>safCsiAttr=Attr2,safCsi=GLND,safSi=NoRed${index},safApp=OpenSAF</dn>
	</object>
""")