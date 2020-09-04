/**************************************************************************
* FILE NAME: etpu_i2c_master.h
* 
* DESCRIPTION: API for initializing and controlling the I2C eTPU function
* (I2C master driver)
*
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial version.     
* 2.0     J Diener   04/Sep/20     Modernize API.
*
**************************************************************************/

#ifndef __ETPU_I2C_MASTER_H
#define __ETPU_I2C_MASTER_H

#include "typedefs.h"	/* type definitions for eTPU interface */
#include "etpu_util_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Type Definitions
*******************************************************************************/

/** A structure to represent an instance of I2C_master
 *  It includes static I2C_master initialization items. */
struct aw_i2c_master_instance_t
{
    ETPU_MODULE         em;
    /* base_chan_num - the base channel for the I2C Master eTPU instance.  The 
     * channels are defined as follows:
     *			base_chan_num   - SCL_out
     *			base_chan_num+1 - SCL_in
     *			base_chan_num+2 - SDA_out
     *			base_chan_num+3 - SDA_in */
    uint8_t             base_chan_num;
    /* priority - the priority assigned to all the channels, with
     *		a range of 1 (low) to 3 (high).  The SCL_out channel is the primary
     *		that does most of the servicing and work, but they are
     *		assigned the same priority as SCL_out. */
    uint8_t             priority;
    void                *p_cpba;        /* set during initialization */
    void                *p_cpba_pse;    /* set during initialization */
};

/** A structure to represent a configuration of I2C_master.
 *  It includes I2C_master configuration items which can be changed in run-time. */
struct aw_i2c_master_config_t
{
    /* p_cmd_buffer - pointer to a buffer in eTPU data memory (SDM) that is to be 
     *		used for setting up transfer commands.  It should be at least 16 bytes
     *		in size in order to allow configuration of a combined transfer. */
    uint8_t             *p_cmd_buffer;
    /* bit_rate_khz - the bit rate in kHz.  By default the initialization function
     *		derives all the various bit timings from this rate.  The
     *		aw_etpu_i2c_master_set_timing() interface can be used to override
     *		the default bit timing. */
    uint32_t            bit_rate_khz;

    // the below are only used by the set_timing() interface and should only be
    // used if the complete control is needed over timing parameters

    /* tLOW - the time in ns of low portion of the SCL clock signal.  tLOW + tHIGH
     *		equals the ideal bit time for the I2C master - the transfers will
     *		run at this rate unless a slave stretches the clock. */
    uint32_t            tLOW;
    /* tHIGH - the time in ns of the high portion of the SCL clock signal.  tLOW + tHIGH
     *		equals the ideal bit time for the I2C master - the transfers will
     *		run at this rate unless a slave stretches the clock. */
    uint32_t            tHIGH;
    /* tBUF - bus free time between a STOP and START condition, in ns. */
    uint32_t            tBUF;
    /* tSU_STA - set-up time for a repeated START, in ns. */
    uint32_t            tSU_STA;
    /* tSU_STO - set-up time for a STOP, in ns. */
    uint32_t            tSU_STO;
    /* tHD_DAT - data hold time (generally can be quite close to 0) in ns. */
    uint32_t            tHD_DAT;
    /* tr_max - maximum rise time for the signals, in ns. */
    uint32_t            tr_max;
};


// define the bitfield order for compiler
#define MSB_BITFIELD_ORDER
//#define LSB_BITFIELD_ORDER

// define the structure of a transfer command
struct aw_etpu_i2c_transfer_cmd
{
#if defined(MSB_BITFIELD_ORDER)
	uint32_t _header : 8;      /* header/address byte */
	uint32_t _p_buffer: 24;    /* pointer to data buffer in eTPU memory */
#elif defined(LSB_BITFIELD_ORDER)
	uint32_t _p_buffer: 24;
	uint32_t _header : 8;
#else
#error Must define either MSB_BITFIELD_ORDER or LSB_BITFIELD_ORDER
#endif
	uint32_t _size;            /* data transfer size in bytes */
};


