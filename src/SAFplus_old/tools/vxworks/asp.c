/*		*//*
 * main.c : Bootstrap ASP
 *
 *  Created on: Nov 16, 2009
 *      Author: karthick
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <taskLib.h>
#include <rtpLib.h>

#define DEFAULT_LOG_LEVEL "WARN"

typedef struct AspArgs
{
    char targetBase[0x100];
#define ASP_SRC_BASE "/romfs"
    char srcBase[0x100];
    char chassis[20];
    char slot[20];
    char node[40];
    char link[40];
    char debugLevel[40];
    unsigned int foreground:1;
    unsigned int controller: 1;
    unsigned int verbose: 1;
} AspArgsT;

static AspArgsT aspArgs;

void copyEnv(char **old_envp, char ***new_envp)
{
    int i;
    int nenv;
    char **newenvp = NULL;

    for (i = 0, nenv = 0; old_envp[i]; ++i) ++nenv;

    newenvp = malloc((nenv+1) * sizeof(char *));
    assert(newenvp != NULL);

    for (i = 0; old_envp[i]; ++i)
    {
        int len = strlen(old_envp[i]);
        
        newenvp[i] = malloc(len+1);
        assert(newenvp[i] != NULL);
        
        strcpy(newenvp[i], old_envp[i]);
    }
    newenvp[i] = NULL;

    *new_envp = newenvp;

    return;
}

static void appendEnv(char ***env, char *name, char *value)
{                                
    int i;                       
    char buf[BUFSIZ];           
    int len;
    char **p = *env;

    for (i = 0; p[i]; ++i);
                                 
    *env = realloc(*env, (i+2) * sizeof(char *));
    assert(*env != NULL);                           

    p = *env;
    
    snprintf(buf, BUFSIZ, "%s=%s", name, value);
    len = strlen(buf);
                                                   
    p[i] = malloc(len+1);                        
    assert(p[i] != NULL);                      

    strcpy(p[i], buf);
                                                   
    p[++i] = NULL;
}

static void freeEnv(char **env)
{
    int i;
    char **p = env;
    for(i = 0 ; p[i]; ++i) free(p[i]);
    free(env);
}

static char *getLogLevel(char *debugLevel)
{
    const char *defaultLogLevel = DEFAULT_LOG_LEVEL;
    char *s = debugLevel;
    while(*s)
    {
        if(tolower(*s))
            *s = toupper(*s);
        ++s;
    }
    if(!strncmp(debugLevel, "TRACE", 5))
        return "TRACE";
    if(!strncmp(debugLevel, "DEBUG", 5))
        return "DEBUG";
    if(!strncmp(debugLevel, "INFO", 4))
        return "INFO";
    if(!strncmp(debugLevel, "NOTICE", 6))
        return "NOTICE";
    if(!strncmp(debugLevel, "WARN", 7))
        return "WARN";
    if(!strncmp(debugLevel, "ERROR", 5))
        return "ERROR";
    if(!strncmp(debugLevel, "CRITICAL", 8))
        return "CRITICAL";
    if(!strncmp(debugLevel, "ALERT", 5))
        return "ALERT";
    if(!strncmp(debugLevel, "EMERGENCY", 9))
        return "EMERGENCY";
    printf("Unrecognized log level [%s]. Defaulting to [%s]\n", debugLevel, defaultLogLevel);
    return (char*)defaultLogLevel;
}

static void setAspEnvs(char **environ, char ***env)
{
    char value[256];
    copyEnv(environ, env);
    appendEnv(env, "NODENAME", aspArgs.node);
    appendEnv(env, "DEFAULT_NODEADDR", aspArgs.slot);
    appendEnv(env, "LINK_NAME", aspArgs.link);
    if(aspArgs.controller)
        appendEnv(env, "SYSTEM_CONTROLLER", "1");
    else
        appendEnv(env, "SYSTEM_CONTROLLER", "0");
    appendEnv(env, "ASP_NODE_REBOOT_DISABLE", "1");
    appendEnv(env, "ASP_NODE_RESTART", "0");
    appendEnv(env, "CL_LOG_SEVERITY", getLogLevel(aspArgs.debugLevel));
    snprintf(value, sizeof(value), "%s/var/log", aspArgs.targetBase);
    appendEnv(env, "ASP_LOGDIR", value);
    snprintf(value, sizeof(value), "%s/bin", aspArgs.targetBase);
    appendEnv(env, "ASP_BINDIR", value);
    snprintf(value, sizeof(value), "%s/var/lib", aspArgs.targetBase);
    appendEnv(env, "ASP_DBDIR", value);
    snprintf(value, sizeof(value), "%s/etc/asp.d", aspArgs.targetBase);
    appendEnv(env, "ASP_SCRIPTDIR", value);
    snprintf(value, sizeof(value), "%s/var/run", aspArgs.targetBase);
    appendEnv(env, "ASP_RUNDIR", value);
    appendEnv(env, "ASP_CPM_CWD", value);
    appendEnv(env, "ASP_DIR", aspArgs.targetBase);
    snprintf(value, sizeof(value), "%s/etc", aspArgs.targetBase);
    appendEnv(env, "ASP_CONFIG", value);
    snprintf(value, sizeof(value), "%s/lib", aspArgs.targetBase);
    appendEnv(env, "LD_LIBRARY_PATH", value);
    appendEnv(env, "CL_AMF_CKPT_DISABLED", "1");
    appendEnv(env, "CPM_LOG_FILE_SIZE", "1M");
    appendEnv(env, "CPM_LOG_FILE_ROTATIONS", "4");
    appendEnv(env, "CL_MEM_PARTITION", "1");
    appendEnv(env, "CL_VXWORKS_CONSOLE", "1");
}

/*
 * Description:
 */
