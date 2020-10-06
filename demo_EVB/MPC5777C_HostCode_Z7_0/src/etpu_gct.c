/*******************************************************************************
*
* Freescale Semiconductor Inc.
* (c) Copyright 2004-2012 Freescale Semiconductor, Inc.
* ALL RIGHTS RESERVED.
*
****************************************************************************//*!
*
* @file    etpu_gct.c_
*
* @author  Marketa Venclikova [nxa17216]
*
* @version 1.3
*
* @date    24-May-2019
*
* @brief   This file contains a template of eTPU module initialization.
*          There are 2 functions to be used by the application:
*          - my_system_etpu_init - initialize eTPU global and channel setting
*          - my_system_etpu_start - run the eTPU
*
*******************************************************************************/

/* architecure defines */
#define ETPU1 0
#define ETPU2 1

/*******************************************************************************
* Includes
*******************************************************************************/
#include "typedefs.h"
#include "etpu_gct.h"        /* private header file */
#if 0 /* include one of etpu_util.h or etpu_util_ext.h */
#include "etpu_util.h"       /* General C Functions for accessing the eTPU */
#endif
#include "etpu_util_ext.h"   /* General C Functions for accessing the eTPU, both eTPU-AB and eTPU-C modules simultaneously */
#include "etpu_set.h"        /* eTPU function set code binary image and other global ddefines */
#if defined(MPC5777C)
#include "mpc5777c_vars.h"
#else
#include "mpc5554_vars.h"    /* chip-specific configuration - must incldue one of these */
#endif
#if 0
#include "etpu_<func1>.h"  /* eTPU function <func1> API */
#include "etpu_<func2>.h"  /* eTPU function <func2> API */
#endif
#include "etpu_i2c.h"
#include "etpu_i2c_master.h"
#include "etpu_i2c_slave.h"
#include "etpu_i2c_common.h"


/*******************************************************************************
* Global variables
*******************************************************************************/
/** @brief   Pointer to the first free parameter in eTPU DATA RAM */
uint32_t *fs_etpu_free_param;
uint32_t *fs_etpu_c_free_param;

/* counter frequencies */
const uint32_t etpu_a_tcr1_freq = 128000000/2;
const uint32_t etpu_a_tcr2_freq = 128000000/8;
const uint32_t etpu_b_tcr1_freq = 128000000/2;
const uint32_t etpu_b_tcr2_freq = 128000000/8;
const uint32_t etpu_c_tcr1_freq = 128000000/2;
const uint32_t etpu_c_tcr2_freq = 128000000/8;

/*******************************************************************************
 * Global eTPU settings - etpu_config structure
 ******************************************************************************/
/** @brief   Structure handling configuration of all global settings */
struct etpu_config_t my_etpu_config =
{
  /* etpu_config.mcr - Module Configuration Register */
  FS_ETPU_GLOBAL_TIMEBASE_DISABLE  /* keep time-bases stopped during intialization (GTBE=0) */
  | FS_ETPU_MISC_DISABLE, /* SCM operation disabled (SCMMISEN=0) */

  /* etpu_config.misc - MISC Compare Register*/
  FS_ETPU_MISC, /* MISC compare value from etpu_set.h */
  
  /* etpu_config.ecr_a - Engine A Configuration Register */
  FS_ETPU_ENTRY_TABLE /* entry table base address = shifted FS_ETPU_ENTRY_TABLE from etpu_set.h */
  | FS_ETPU_CHAN_FILTER_2SAMPLE /* channel filter mode = three-sample mode (CDFC=0) */
  | FS_ETPU_FCSS_DIV2 /* filter clock source selection = div 2 (FSCC=0) */
  | FS_ETPU_FILTER_CLOCK_DIV2 /* filter prescaler clock control = div 2 (FPSCK=0) */
  | FS_ETPU_PRIORITY_PASSING_ENABLE /* scheduler priority passing is enabled (SPPDIS=0) */
  | FS_ETPU_ENGINE_ENABLE, /* engine is enabled (MDIS=0) */

