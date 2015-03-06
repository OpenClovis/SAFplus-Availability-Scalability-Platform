
all: externalMain

externalMain: externalMain.c log.c log.h ../../externalLib/libmw.so
	g++ -g -o externalMain externalMain.c log.c -I../../../include -DNO_SAF  -L../../externalLib -lm -lmw  -lrt -ldl -lpthread -lezxml