/****************************************************************
 * I2C Master eTPU app initialization.  This one routine initializes all
 * four eTPU channels that act together as an I2C master.  Four consecutive
 * channels must be used for an I2C master.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_init(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    struct aw_i2c_master_config_t   *p_i2c_master_config);


/****************************************************************
 * Allows direct configuration of each timing parameter used in
 * the I2C master driver.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_set_timing(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    struct aw_i2c_master_config_t   *p_i2c_master_config);

/****************************************************************
 * Transmit a buffer of data to the specified slave address.  When
 * transmission is complete, a channel interrupt will be generated
 * from the base channel.
 *
 * slave_address - the slave address, where it is assumed bit 0
 *		is 0 and will be filled in based upon read/write.
 * buf_size - the size in bytes of the data to be transmitted.  It
 *		does not include the header byte that is made out of the
 *		slave address.
 * buffer_ptr - the buffer from which data is transmitted.  The buffer
 *		must reside in eTPU data memory (SDM).
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_transmit(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    uint8_t slave_address,
    uint32_t buffer_size,
    uint8_t* buffer_ptr);


/****************************************************************
 * Receive a data message from the specified slave address.  When
 * receipt is complete, a channel interrupt will be generated
 * from the base channel.
 *
 * slave_address - the slave address, where it is assumed bit 0
 *		is 0 and will be filled in based upon read/write.
 * buf_size - the size in bytes of the data to be received.  It
 *		does not include the header byte that is made out of the
 *		slave address.
 * buffer_ptr - the buffer into which data is to be received.  The buffer
 *		must reside in eTPU data memory (SDM).
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_receive(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    uint8_t slave_address,
    uint32_t buffer_size,
    uint8_t* buffer_ptr);


/****************************************************************
 * Perform a combined transfer using the specified slave address.
 * The combined transfer allows for 2 back-to-back data transfers,
 * each of which can be a read or write transaction, independently.
 * When the combined transfer completes, a channel interrupt is
 * generated from the base channel of the I2C instance.
 *
 * header1 - the header (address and R/W bit) for the first transfer.
 * buf1_size - the size in bytes of the data to be received or 
 *		transmitted in the first half of the combined transfer. It
 *		does not include the header byte that is made out of the
 *		slave address.
 * buf1_ptr - the buffer for data receipt/transmit for the first
 *		transfer.  Buffer must be in eTPU data memory (SDM).
 * header2 - the header (address and R/W bit) for the second transfer.
 * buf2_size - the size in bytes of the data to be received or 
 *		transmitted in the second half of the combined transfer. It
 *		does not include the header byte that is made out of the
 *		slave address.
 * buf2_ptr - the buffer for data receipt/transmit for the second
 *		transfer.  Buffer must be in eTPU data memory (SDM).
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_combined_transfer(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    uint8_t header1,
    uint32_t buf1_size,
    uint8_t* buf1_ptr,
    uint8_t header2,
    uint32_t buf2_size,
    uint8_t* buf2_ptr);

/****************************************************************
 * Raw interface for setting up any kind of combined transfer.
 * The driver will process each of the cmd_cnt transfer commands
 * in sequence with a repeated START separating them.  When the full
 * transfer completes, a channel interrupt is generated from the base
 * channel of the I2C instance.
 *
 * cmd_buffer_ptr - a buffer in eTPU data memory (SDM) that holds 
 *		cmd_cnt transfer commands (see the type struct aw_etpu_i2c_transfer_cmd
 *		for the layout of each command.
 * cmd_cnt - number of commands in the cmd_buffer.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_raw_transfer(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    struct aw_etpu_i2c_transfer_cmd* cmd_buffer_ptr,
    uint32_t cmd_cnt);


/****************************************************************
 * Latch, clear and get the error flags associated with an I2C transfer.
 * The "latch and clear" interface does coherently latch the error
 * flags and clear the running error flags.  Otherwise the get and clear
 * are not coherent, but they generally don't need to be since
 * the user should only be calling this before or after a transfer,
 * not during.  Error flag meanings can be found in etpu_i2c_common.h.
 *
 * error_flags_ptr - the byte location at which to write the error flags
 *		retrieved from the eTPU.  If a NULL pointer is passed, only
 *		the error flag clear is done.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_master_latch_clear_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance);
int32_t aw_etpu_i2c_master_get_running_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance,
												   uint8_t* error_flags_ptr);
int32_t aw_etpu_i2c_master_clear_running_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance);
int32_t aw_etpu_i2c_master_get_latched_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance,
												   uint8_t* error_flags_ptr);
int32_t aw_etpu_i2c_master_clear_latched_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance);


#ifdef __cplusplus
}
#endif

#endif // __ETPU_I2C_MASTER_H