  /* etpu_config.tbcr_a - Time Base Configuration Register A */
  FS_ETPU_TCRCLK_MODE_2SAMPLE /* TCRCLK signal filter control mode = two-sample mode (TCRCF=0x) */
  | FS_ETPU_TCRCLK_INPUT_DIV2CLOCK /* TCRCLK signal filter control clock = div 2 (TCRCF=x0) */
  | FS_ETPU_TCR1CS_DIV2 /* TCR1 clock source = div 2 (TCR1CS=0)*/
  | FS_ETPU_TCR1CTL_DIV2 /* TCR1 source = div 2 (TCR1CTL=2) */
  | FS_ETPU_TCR1_PRESCALER(1) /* TCR1 prescaler = 1 (TCR1P=0) */
  | FS_ETPU_TCR2CTL_DIV8 /* TCR2 source = etpuclk div 8 (TCR2CTL=4) */
  | FS_ETPU_TCR2_PRESCALER(1) /* TCR2 prescaler = 1 (TCR2P=0) */
  | FS_ETPU_ANGLE_MODE_DISABLE, /* TCR2 angle mode is disabled (AM=0) */

  /* etpu_config.stacr_a - Shared Time And Angle Count Register A */
  FS_ETPU_TCR1_STAC_DISABLE /* TCR1 on STAC bus = disabled (REN1=0) */
  | FS_ETPU_TCR1_STAC_CLIENT /* TCR1 resource control = client (RSC1=0) */
  | FS_ETPU_TCR1_STAC_SRVSLOT(0) /* TCR1 server slot = 0 (SRV1=0) */
  | FS_ETPU_TCR2_STAC_DISABLE /* TCR2 on STAC bus = disabled (REN2=0) */
  | FS_ETPU_TCR1_STAC_CLIENT /* TCR2 resource control = client (RSC2=0) */
  | FS_ETPU_TCR2_STAC_SRVSLOT(0), /* TCR2 server slot = 0 (SRV2=0) */

  /* etpu_config.ecr_b - Engine B Configuration Register */
  FS_ETPU_ENTRY_TABLE /* entry table base address = shifted FS_ETPU_ENTRY_TABLE from etpu_set.h */
  | FS_ETPU_CHAN_FILTER_2SAMPLE /* channel filter mode = three-sample mode (CDFC=0) */
  | FS_ETPU_FCSS_DIV2 /* filter clock source selection = div 2 (FSCC=0) */
  | FS_ETPU_FILTER_CLOCK_DIV2 /* filter prescaler clock control = div 2 (FPSCK=0) */
  | FS_ETPU_PRIORITY_PASSING_ENABLE /* scheduler priority passing is enabled (SPPDIS=0) */
  | FS_ETPU_ENGINE_ENABLE, /* engine is enabled (MDIS=0) */

  /* etpu_config.tbcr_b - Time Base Configuration Register B */
  FS_ETPU_TCRCLK_MODE_2SAMPLE /* TCRCLK signal filter control mode = two-sample mode (TCRCF=0x) */
  | FS_ETPU_TCRCLK_INPUT_DIV2CLOCK /* TCRCLK signal filter control clock = div 2 (TCRCF=x0) */
  | FS_ETPU_TCR1CS_DIV2 /* TCR1 clock source = div 2 (TCR1CS=0)*/
  | FS_ETPU_TCR1CTL_DIV2 /* TCR1 source = div 2 (TCR1CTL=2) */
  | FS_ETPU_TCR1_PRESCALER(1) /* TCR1 prescaler = 1 (TCR1P=0) */
  | FS_ETPU_TCR2CTL_DIV8 /* TCR2 source = etpuclk div 8 (TCR2CTL=4) */
  | FS_ETPU_TCR2_PRESCALER(1) /* TCR2 prescaler = 1 (TCR2P=0) */
  | FS_ETPU_ANGLE_MODE_DISABLE, /* TCR2 angle mode is disabled (AM=0) */

  /* etpu_config.stacr_b - Shared Time And Angle Count Register B */
  FS_ETPU_TCR1_STAC_DISABLE /* TCR1 on STAC bus = disabled (REN1=0) */
  | FS_ETPU_TCR1_STAC_CLIENT /* TCR1 resource control = client (RSC1=0) */
  | FS_ETPU_TCR1_STAC_SRVSLOT(0) /* TCR1 server slot = 0 (SRV1=0) */
  | FS_ETPU_TCR2_STAC_DISABLE /* TCR2 on STAC bus = disabled (REN2=0) */
  | FS_ETPU_TCR1_STAC_CLIENT /* TCR2 resource control = client (RSC2=0) */
  | FS_ETPU_TCR2_STAC_SRVSLOT(0), /* TCR2 server slot = 0 (SRV2=0) */

