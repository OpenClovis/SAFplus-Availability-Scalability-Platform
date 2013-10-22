# Generated make include file. do not modify
/root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/lib/libClConfig.a: /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/obj/src/config/clASPCfg.o /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/obj/src/config/clHeapCustom.o /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/obj/src/config/asp_build.o
	$(call cmd,ar)
/root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/lib/libClConfig.so: /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/lib/shared-6.1/libClConfig.so.6.1
	$(call cmd,ln) shared-6.1/libClConfig.so.6.1 /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/lib/libClConfig.so

/root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/lib/shared-6.1/libClConfig.so.6.1: /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/obj/src/config/clASPCfg.o /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/obj/src/config/clHeapCustom.o /root/workspace333/rmdExternalExampleModel/target/x86_64/linux-3.2.30/obj/src/config/asp_build.o
	$(call cmd,link_shared)
