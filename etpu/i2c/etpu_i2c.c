/**************************************************************************
* FILE NAME: etpu_i2c.c
* 
* DESCRIPTION: Implementation of API for initializing and controlling the 
* I2C eTPU function.  See the .h file for API documentation.
*
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial version.     
* 2.0     J Diener   04/Sep/20     Modernize API.
*
**************************************************************************/

#include "etpu_util_ext.h"
#include "etpu_i2c.h"
#include "etpu_i2c_common.h"


// returned buffer ptr is in host data space (not eTPU-relative space)
int32_t aw_etpu_i2c_allocate_buffer(
    ETPU_MODULE em,
    uint32_t byte_size,
    uint8_t** buf_ptr)
{
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (!buf_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif

	*buf_ptr = (uint8_t*)fs_etpu_malloc_ext(em, byte_size);
	if (buf_ptr == 0)
		return (FS_ETPU_ERROR_MALLOC);
	return 0;
}


int32_t aw_etpu_i2c_shutdown(
    ETPU_MODULE em,
    uint8_t channel)
{
    volatile struct eTPU_struct * eTPU;
	int32_t hsrr;
	volatile uint32_t cnt; // used to limit the shutdown wait
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
    if (em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	hsrr = eTPU->CHAN[channel  ].HSRR.R + 
		   eTPU->CHAN[channel+1].HSRR.R +
		   eTPU->CHAN[channel+2].HSRR.R +
		   eTPU->CHAN[channel+3].HSRR.R;
	if (hsrr)
		return FS_ETPU_ERROR_NOT_READY;
	eTPU->CHAN[channel  ].HSRR.R = ETPU_I2C_SHUTDOWN_HSR;
	eTPU->CHAN[channel+1].HSRR.R = ETPU_I2C_SHUTDOWN_HSR;
	eTPU->CHAN[channel+2].HSRR.R = ETPU_I2C_SHUTDOWN_HSR;
	eTPU->CHAN[channel+3].HSRR.R = ETPU_I2C_SHUTDOWN_HSR;
	cnt = 0; while ((eTPU->CHAN[channel  ].HSRR.R != 0) && (cnt++ < 1000)) ; // wait for shutdown to occur
	cnt = 0; while ((eTPU->CHAN[channel+1].HSRR.R != 0) && (cnt++ < 1000)) ; // wait for shutdown to occur
	cnt = 0; while ((eTPU->CHAN[channel+2].HSRR.R != 0) && (cnt++ < 1000)) ; // wait for shutdown to occur
	cnt = 0; while ((eTPU->CHAN[channel+3].HSRR.R != 0) && (cnt++ < 1000)) ; // wait for shutdown to occur
	fs_etpu_disable_ext(em, channel   );
	fs_etpu_disable_ext(em, channel+1 );
	fs_etpu_disable_ext(em, channel+2 );
	fs_etpu_disable_ext(em, channel+3 );

	return 0;
}