  /* etpu_config.wdtr_a - Watchdog Timer Register A(eTPU2 only) */
  FS_ETPU_WDM_DISABLED /* watchdog mode = disabled */
  | FS_ETPU_WDTR_WDCNT(0), /* watchdog count = 0 */

  /* etpu_config.wdtr_b - Watchdog Timer Register B (eTPU2 only) */
  FS_ETPU_WDM_DISABLED /* watchdog mode = disabled */
  | FS_ETPU_WDTR_WDCNT(0), /* watchdog count = 0 */

  /* etpu_config.scmoff - off SCM Register */
  _SCM_OFF_OPCODE_,
};

#if defined(MPC5777C)
/* eTPU-C module config */
struct etpu_config_t my_etpu_c_config =
{
  /* etpu_config.mcr - Module Configuration Register */
  FS_ETPU_GLOBAL_TIMEBASE_DISABLE  /* keep time-bases stopped during intialization (GTBE=0) */
  | FS_ETPU_MISC_DISABLE, /* SCM operation disabled (SCMMISEN=0) */

  /* etpu_config.misc - MISC Compare Register*/
  FS_ETPU_C_MISC, /* MISC compare value from etpu_set.h */
  
  /* etpu_config.ecr_a - Engine A Configuration Register */
  FS_ETPU_C_ENTRY_TABLE /* entry table base address = shifted FS_ETPU_ENTRY_TABLE from etpu_set.h */
  | FS_ETPU_CHAN_FILTER_2SAMPLE /* channel filter mode = three-sample mode (CDFC=0) */
  | FS_ETPU_FCSS_DIV2 /* filter clock source selection = div 2 (FSCC=0) */
  | FS_ETPU_FILTER_CLOCK_DIV2 /* filter prescaler clock control = div 2 (FPSCK=0) */
  | FS_ETPU_PRIORITY_PASSING_ENABLE /* scheduler priority passing is enabled (SPPDIS=0) */
  | FS_ETPU_ENGINE_ENABLE, /* engine is enabled (MDIS=0) */

  /* etpu_config.tbcr_a - Time Base Configuration Register A */
  FS_ETPU_TCRCLK_MODE_2SAMPLE /* TCRCLK signal filter control mode = two-sample mode (TCRCF=0x) */
  | FS_ETPU_TCRCLK_INPUT_DIV2CLOCK /* TCRCLK signal filter control clock = div 2 (TCRCF=x0) */
  | FS_ETPU_TCR1CS_DIV2 /* TCR1 clock source = div 2 (TCR1CS=0)*/
  | FS_ETPU_TCR1CTL_DIV2 /* TCR1 source = div 2 (TCR1CTL=2) */
  | FS_ETPU_TCR1_PRESCALER(1) /* TCR1 prescaler = 1 (TCR1P=0) */
  | FS_ETPU_TCR2CTL_DIV8 /* TCR2 source = etpuclk div 8 (TCR2CTL=4) */
  | FS_ETPU_TCR2_PRESCALER(1) /* TCR2 prescaler = 1 (TCR2P=0) */
  | FS_ETPU_ANGLE_MODE_DISABLE, /* TCR2 angle mode is disabled (AM=0) */

  /* etpu_config.stacr_a - Shared Time And Angle Count Register A */
  FS_ETPU_TCR1_STAC_DISABLE /* TCR1 on STAC bus = disabled (REN1=0) */
  | FS_ETPU_TCR1_STAC_CLIENT /* TCR1 resource control = client (RSC1=0) */
  | FS_ETPU_TCR1_STAC_SRVSLOT(0) /* TCR1 server slot = 0 (SRV1=0) */
  | FS_ETPU_TCR2_STAC_DISABLE /* TCR2 on STAC bus = disabled (REN2=0) */
  | FS_ETPU_TCR1_STAC_CLIENT /* TCR2 resource control = client (RSC2=0) */
  | FS_ETPU_TCR2_STAC_SRVSLOT(0), /* TCR2 server slot = 0 (SRV2=0) */

  /* etpu_config.ecr_b - Engine B Configuration Register */
  0,

  /* etpu_config.tbcr_b - Time Base Configuration Register B */
  0,

  /* etpu_config.stacr_b - Shared Time And Angle Count Register B */
  0,

  /* etpu_config.wdtr_a - Watchdog Timer Register A(eTPU2 only) */
  0,

