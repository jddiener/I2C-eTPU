/*******************************************************************************
*
* Freescale Semiconductor Inc.
* (c) Copyright 2004-2012 Freescale Semiconductor, Inc.
* ALL RIGHTS RESERVED.
*
****************************************************************************//*!
*
* @file    etpu_gct.h_
*
* @author  Marketa Venclikova [nxa17216]
*
* @version 1.3
*
* @date    24-May-2019
*
* @brief   This file contains a template of prototypes and definese for
*          the same name .c file.
*
*******************************************************************************/

/*******************************************************************************
* General Macros
*******************************************************************************/
#define ETPU_ENGINE_A_CHANNEL(x)  (x)
#define ETPU_ENGINE_B_CHANNEL(x)  ((x)+64)

#define FS_ETPU_ENTRY_TABLE_ADDR  (((FS_ETPU_ENTRY_TABLE)>>11) & 0x1F)

/******************************************************************************
* Application Constants and Macros
******************************************************************************/
#define SYS_FREQ_HZ                                                       200E6
#define TCR1_FREQ_HZ                                            (SYS_FREQ_HZ/2)
#define MSEC2TCR1(x)                                 (TCR1_FREQ_HZ/1E3*(x)/1E0)
#define USEC2TCR1(x)                                 (TCR1_FREQ_HZ/1E3*(x)/1E3)
#define NSEC2TCR1(x)                                 (TCR1_FREQ_HZ/1E3*(x)/1E6)
#define UFRACT24(x)                                              ((x)*0xFFFFFF)
#define SFRACT24(x)                                              ((x)*0x7FFFFF)

#define FRACT2RPM(speed_fract, period_ns) \
         ((speed_fract) * TCR1_FREQ_HZ * 60 / (0x1000000 * NSEC2TCR1(period_ns))
#define RPM2FRACT(rpm, period_ns) \
                ((rpm) * NSEC2TCR1(period_ns) / TCR1_FREQ_HZ * (0x1000000 / 60))

#define DEG2FRACT(deg)                                       ((deg)*0x200000/45)
#define FRACT2DEG(angle_fract)                             ((angle)*45/0x200000)
/*******************************************************************************
* Define Functions to Channels
*******************************************************************************/
#define ETPU_SPI_MASTER1_SS_CHAN    ETPU_ENGINE_A_CHANNEL(1)
#define ETPU_SPI_MASTER1_MISO_CHAN  ETPU_ENGINE_A_CHANNEL(3)
#define ETPU_SPI_MASTER1_SCLK_CHAN  ETPU_ENGINE_A_CHANNEL(4)
#define ETPU_SPI_MASTER1_MOSI_CHAN  ETPU_ENGINE_A_CHANNEL(5)

#define ETPU_SPI_SLAVE1_SS_CHAN     ETPU_ENGINE_A_CHANNEL(6)
#define ETPU_SPI_SLAVE1_MISO_CHAN   ETPU_ENGINE_A_CHANNEL(7)
#define ETPU_SPI_SLAVE1_SCLK_CHAN   ETPU_ENGINE_A_CHANNEL(8)
#define ETPU_SPI_SLAVE1_MOSI_CHAN   ETPU_ENGINE_A_CHANNEL(9)

/*******************************************************************************
* Define Interrupt Enable, DMA Enable and Output Disable
*******************************************************************************/
#define ETPU_CIE_A    (1<<ETPU_<FUNC1>_CHAN)  /* enable interrupt on channel ETPU_<FUNC1>_CHAN */
#define ETPU_DTRE_A   (1<<ETPU_<FUNC2>_CHAN)  /* enable DMA request on channel ETPU_<FUNC2>_CHAN */
#define ETPU_ODIS_A   0x00000000
#define ETPU_OPOL_A   0x00000000
#define ETPU_CIE_B    0x00000000
#define ETPU_DTRE_B   0x00000000
#define ETPU_ODIS_B   0x00000000
#define ETPU_OPOL_B   0x00000000

/******************************************************************************
* Global Variables Access
******************************************************************************/

#if 0
/* Global <FUNC1> structures defined in etpu_gct.c */
extern struct <func1>_instance_t <func1>_instance;
extern struct <func1>_config_t   <func1>_config;
extern struct <func1>_states_t   <func1>_states;

/* Global <FUNC2> structures defined in etpu_gct.c */
extern struct <func2>_instance_t <func2>_instance;
extern struct <func2>_config_t   <func2>_config;
extern struct <func2>_states_t   <func2>_states;
#endif

/* I2C Master */
extern struct aw_i2c_master_instance_t  i2c_master_instance;
extern struct aw_i2c_master_config_t    i2c_master_config;

/* I2C Slave 1 */
extern struct aw_i2c_slave_instance_t   i2c_slave1_instance;
extern struct aw_i2c_slave_config_t     i2c_slave1_config;
/* I2C Slave 2 */
extern struct aw_i2c_slave_instance_t   i2c_slave2_instance;
extern struct aw_i2c_slave_config_t     i2c_slave2_config;


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
int32_t my_system_etpu_init (void);
void    my_system_etpu_start(void);

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
 * data size determination whenever it is defined.
 * Added useful macros.
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