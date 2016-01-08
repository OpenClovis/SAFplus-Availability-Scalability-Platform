

all: cloudCfg


cloudCfg.o: cloudCfg.cpp Makefile
	g++ -g -c $< -o $@ 

cloudCfg: cloudCfg.o Makefile
	g++ $< -lboost_system -o $@
#	g++ $< -lboost_system -lboost_program_options -o $@


clean:
	rm cloudCfg.o cloudCfg
