/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/* ev.h
 * Clovis Evaluation Kit Console Library
 */
#include "ev.h"
int ptsout;
FILE *ptsoutfile;
int ptserr;
FILE *ptserrfile;
/* int ev_init(int argc, char *argv)
 *
 * Initializes console to direct output to given
 * pts specified as a command line parameter
 *
 * Takes: argc, argv - standard cli parameters
 *
 * Returns: 0 if success
 */
int ev_init(int argc, char *argv[], char *instance_name)
{
	/* process command line args to see if we want to redirect
	 * output to or pseudo terminal console.  we are checking
	 * if the -p flag is specified */
	int pflag = 0;
	int options;
	int retval;
	char *ptsfile = malloc(sizeof(char[255]));

	ptsfile = strcpy(ptsfile, "/var/log/");
	
	while ((options = getopt(argc, argv, "p")) != -1)
		switch (options)
		{
			case 'p':
				pflag = 1;
				break;
			default:
				break;
		}
	
	/* if -p is not specified, do nothing and return */
	if (pflag == 0)
		return 0;
	
	printf("ev: redirecting output of %s to ", instance_name);
	/* otherwise, redirect stdout to /dev/pts/ file specified
	 * in /tmp/clinstance_name.tty */
	ptsfile = strcat(ptsfile, instance_name);
	ptsfile = strcat(ptsfile, ".log");
	printf("%s\n", ptsfile);

	setvbuf(stdout, NULL, _IONBF, 0);

	ptsout = open(ptsfile,
		   (O_WRONLY | O_CREAT | O_SYNC | O_APPEND),
		   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (ptsout == -1)
	{
		fprintf(stderr, "ev: Failed to open %s for console output\n", ptsfile);
		return 1;
	}

	if (dup2(ptsout, STDOUT_FILENO) == -1)
	{
		fprintf(stderr, "ev: Failed to redirect stdout to %s\n", ptsfile);
		return 1;
	}

	while (retval = close(ptsout), retval == -1 && errno == EINTR);
	if (retval == -1) { 
		fprintf(stderr, "ev: Failed to close %s\n", ptsfile);
		return 1;
	}

	/* now redirect stderr to /dev/pts/ file specified in
	 * /tmp/clinstancename.tty */
	ptserr = open(ptsfile,
		   (O_WRONLY | O_CREAT | O_SYNC | O_APPEND),
		   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (ptserr == -1)
	{
		fprintf(stderr, "ev: Failed to open %s for console output\n", ptsfile);
		return 1;
	}

	if (dup2(ptserr, STDERR_FILENO) == -1)
	{
		fprintf(stderr, "ev: Failed to redirect stdout to %s\n", ptsfile);
		return 1;
	}

	while (retval = close(ptserr), retval == -1 && errno == EINTR);
	if (retval == -1) {
		fprintf(stderr, "ev: Failed to close %s\n", ptsfile);
		return 1;
	}

	return 0;
}

