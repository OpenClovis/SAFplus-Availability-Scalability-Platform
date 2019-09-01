#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/prctl.h>
#include <string.h>

void sigchild_handler(int signum)
{
	if (signum == SIGUSR1)
	{
		syslog(LOG_INFO, "hello world: got signal [%d]", signum);
		exit(0);
	}
	else
		syslog(LOG_INFO, "hello world: got invalid signal [%d]", signum);
}

int main()
{
	int i=0;
	//int sig=0;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigchild_handler;
	sigaction(SIGUSR1, &sa, NULL);
	prctl(PR_SET_PDEATHSIG, SIGUSR1);
	do
	{
		syslog(LOG_INFO, "hello world! timesss [%d]", i);
		sleep(5);
		if (i==1000)
		{
			syslog(LOG_INFO, "I'm exiting....");
			exit(1);
		}
		i++;
		/*prctl(PR_GET_PDEATHSIG, &sig);
		syslog(LOG_INFO, "got sig [%d]",sig);
		if (sig>0) {
			syslog(LOG_INFO, "Parent process died, I'm following it too");
			break;
		}*/
	}
	while(1);
	syslog(LOG_INFO, "Exiting main app...");
	return 0;
}