  /* etpu_config.wdtr_b - Watchdog Timer Register B (eTPU2 only) */
  0,

  /* etpu_config.scmoff - off SCM Register */
  C_SCM_OFF_OPCODE_,
};
#endif

#if 0
/*******************************************************************************
 * eTPU channel settings - <FUNC1>
 ******************************************************************************/
/** @brief   Initialization of <FUNC1> structures */
struct <func1>_instance_t <func1>_instance =
{
  ETPU_<FUNC1>_CHAN,     /* chan_num */
  FS_ETPU_PRIORITY_HIGH, /* priority */
  ...
  0                      /* *cpba */     /* 0 for automatic allocation */
};

struct <func1>_config_t <func1>_config =
{
  FS_ETPU_<FUNC1>_MODE_A, /* mode */
  ...
};

struct <func1>_states_t <func1>_states;

/*******************************************************************************
 * eTPU channel settings - <FUNC2>
 ******************************************************************************/
/** @brief   Initialization of <FUNC2> structures */
struct <func2>_instance_t <func2>_instance =
{
  ETPU_<FUNC2>_CHAN,     /* chan_num */
  FS_ETPU_PRIORITY_LOW,  /* priority */
  ...
  0                      /* *cpba */     /* 0 for automatic allocation */
};

struct <func2>_config_t <func2>_config =
{
  USEC2TCR1(1000),       /* period */
  ...
};

struct <func2>_states_t <func2>_states;
#endif


/* defined I2C structures */

/* I2C Master */
struct aw_i2c_master_instance_t  i2c_master_instance =
{
    EM_AB,
	ETPU_I2C_MASTER_BASE_CHAN,
    3,
    (void*)0,
    (void*)0,
};
struct aw_i2c_master_config_t    i2c_master_config =
{
    (uint8_t*)0, // will be filled in once allocated
    100, // kHz bit rate
    // detailed timing parameters - not used in this example - just set to 0
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

/* I2C Slave 1 */
struct aw_i2c_slave_instance_t   i2c_slave1_instance =
{
    EM_AB,
	ETPU_I2C_SLAVE1_BASE_CHAN,
    3,
    (void*)0,
    (void*)0,
};
struct aw_i2c_slave_config_t     i2c_slave1_config =
{
    0x64,
    0xfe,
    1,
    ETPU_I2C_SLAVE_DATA_READY_FM0,
    (uint8_t*)0, // read buffer addr will be filled in once allocated
    0, // read buffer size will be filled in once allocated
    (uint8_t*)0, // write buffer addr will be filled in once allocated
    0, // write buffer size will be filled in once allocated
    1250, // tSU_DAT, ns
    4700, // tBUF, ns
};
/* I2C Slave 2 */
struct aw_i2c_slave_instance_t   i2c_slave2_instance =
{
    EM_C,
	ETPU_I2C_SLAVE2_BASE_CHAN,
    3,
    (void*)0,
    (void*)0,
};
struct aw_i2c_slave_config_t     i2c_slave2_config =
{
    0x70,
    0xf0,
    1,
    ETPU_I2C_SLAVE_DATA_READY_FM0,
    (uint8_t*)0, // read buffer addr will be filled in once allocated
    0, // read buffer size will be filled in once allocated
    (uint8_t*)0, // write buffer addr will be filled in once allocated
    0, // write buffer size will be filled in once allocated
    1250, // tSU_DAT, ns
    4700, // tBUF, ns
};

// I2C buffers
extern uint8_t* g_p_i2c_master_cmd_buf;
extern uint8_t* g_p_i2c_master_buf1;
extern uint8_t* g_p_i2c_master_buf2;
extern uint8_t* g_p_i2c_master_buf3;
extern uint8_t* g_p_i2c_master_buf4;
extern uint8_t* g_p_i2c_slave1_read_buf;
extern uint8_t* g_p_i2c_slave1_write_buf;
extern uint8_t* g_p_i2c_slave2_read_buf;
extern uint8_t* g_p_i2c_slave2_write_buf;


/*******************************************************************************
* FUNCTION: my_system_etpu_init
****************************************************************************//*!
* @brief   This function initialize the eTPU module:
*          -# Initialize global setting using fs_etpu_init function
*             and the my_etpu_config structure
*          -# On eTPU2, initialize the additional eTPU2 setting using
*             fs_etpu2_init function
*          -# Initialize channel setting using channel function APIs
*
* @return  Zero or an error code is returned.
*******************************************************************************/
int32_t my_system_etpu_init(void)
{
  int32_t err_code;

  /* Initialization of eTPU DATA RAM */
  fs_memset32_ext((uint32_t*)fs_etpu_data_ram_start, 0, fs_etpu_data_ram_end - fs_etpu_data_ram_start);

  /* Initialization of eTPU global settings */
  err_code = fs_etpu_init_ext(
    EM_AB,
    &my_etpu_config,
    (uint32_t *)etpu_code, sizeof(etpu_code),
    (uint32_t *)etpu_globals, sizeof(etpu_globals));
  if(err_code != 0) return(err_code);

#ifdef FS_ETPU_ARCHITECTURE
 #if FS_ETPU_ARCHITECTURE == ETPU2
  /* Initialization of additional eTPU2-only global settings */
  err_code = fs_etpu2_init_ext(
    EM_AB,
    &my_etpu_config,
  #ifdef FS_ETPU_ENGINE_MEM_SIZE
    FS_ETPU_ENGINE_MEM_SIZE);
  #else
    0);
  #endif
  if(err_code != FS_ETPU_ERROR_NONE) return(err_code);
 #endif
#endif

#if defined(MPC5777C)
/* eTPU-C module */
  /* Initialization of eTPU DATA RAM */
  fs_memset32_ext((uint32_t*)fs_etpu_c_data_ram_start, 0, fs_etpu_c_data_ram_end - fs_etpu_c_data_ram_start);

