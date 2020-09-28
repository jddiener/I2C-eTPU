// This file is auto-generated by the ASH WARE ETEC Linker.
//    !!!   DO NOT EDIT THIS FILE   !!!
// Copyright (C) 2015 ASH WARE, Inc. 

// It contains data structures with initialization information.
// The data structures are created using the data initialization macros.
// The host side code uses this initialization information
// to initialize global data and also the private channel frames.

// Global Memory Initialization Data Array
unsigned int C_global_mem_init[] =
{
#undef C_GLOBAL_MEM_INIT32
#define C_GLOBAL_MEM_INIT32( addr , val ) val,
#include "etpu_c_set_idata.h"
#undef C_GLOBAL_MEM_INIT32
};

// I2C_slave Channel Frame Initialization Data Array
unsigned int C_I2C_slave_frame_init[] =
{
#undef C_I2C_slave_CHAN_FRAME_INIT32
#define C_I2C_slave_CHAN_FRAME_INIT32( addr , val ) val,
#include "etpu_c_set_idata.h"
#undef C_I2C_slave_CHAN_FRAME_INIT32
};

// I2C_master Channel Frame Initialization Data Array
unsigned int C_I2C_master_frame_init[] =
{
#undef C_I2C_master_CHAN_FRAME_INIT32
#define C_I2C_master_CHAN_FRAME_INIT32( addr , val ) val,
#include "etpu_c_set_idata.h"
#undef C_I2C_master_CHAN_FRAME_INIT32
};

