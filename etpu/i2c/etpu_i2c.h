/**************************************************************************
* FILE NAME: etpu_i2c.h
* 
* DESCRIPTION: API for initializing and controlling the I2C eTPU function
* (functions common to both master and slave drivers)
*
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial version.     
* 2.0     J Diener   04/Sep/20     Modernize API.
*
**************************************************************************/

#ifndef __ETPU_I2C_H
#define __ETPU_I2C_H

#include "typedefs.h"	/* type definitions for eTPU interface */

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 * Allocate a buffer from eTPU Shared Data Memory for use as an I2C
 * transmit/receive buffer.  Once allocated, buffers are not expected
 * to be de-allocated unless the entire eTPU module is re-initialized.
 *
 * byte_size - the size in bytes of the buffer to be allocated.  The 
 *		buffer can be used for both transmit and receive operations.
 * buf_ptr - the pointer to the allocated buffer is returned at this
 *		location.  The pointer value is in host memory space and thus
 *		is ready to be used for reads/writes by host code.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_allocate_buffer(
    ETPU_MODULE em,
    uint32_t byte_size,
	uint8_t** buf_ptr);


/****************************************************************
 * Shut down the I2C instance referenced by the base channel.
 *
 * channel - the I2C eTPU base channel (master or slave)
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_shutdown(
    ETPU_MODULE em,
    uint8_t channel);


#ifdef __cplusplus
}
#endif

#endif // __ETPU_I2C_H
