#ifndef _CL_VERSION_H_
#define _CL_VERSION_H_

#define CL_VERSION_SHIFT (24)
#define CL_VERSION_MASK ( (1 << CL_VERSION_SHIFT) - 1)

/*
 * Min. or the base version of ASP.
 */
#define CL_RELEASE_VERSION_BASE 4 
#define CL_MAJOR_VERSION_BASE   0
#define CL_MINOR_VERSION_BASE   0

#define CL_RELEASE_VERSION 4
#define CL_MAJOR_VERSION 0
#define CL_MINOR_VERSION 0

#define CL_VERSION_CODE(rel, major, minor) ( ( ( (rel) << 16) ) | ( ( (major) << 8 )  ) | ( (minor) ) )

#define CL_VERSION_RELEASE(version)  ( ((version) >> 16) & 0xff )
#define CL_VERSION_MAJOR(version)    ( ((version) >> 8 ) & 0xff )
#define CL_VERSION_MINOR(version)    ( ((version) & 0xff ) )

#define __VDECL(sym, rel, major, minor) sym##_##rel##_##major##_##minor
#define VDECL_VER(sym, rel, major, minor) __VDECL(sym, rel, major, minor)
#define VDECL(sym) VDECL_VER(sym, CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION)

#if defined (__SERVER__)

#define VSYM(sym, id) { VDECL(sym), id, CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION) }
#define VSYM_VER(sym, rel, major, minor, id) { VDECL_VER(sym, rel, major, minor), id, \
            CL_VERSION_CODE(rel, major, minor)}
#define VSYM_EMPTY(sym, id) { NULL, id, CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION) }
#define VSYM_NULL           { NULL, 0, 0 }

#elif defined (__CLIENT__)

#define VSYM(sym, id) {id, CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION)}
#define VSYM_VER(sym, rel, major, minor, id) {id, CL_VERSION_CODE(rel, major, minor) }
#define VSYM_EMPTY(sym, id) {0, CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION)}
#define VSYM_NULL           {0, 0}

#else

#define VSYM(sym, id) { sym, id }
#define VSYM_VER(sym, rel, major, minor, id) VSYM(sym, id)
#define VSYM_EMPTY(sym, id) VSYM(sym, id) 
#endif

#define VTAB(sym) { sym, CL_VERSION_CODE(CL_RELEASE_VERSION, CL_MAJOR_VERSION, CL_MINOR_VERSION) }
#define VTAB_VER(sym, rel, major, minor) { sym, CL_VERSION_CODE(rel, major, minor) }

#endif