  /* Initialization of eTPU global settings */
  err_code = fs_etpu_init_ext(
    EM_C,
    &my_etpu_c_config,
    (uint32_t *)etpu_c_code, sizeof(etpu_c_code),
    (uint32_t *)etpu_c_globals, sizeof(etpu_c_globals));
  if(err_code != 0) return(err_code);

#ifdef FS_ETPU_ARCHITECTURE
 #if FS_ETPU_ARCHITECTURE == ETPU2
  /* Initialization of additional eTPU2-only global settings */
  err_code = fs_etpu2_init_ext(
    EM_C,
    &my_etpu_c_config,
  #ifdef FS_ETPU_ENGINE_MEM_SIZE
    FS_ETPU_C_ENGINE_MEM_SIZE);
  #else
    0);
  #endif
  if(err_code != FS_ETPU_ERROR_NONE) return(err_code);
 #endif
#endif
#endif

#if 0
  /* Initialization of eTPU channel settings */
  err_code = fs_etpu_<func1>_init(
    &<func1>_instance,
    &<func1>_config);
  if(err_code != FS_ETPU_ERROR_NONE) return(err_code + (ETPU_<FUNC1>_CHAN<<16));

  err_code = fs_etpu_<func2>_init(
    &<func2>_instance,
    &<func2>_config);
  if(err_code != FS_ETPU_ERROR_NONE) return(err_code + (ETPU_<FUNC2>_CHAN<<16));

  ...
#endif

    // initialize I2C channels
    
    // first need to allocate some buffers
	if (aw_etpu_i2c_allocate_buffer(i2c_master_instance.em, 64, &g_p_i2c_master_cmd_buf))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_master_instance.em, 64, &g_p_i2c_master_buf1))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_master_instance.em, 64, &g_p_i2c_master_buf2))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_master_instance.em, 64, &g_p_i2c_master_buf3))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_master_instance.em, 64, &g_p_i2c_master_buf4))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_slave1_instance.em, 64, &g_p_i2c_slave1_read_buf))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_slave1_instance.em, 64, &g_p_i2c_slave1_write_buf))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_slave2_instance.em, 64, &g_p_i2c_slave2_read_buf))
		return FS_ETPU_ERROR_MALLOC;
	if (aw_etpu_i2c_allocate_buffer(i2c_slave2_instance.em, 64, &g_p_i2c_slave2_write_buf))
		return FS_ETPU_ERROR_MALLOC;
    // set buffer info into config structures
    i2c_master_config.p_cmd_buffer = g_p_i2c_master_cmd_buf;
    i2c_slave1_config.p_read_buffer = g_p_i2c_slave1_read_buf;
    i2c_slave1_config.read_buffer_size = 64;
    i2c_slave1_config.p_write_buffer = g_p_i2c_slave1_write_buf;
    i2c_slave1_config.write_buffer_size = 64;
    i2c_slave2_config.p_read_buffer = g_p_i2c_slave2_read_buf;
    i2c_slave2_config.read_buffer_size = 64;
    i2c_slave2_config.p_write_buffer = g_p_i2c_slave2_write_buf;
    i2c_slave2_config.write_buffer_size = 64;

    // now initialize functions
    err_code = aw_etpu_i2c_master_init(&i2c_master_instance, &i2c_master_config);
    if (err_code != FS_ETPU_ERROR_NONE) return(err_code + (i2c_master_instance.base_chan_num<<16));
    err_code = aw_etpu_i2c_slave_init(&i2c_slave1_instance, &i2c_slave1_config);
    if (err_code != FS_ETPU_ERROR_NONE) return(err_code + (i2c_slave1_instance.base_chan_num<<16));
    err_code = aw_etpu_i2c_slave_init(&i2c_slave2_instance, &i2c_slave2_config);
    if (err_code != FS_ETPU_ERROR_NONE) return(err_code + (i2c_slave2_instance.base_chan_num<<16));

  return(0);
}

