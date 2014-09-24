S7 := 1
include ../../mk/preface.mk

SAFplusRpcGen := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

#TODO
all: 

gen:
	protoc -I../../3rdparty -I. --cpp_out=./client/ MgtMsg.proto
	$(SAFplusRpcGen) -I../../3rdparty -I. --rpc_out=./client --rename=true MgtMsg.proto
	

