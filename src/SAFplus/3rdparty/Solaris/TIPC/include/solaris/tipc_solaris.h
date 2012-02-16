#ifndef _TIPC_SOLARIS_H
#define _TIPC_SOLARIS_H

#include <sys/ksynch.h>
#include <sys/kmem.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <atomic.h>
#include <sys/varargs.h>
#include <sys/cmn_err.h>
#include <sys/stream.h>
#include <sys/systm.h>
#include <sys/sunddi.h>

extern int sscanf(const char *, const char *, ...); 

#define likely(a) (a)
#define unlikely(a) (a)

#ifndef min 
#define min(x, y)       ((x) < (y) ? (x) : (y))
#endif  /* min */
  
#define BUG_ON(i)  ASSERT(!(i))

#define __init
#define __exit

struct sk_buff;
typedef struct solaris_spinlock {
	int		init_done; 
	kmutex_t	mutex;
} spinlock_t;

typedef unsigned gfp_t;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint32_t atomic_t;
#define ATOMIC_INIT(i)          ((atomic_t)(i))
#define atomic_inc(p)           atomic_add_int(p, 1) 
#define atomic_dec(p)           atomic_dec_uint(p)
#define atomic_add(n, p)        atomic_add_int(p, n) 
#define atomic_inc_return(v)	atomic_inc_32_nv(v)
#define atomic_read(p)          (*(p))

#define HZ 100

struct timer_list {
	unsigned long expires;
	void (*function)(unsigned long);
	unsigned long data;
	timeout_id_t id;
};

/*
 * TIPC message buffer code
 *
 * TIPC message buffer headroom leaves room for 14 byte Ethernet header, 
 * while ensuring TIPC header is word aligned for quicker access
 */
#ifndef BUF_HEADROOM
#define BUF_HEADROOM 24u
#endif



typedef struct solaris_rwlock {
	int		init_done; 
	krwlock_t	lock;
} rwlock_t;
 
#define KERN_EMERG      "<0>"   /* system is unusable                   */
#define KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define KERN_CRIT	"<2>"	/* critical conditions  		*/
#define KERN_ERR	"<3>"	/* error conditions			*/
#define KERN_WARNING	"<4>"	/* warning conditions			*/
#define KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define KERN_INFO	"<6>"	/* informational			*/
#define KERN_DEBUG	"<7>"	/* debug-level messages 		*/
extern int printk(const char * fmt, ...);

#define DEFINE_SPINLOCK(x)      spinlock_t x = { .init_done = 0 }
extern void spin_lock_init(spinlock_t *lock);
extern void spin_lock_bh(spinlock_t *lock);
extern void spin_unlock_bh(spinlock_t *lock);
extern int spin_trylock_init(spinlock_t *lock);
extern int spin_trylock_bh(spinlock_t *lock);

#define DEFINE_RWLOCK(x)        rwlock_t x =  { .init_done = 0 }
extern void write_lock_bh(rwlock_t *lock);
extern void write_unlock_bh(rwlock_t *lock);
extern void read_lock_bh(rwlock_t *lock);
extern void read_unlock_bh(rwlock_t *lock);
#define GFP_ATOMIC	KM_NOSLEEP
extern void *kmalloc(size_t size, gfp_t flags);
extern void * kzalloc(size_t size, gfp_t flags) ;
/**
 * kcalloc - allocate memory for an array. The memory is set to zero.
 * @n: number of elements.
 * @size: element size.
 * @flags: the type of memory to allocate.
 */
extern void *kcalloc(size_t n, size_t size, gfp_t flags);

#define kfree(x) cmn_err(CE_WARN, "Implement kfree\n")

extern void get_random_bytes(void *buf, int nbytes);

/*
 * struct kmem_cache
 *
 * manages a cache.
 */

struct kmem_cache {
/* 1) per-cpu data, touched during every alloc/free */
/*	struct array_cache *array[NR_CPUS]; */
/* 2) Cache tunables. Protected by cache_chain_mutex */
	unsigned int batchcount;
	unsigned int limit;
	unsigned int shared;

	unsigned int buffer_size;
/* 3) touched by every alloc & free from the backend */
/*	struct kmem_list3 *nodelists[MAX_NUMNODES]; */

	unsigned int flags;		/* constant flags */
	unsigned int num;		/* # of objs per slab */

/* 4) cache_grow/shrink */
	/* order of pgs per slab (2^n) */
	unsigned int gfporder;

	/* force GFP flags, e.g. GFP_DMA */
	gfp_t gfpflags;

	size_t colour;  		/* cache colouring range */
	unsigned int colour_off;	/* colour offset */
	struct kmem_cache *slabp_cache;
	unsigned int slab_size;
	unsigned int dflags;		/* dynamic flags */

	/* constructor func */
	void (*ctor) (void *, struct kmem_cache *, unsigned long);

	/* de-constructor func */
	void (*dtor) (void *, struct kmem_cache *, unsigned long);

/* 5) cache creation/removal */
	const char *name;

};

/**
 * kmem_cache_alloc - Allocate an object
 * @cachep: The cache to allocate from.
 * @flags: See kmalloc().
 *
 * Allocate an object from this cache.  The flags are only relevant
 * if the cache has no available objects.
 */
/* extern void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags); */

extern void *vmalloc(unsigned long size);
extern void vfree(void *addr);

extern unsigned long copy_from_user(void *dst, const void *src, unsigned long len);

extern void init_timer(struct timer_list * timer);
extern int mod_timer(struct timer_list *timer, unsigned long expires);
extern int del_timer_sync(struct timer_list *timer);

 
/* Missing declaration in TIPC include files */
extern int tipc_deleteport(u32 ref);

#endif /* _TIPC_SOLARIS_H */
