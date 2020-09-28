#ifndef __ETPU_SET_H
#define __ETPU_SET_H

#include "etpu_set_defines.h"
#if defined(MPC5777C)
#include "etpu_c_set_defines.h"
#endif

#define FS_ETPU_ENTRY_TABLE _ENTRY_TABLE_BASE_ADDR_
#define FS_ETPU_MISC _MISC_VALUE_
#define FS_ETPU_ENGINE_MEM_SIZE _ENGINE_DATA_SIZE_
#if defined(MPC5777C)
#define FS_ETPU_C_ENTRY_TABLE C_ENTRY_TABLE_BASE_ADDR_
#define FS_ETPU_C_MISC C_MISC_VALUE_
#define FS_ETPU_C_ENGINE_MEM_SIZE C_ENGINE_DATA_SIZE_
#endif

unsigned int etpu_code[] = {
#include "etpu_set_scm.h"
};

unsigned int etpu_globals[] =
{
#undef __GLOBAL_MEM_INIT32
#define __GLOBAL_MEM_INIT32( addr , val ) val,
#include "etpu_set_idata.h"
#undef __GLOBAL_MEM_INIT32
};

#if defined(MPC5777C)
unsigned int etpu_c_code[] = {
#include "etpu_c_set_scm.h"
};

unsigned int etpu_c_globals[] =
{
#undef C_GLOBAL_MEM_INIT32
#define C_GLOBAL_MEM_INIT32( addr , val ) val,
#include "etpu_c_set_idata.h"
#undef C_GLOBAL_MEM_INIT32
};
#endif

#endif /* __ETPU_SET_H */
