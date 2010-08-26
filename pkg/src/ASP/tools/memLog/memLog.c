/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*  
 * Author:A.R.Karthick  - karthick@openclovis.com
 * C equivalent to beat the guts out of Vikrams python implementation
 * for the same which is a memory hogger.
 * For example: Vikrams beloved python implementation takes a whopping
 * 701 MB at max w.r.t VM RSS for a 110 MB asp_amf.log
 * C implementation starts off low at 5-8 MB and reaches a max RSS of the file
 * size which is 110 MB or so.
 * 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ftw.h>
#include <assert.h>
#undef CL_ASSERT
#define CL_ASSERT assert
#define SIZEOF(bar) (sizeof((bar))/sizeof((bar)[0]))
#define BAR_RATE (1023)
static char asp_bin[1024] = {0};
static FILE *fptr;
static int file_index;
static int bar_index;

static __inline__ void progress_bar(off_t total,int bytes)
{
    int p = (bytes*100.0)/total;
    static const char bar[] = {'\\','|','/','-'};
    int index = bar_index++;
    bar_index %= SIZEOF(bar);
    if(p >= 100)
    {
        p = 100;
        index = SIZEOF(bar)-1; /*hit the last at the end*/
    }
    /*clear the contents on a refresh*/
    fprintf(stderr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    fprintf(stderr,"%3d %% %.*s [  %c  ]\r",p,60,"................................................................................",bar[index]);
}

static int do_write(const char *name,const char *str,int bytes)
{
    int err = -1;
    static const char *pat = "START";
    if(!fptr && !(fptr = fopen(name,"w") ) )
    {
        fprintf(stderr,"error in fopen of file:%s\n",name);
        goto out;
    }
    if(!strstr(str,pat))
    {
        fprintf(fptr,"\t%s\n",str);
    }
    else
    {
        fprintf(fptr,str);
    }
    fflush(fptr);
    err = 0;
    out:
    return err;
}

static int do_read(int in_fd,const char *str,char *out_buf,int max_bytes)
{
    char buf[256];
    int bytes;
    register char *p;
    char *s;
    bytes = read(in_fd,buf,sizeof(buf)-1);
    CL_ASSERT(bytes > 0 );
    buf[bytes] = 0;
    if(buf[bytes-1] == '\n')
        buf[bytes-1] = 0;

    s = buf;

    /*check for unresolved symbols*/
    if(strstr(s,"??"))
    {
        /*unresolved stuff*/
        char addr[40] = {0};
        bytes = strcspn(str,"\t ");
        strncpy(addr,str,bytes);
        bytes = snprintf(out_buf,max_bytes,"%s(unresolved) %s",addr,str+bytes);
        return bytes;
    }
    if( (p = strrchr(buf,'/') ) ) 
    {
        s = p+1;
    }
    bytes = snprintf(out_buf,max_bytes,"%s %s",s,str += strcspn(str,"\t "));
    return bytes;
}

static __inline__ int do_readline(const char *str,off_t len,char *buf,int bytes)
{
    register int i = 0;
    register const char *p ;
    register char *c = buf;
    if(file_index >= len)
    {
        goto out;
    }
    p = str + file_index;
    str = p;
    while(isspace(*p)) ++p;
    file_index += p - str;
    
    /*
     * Skip holes in the line
     */
    while(file_index < len && *p == 0) 
    {
        ++file_index;
        ++p;
    }

    if(file_index >= len)
    {
        goto out;
    }

    for(i = 0; *p != '\n' && i < bytes - 1 && file_index +i < len; ++i)
        *c++ = *p++;
    *c++ = '\n';
    *c = 0;
    file_index += i;
    out:
    return i;
}

static int do_addr2line(const char *str,off_t len,const char *bin,const char *logfile)
{
    int fds[2][2];
    int pid;
    int err = -1;
    char temp_buf[1024];
    int bytes;
    int bar_rate = 0;
    char *const argv[] = { "addr2line","-e",(char*)bin,NULL};
    int retVal = 0;
    
    if( pipe(fds[0]) < 0 ) 
    {
        perror("pipe:");
        goto out;
    }
    if(pipe(fds[1]) < 0 )
    {
        perror("pipe:");
        goto out;
    }
    switch( (pid = fork() ) )
    {
    case 0:
        {
            /*child*/
            close(fds[0][1]);
            close(fds[1][0]);
            dup2(fds[0][0],0);
            dup2(fds[1][1],1);
            retVal = execvp(argv[0],argv);
            CL_ASSERT(retVal == 0 );
            /*unreached*/
        }
        break;
    case -1:
        perror("fork:");
        goto out;
    default:
        /*parent*/
        {
            char buf[256];
            close(fds[0][0]);
            close(fds[1][1]);
            file_index = 0;
            bar_index = 0;
            fptr = NULL;
            /*
             * now start the parent loop.
             * fire the writes to addr2line and read it back
             */
            while((bytes = do_readline(str,len,buf,sizeof(buf))) > 0 )
            {
                /*write the newline for addr2line to work*/
                write(fds[0][1],buf,bytes+1);
                /*now read from the other end of the pipe*/
                if((bytes = do_read(fds[1][0],buf,temp_buf,sizeof(temp_buf))) < 0 )
                {
                    fprintf(stderr,"error in do_read\n");
                    goto out;
                }
                if( (err = do_write(logfile,temp_buf,bytes) ) < 0 )
                {
                    fprintf(stderr,"error in do_write\n");
                    goto out;
                }
                if(!(++bar_rate & BAR_RATE ) )
                    progress_bar(len,file_index);
            }
            progress_bar(len,file_index);
            fprintf(stderr,"\nDone...\n");
            close(fds[0][1]);
            close(fds[1][0]);
        }
        break;
    }
    err = 0;
    out:
    return err;
}

static int run_addr2line(const char *in_log,off_t len,const char *binary,const char *out_log)
{
    int fd;
    int err = -1;
    char *s;
    fd = open(in_log,O_RDONLY);
    if(fd < 0 )
    {
        perror("open:");
        fprintf(stderr,"error opening file:%s\n",in_log);
        goto out;
    }
    /*now mmap the whole file*/
    s = mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    if(s == MAP_FAILED)
    {
        perror("mmap:");
        fprintf(stderr,"error mapping len:%d\n",(int)len);
        goto out;
    }
    /*give hints to VM for readahead*/
    err = madvise(s,len,MADV_SEQUENTIAL);
    CL_ASSERT(err == 0);
    /*execute addr2line now*/
    err = do_addr2line(s,len,binary,out_log);
    if(err < 0 )
    {
        fprintf(stderr,"Error in do_addr2line for binary:\"%s\"\n",binary);
        goto out;
    }
    err = munmap(s,len);
    CL_ASSERT(err == 0);
    close(fd);
    out:
    return err;
}

static int ftw_callback(const char *file,const struct stat *statbuf,int flags)
{
    int err = 0;
    register char *p ;
    if(flags != FTW_F)
    {
        fprintf(stdout,"\nSkipping entry: %s as its not a file:%d...\n",file,flags);
        goto out;
    }
    if(!statbuf->st_size)
    {
        fprintf(stdout,"\nSkipping entry: %s as its a zero byte file\n",file);
        goto out;
    }
    /*this is a file entry*/
    if( (p = strrchr(file,'.') ) && !strncmp(p+1,"log",3) )
    {
        int len = p - file;
        char log_file[1024] = {0};
        char full_binary[1024] = {0};
        char binary[1024] = {0};
        char *s;
        fprintf(stderr,"\nProcessing file... %s\n",file);
        if(len > sizeof(binary))
        {
            fprintf(stderr,"insufficient binary space:%d\n",len);
            err = -1;
            goto out;
        }
        strncpy(binary,file,len);
        snprintf(log_file,sizeof(log_file),"%s.dat",binary);
        s = binary;
        p = strrchr(binary,'/');
        if(p)
            s = p+1;
        if(!*s)
        {
            fprintf(stderr,"Skipping entry:%s...\n",file);
            goto out;
        }
        snprintf(full_binary,sizeof(full_binary),"%s/%s",asp_bin,s);
        /*run addr2line on this*/
        if(run_addr2line(file,statbuf->st_size,full_binary,log_file) < 0 )
        {
            fprintf(stderr,"Error running addr2line on file:%s\n",file);
            goto out;
        }
    }
    out:
    return err;
}

static int do_parse_log(const char *arg)
{
    char log_path[1024];
    char *s = NULL;
    int err = -1;
    struct stat statbuf;
    s = getenv("ASP_BINDIR");
    if(!s)
    {
        fprintf(stderr,"Please export ASP_BINDIR\n");
        goto out;
    }
    strncpy(asp_bin,s,sizeof(asp_bin)-1);
    if(!arg)
    {
        s = getenv("ASP_LOGDIR");
        if(!s)
        {
            fprintf(stderr,"Please export ASP_LOGDIR\n");
            goto out;
        }
        snprintf(log_path,sizeof(log_path),"%s/%s",s,"mem_logs");
        arg = log_path;
    }
    if(stat(arg,&statbuf) < 0 )
    {
        perror("stat:");
        fprintf(stderr,"error in stat of %s\n",arg);
        goto out;
    }
    /*If its a directory, do a DIR walk*/
    if(S_ISDIR(statbuf.st_mode))
    {
        err = ftw(arg,ftw_callback,1024);
    }
    else
    {
        /*else simulate a ftw_callback*/
        err = ftw_callback(arg,&statbuf,FTW_F);
    }
    if(err < 0 )
    {
        fprintf(stderr,"error in ftw callback\n");
        goto out;
    }
    err = 0;
    out:
    return err;
}

int main(int argc,char **argv)
{
    return do_parse_log(argc > 1 ? argv[1]:NULL);
}
