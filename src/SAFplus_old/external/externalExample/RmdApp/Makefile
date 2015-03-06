
all: externalMain

externalMain: rmdExternalMain.c  alarm_clock_EOClient.c alarm_clock_EOServer.c  alarm_clock_EOAlarmClockopServerStub.c alarm_clock_EOAlarmClockopServer.c common/clIdlOpen.c common/xdrAcClockTMarshall.c common/xdrAcClockTUnMarshall.c common/xdrAcTimeTMarshall.c common/xdrAcTimeTUnMarshall.c alarm_clock_EOAlarmClockopClient.c  ../../externalLib/libmw.so
	g++ -g -o rmdExternalMain rmdExternalMain.c  alarm_clock_EOClient.c alarm_clock_EOServer.c  alarm_clock_EOAlarmClockopServerStub.c alarm_clock_EOAlarmClockopServer.c common/clIdlOpen.c common/xdrAcClockTMarshall.c common/xdrAcClockTUnMarshall.c common/xdrAcTimeTMarshall.c common/xdrAcTimeTUnMarshall.c alarm_clock_EOAlarmClockopClient.c  -I../../../include  -DNO_SAF  -L../../externalLib -lm -lmw -lezxml -lrt -lm -ldl -lpthread 



