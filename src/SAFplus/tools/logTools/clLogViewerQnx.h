#ifndef __CL_LOG_VIEWER_COMMON_H__
#define __CL_LOG_VIEWER_COMMON_H__


#ifdef QNX_BUILD

typedef void (*sighandler_t)(int);

struct _ENTRY;

struct hsearch_data
{
    struct _ENTRY *table;
    unsigned int size;
    unsigned int filled;
};

#define hcreate_r(a, b)    do{int z; z = (b==NULL)?1:0;}while(0)
#define hsearch_r(a,b,c,d) (b==0||c==NULL||d==NULL)
#define hdestroy_r(a)

#define strptime(a, b, c) (NULL)

#endif

#endif
