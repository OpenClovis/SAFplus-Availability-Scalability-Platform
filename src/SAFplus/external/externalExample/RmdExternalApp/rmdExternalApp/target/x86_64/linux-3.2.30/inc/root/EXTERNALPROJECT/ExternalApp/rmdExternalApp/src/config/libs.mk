# Generated make include file. do not modify
/root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/lib/libClConfig.a: /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/obj//root/EXTERNALPROJECT/ExternalApp/rmdExternalApp/src/config/clASPCfg.o /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/obj//root/EXTERNALPROJECT/ExternalApp/rmdExternalApp/src/config/clHeapCustom.o /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/obj//root/EXTERNALPROJECT/ExternalApp/rmdExternalApp/src/config/asp_build.o
	$(call cmd,ar)
/root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/lib/libClConfig.so: /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/lib/shared-6.1/libClConfig.so.6.1
	$(call cmd,ln) shared-6.1/libClConfig.so.6.1 /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/lib/libClConfig.so

/root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/lib/shared-6.1/libClConfig.so.6.1: /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/obj//root/EXTERNALPROJECT/ExternalApp/rmdExternalApp/src/config/clASPCfg.o /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/obj//root/EXTERNALPROJECT/ExternalApp/rmdExternalApp/src/config/clHeapCustom.o /root/EXTERNALPROJECT/ExternalApp/rmdExternalApp//target/x86_64/linux-3.2.30/obj//root/EXTERNALPROJECT/ExternalApp/rmdExternalApp/src/config/asp_build.o
	$(call cmd,link_shared)