/*******************************************************************************
* FUNCTION: my_system_etpu_start
****************************************************************************//*!
* @brief   This function enables channel interrupts, DMA requests and "output
*          disable" feature on selected channels and starts TCR time bases using
*          Global Timebase Enable (GTBE) bit.
* @warning This function should be called after all device modules, including
*          the interrupt and DMA controller, are configured.
*******************************************************************************/
void my_system_etpu_start(void)
{
#if 0
  /* Initialization of Interrupt Enable, DMA Enable
     and Output Disable channel options */
  fs_etpu_set_interrupt_mask_a(ETPU_CIE_A);
  fs_etpu_set_interrupt_mask_b(ETPU_CIE_B);
  fs_etpu_set_dma_mask_a(ETPU_DTRE_A);
  fs_etpu_set_dma_mask_b(ETPU_DTRE_B);
  fs_etpu_set_output_disable_mask_a(ETPU_ODIS_A, ETPU_OPOL_A);
  fs_etpu_set_output_disable_mask_b(ETPU_ODIS_B, ETPU_OPOL_B);
#endif

  /* Synchronous start of all TCR time bases */
  fs_timer_start_ext(EM_AB);
#if defined(MPC5777C)
  fs_timer_start_ext(EM_C);
#endif
}

/*******************************************************************************
 *
 * Copyright:
 *  Freescale Semiconductor, INC. All Rights Reserved.
 *  You are hereby granted a copyright license to use, modify, and
 *  distribute the SOFTWARE so long as this entire notice is
 *  retained without alteration in any modified and/or redistributed
 *  versions, and that such modified versions are clearly identified
 *  as such. No licenses are granted by implication, estoppel or
 *  otherwise under any patents or trademarks of Freescale
 *  Semiconductor, Inc. This software is provided on an "AS IS"
 *  basis and without warranty.
 *
 *  To the maximum extent permitted by applicable law, Freescale
 *  Semiconductor DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
 *  INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
 *  PARTICULAR PURPOSE AND ANY WARRANTY AGAINST INFRINGEMENT WITH
 *  REGARD TO THE SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF)
 *  AND ANY ACCOMPANYING WRITTEN MATERIALS.
 *
 *  To the maximum extent permitted by applicable law, IN NO EVENT
 *  SHALL Freescale Semiconductor BE LIABLE FOR ANY DAMAGES WHATSOEVER
 *  (INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
 *  BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER
 *  PECUNIARY LOSS) ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
 *
 *  Freescale Semiconductor assumes no responsibility for the
 *  maintenance and support of this software
 ******************************************************************************/
/*******************************************************************************
 *
 * REVISION HISTORY:
 *
 * FILE OWNER: Marketa Venclikova [nxa17216]
 * Revision 1.3  2019/05/24  nxa17216
 * Adjusted the  my_system_etpu_init() to use FS_ETPU_GLOBALS_DATA_SIZE for global
 * data size determination whenever it is defined
 *
 * Revision 1.2  2012/04/10  r54529
 * Adjusted to new API style comming with AN4908.
 *  
 * Revision 1.1  2012/04/10  r54529
 * Usage of fs_etpu2_init with autogenerated macro FS_ETPU_ENGINE_MEM_SIZE.
 *  
 * Revision 1.0  2012/02/22  r54529
 * Initial version based on eTPU Graphical Configuration Tool (GCT) output.
 *
 ******************************************************************************/