static void usage(const char *prog)
{
    printf("%s [ -h | This help ] [ -c chassis_id ] [ -l slot_id] [ -n nodename ] [-i tipc_linkname] [ -s | controller ] [ -v | verbose ] [ -d loglevel] aspRunDirBase [aspSrcDirBase]\n\n", prog);
    exit(127);
}

#ifdef TEST_MMAP
static int testMmap(const char *base)
{
    char path[0xff+1];
    char *map = NULL;
    int fd;
    struct stat statbuf;
    int accessMode = 0;
    if(!base)
        base = "/root/vxworks";
    snprintf(path, sizeof(path), "%s/mmapTestFile.cfg", base);
    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0777);
    if(fd < 0)
    {
        printf("opening file [%s] for mmap test returned [%s]\n", path, strerror(errno));
        return -1;
    }
    if(stat(path, &statbuf))
    {
        printf("stat file [%s] for mmap test returned [%s]\n", path, strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    printf("stat dev [%#x], inode no [%d]\n", statbuf.st_dev, statbuf.st_ino);
    if((accessMode = fcntl(fd, F_GETFL)) == ERROR)
    {
        printf("fcntl get flags file [%s] for mmap test returned [%s]. Potential mmaptest failure\n", 
               path, strerror(errno));
    }
    if(ftruncate(fd, 80*4096))
    {
        printf("ftruncate file [%s] for mmap test returned [%s]\n", path, strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    map = mmap(0, 80*4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0);
    if(map == MAP_FAILED)
    {
        printf("mmap test for file [%s], size [%d] failed with [%s]\n", path, 80*4096, strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    if(munmap(map, 80*4096))
    {
        printf("munmap test for file [%s], size [%d] failed with [%s]\n", path, 80*4096, strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    close(fd);
    unlink(path);
    printf("MMAP test success for file [%s], size [%d]. Okay to start ASP now\n", path, 80*4096);
    snprintf(path, sizeof(path), "%s/etc/clEoConfig.xml", base);
    printf("Now mmapping [%s] in read mode\n", path);
    fd = open(path, O_RDWR, 0777);
    if(fd < 0)
    {
        printf("Unable to open file [%s]\n", path);
        return -1;
    }
    map = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(map == MAP_FAILED)
    {
        printf("mmap for file [%s] returned [%s]\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    if(munmap(map, 4096))
    {
        printf("munmap for file [%s] returned [%s]\n", path, strerror(errno));
        close(fd);
    }
    return 0;
    
}
#endif

/*
 * Copy from source to destination.
 */
static int doCp(const char *srcPath, const char *dstPath)
{
    int fd;
    int dstFd;
    char buf[1024];
    int nbytes;
    fd = open(srcPath, O_RDONLY);
    if(fd < 0)
    {
        printf("Opening file [%s] returned with [%s]\n", srcPath, strerror(errno));
        return -1;
    }
    dstFd = open(dstPath, O_CREAT | O_TRUNC | O_RDWR, 0777);
    if(dstFd < 0)
    {
        printf("Opening dst file [%s] returned with [%s]\n", dstPath, strerror(errno));
        close(fd);
        return -1;
    }
    printf("Copying file [%s] to [%s] ...\n", srcPath, dstPath);
    while ( (nbytes = read(fd, buf, sizeof(buf)) ) > 0 )
    {
        if(write(dstFd, buf, nbytes) != nbytes)
        {
            printf("Error writing [%d] bytes to file [%s]\n", nbytes, dstPath);
            close(dstFd);
            close(fd);
            return -1;
        }
    }
    if(nbytes < 0)
    {
        printf("Error reading from file [%s], error [%s]\n", srcPath, strerror(errno));
    }
    else
    {
        printf("Done copying file [%s] to [%s]\n", srcPath, dstPath);
        fsync(dstFd);
    }
    close(fd);
    close(dstFd);
    return 0;
}

static int doCopy(const char *srcDir, const char *dstDir)
{
    int err = 0;
    DIR *d;
    struct dirent *dirent = NULL;
    struct stat statbuf;
    char dirpath[0xff+1];
    char dstpath[0xff+1];
    d = opendir(srcDir);
    if(!d)
    {
        printf("Unable to open src directory [%s]\n", srcDir);
        return -1;
    }
    while ( (dirent = readdir(d) ) )
    {
        if(!strcmp(dirent->d_name, ".") 
           ||
           !strcmp(dirent->d_name, ".."))
            continue;
        snprintf(dirpath, sizeof(dirpath), "%s/%s", srcDir, dirent->d_name);
        if(stat(dirpath, &statbuf))
        {
            printf("Unable to stat path [%s]\n", dirpath);
            goto out;
        }
        /*
         * Single level copy
         */
        if(!S_ISREG(statbuf.st_mode))
        {
            continue;
        }
        snprintf(dstpath, sizeof(dstpath), "%s/%s", dstDir, dirent->d_name);
        if(doCp(dirpath, dstpath))
            goto out;
    }
    err = 0;
    out:
    closedir(d);
    return err;
}

static int setupRamdisk(const char *srcBase, const char *dstBase)
{
    char path[0xff+1];
    char srcpath[0xff+1];
    char cmdbuf[1024];
    register int i;
    const char *dirs[] = {"etc", "bin", "var", "var/log", "var/run", NULL};
    const char *cpdirs[] = {"etc", "bin", NULL};
    const char *dir = NULL;
    /*
     * create the directory layout first
     */
    for(i = 0; dir = dirs[i]; ++i)
    {
        snprintf(path, sizeof(path), "%s/%s", dstBase, dir);
        if(mkdir(path, 0777))
        {
            if(errno != EEXIST)
            {
                printf("mkdir [%s] returned with [%s]\n", path, strerror(errno));
                return -1;
            }
        }
    }
    /*
     * copy the directory layout next. from romfs or src mountpoint
     */
    for(i = 0; dir = cpdirs[i]; ++i)
    {
        snprintf(path, sizeof(path), "%s/%s", dstBase, dir);
        snprintf(srcpath, sizeof(srcpath), "%s/%s", srcBase, dir);
        if(doCopy(srcpath, path))
        {
            printf("xcopy [%s] to [%s] returned with [%s]\n", srcpath, path, strerror(errno));
            return -1;
        }
    }
#if 0
    /*
     * Copy the bootstrapper. asp.vxe
     */
    snprintf(path, sizeof(path), "%s", dstBase);
    snprintf(srcpath, sizeof(srcpath), "%s/bin/%s", srcBase, "asp.vxe");
    if(cp(srcpath, path))
    {
        printf("cp [%s] to [%s] returned with [%s]\n", srcpath, path, strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int main(int argc, char **argv)
{
    
    int rtpId = 0;
    const char *args[] = {"safplus_amf", "-c", aspArgs.chassis, "-l", aspArgs.slot, "-n", aspArgs.node, NULL, NULL};
    char aspImage[256];
    char **env = NULL;
    int c;
    int required = 0;
    int fgIndex = sizeof(args)/sizeof(args[0]) - 2;
    opterr = 0;
    strncpy(aspArgs.chassis, "0", sizeof(aspArgs.chassis)-1);
    strncpy(aspArgs.slot, "1", sizeof(aspArgs.slot)-1);
    strncpy(aspArgs.debugLevel, DEFAULT_LOG_LEVEL, sizeof(aspArgs.debugLevel)-1);
    strncpy(aspArgs.link, "simnet0", sizeof(aspArgs.link)-1);
    strncpy(aspArgs.srcBase, ASP_SRC_BASE, sizeof(aspArgs.srcBase)-1);
    aspArgs.foreground = 0;
    aspArgs.controller = 0;
    aspArgs.verbose = 0;
    while( (c = getopt(argc, argv, "c:l:n:i:d:fsvh")) != EOF)
        switch(c)
        {
        case 'c':
            strncpy(aspArgs.chassis, optarg, sizeof(aspArgs.chassis)-1);
            break;
        case 'l':
            strncpy(aspArgs.slot, optarg, sizeof(aspArgs.slot)-1);
            break;
        case 'n':
            required |= 1;
            strncpy(aspArgs.node, optarg, sizeof(aspArgs.node)-1);
            break;
        case 'i':
            strncpy(aspArgs.link, optarg, sizeof(aspArgs.link)-1);
            break;
        case 'f':
            aspArgs.foreground = 1;
            break;
        case 's':
            aspArgs.controller = 1;
            break;
        case 'v':
            aspArgs.verbose = 1;
            strncpy(aspArgs.debugLevel, "TRACE", sizeof(aspArgs.debugLevel)-1);
            break;
        case 'd':
            strncpy(aspArgs.debugLevel, optarg, sizeof(aspArgs.debugLevel)-1);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
        }
    
    if(!(required & 1) )
    {
        printf("Nodename argument required for spawn AMF\n");
        usage(argv[0]);
    }
    if(optind == argc)
    {
        printf("Target base location for ASP not specified\n");
        usage(argv[0]);
    }
    strncpy(aspArgs.targetBase, argv[optind++], sizeof(aspArgs.targetBase)-1);
    if(optind < argc)
    {
        strncpy(aspArgs.srcBase, argv[optind], sizeof(aspArgs.srcBase)-1);
    }
    if(aspArgs.foreground && fgIndex > 0)
        args[fgIndex] = "-f";
    setupRamdisk(aspArgs.srcBase, aspArgs.targetBase);
#ifdef TEST_MMAP
    if(testMmap(aspArgs.targetBase))
    {
        printf("MMAP doesnt work. Hence ASP component safplus_logd wont function properly\n");
    }
#endif
    setAspEnvs(environ, &env);
    if(chdir(aspArgs.targetBase) == ERROR)
    {
        printf("Cannot change directory to asp runtime base [%s]\n", aspArgs.targetBase);
        exit(127);
    }
    printf("Starting ASP on [%s] with "
           "src base [%s], target base [%s], chassis [%s], slot [%s], node [%s], link [%s], log level [%s] in [%s]\n",
           aspArgs.controller ? "controller" : "payload", aspArgs.srcBase, aspArgs.targetBase, aspArgs.chassis, 
           aspArgs.slot, aspArgs.node, aspArgs.link, aspArgs.debugLevel, aspArgs.foreground ? "foreground" : "background");

    snprintf(aspImage, sizeof(aspImage)-1, "%s/bin/safplus_amf", aspArgs.targetBase);
    rtpId = rtpSpawn(aspImage, (const char **)args, (const char **)env, 100, 1<<20, 0, VX_FP_TASK);
    freeEnv(env);
    if(rtpId == ERROR)
    {
        printf("rtpSpawn for image [%s] returned [%s]\n", aspImage, strerror(errno));
    }
    else
    {
        printf("rtpSpawn for image [%s] success with pid [%d]\n", aspImage, rtpId);
        /*
         * Optional wait for the asp to exit. if required
         */
#if 0
        printf("Waiting for [%s] rtp [%d] to exit\n", args[0], rtpId);
        while(waitpid(rtpId, NULL, 0) == 0);
        printf("Child [%s] [%d] exited\n", args[0], rtpId);
#endif
    }
	return 0;
}
