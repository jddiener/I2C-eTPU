/**************************************************************************
* FILE NAME: etpu_i2c_slave.h
* 
* DESCRIPTION: API for initializing and controlling the I2C eTPU function
*
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial version.     
* 2.0     J Diener   04/Sep/20     Modernize API.
*
**************************************************************************/

#ifndef __ETPU_I2C_SLAVE_H
#define __ETPU_I2C_SLAVE_H

#include "typedefs.h"	/* type definitions for eTPU interface */

#ifdef __cplusplus
extern "C" {
#endif

/** A structure to represent an instance of I2C_slave
 *  It includes static I2C_slave initialization items. */
struct aw_i2c_slave_instance_t
{
    ETPU_MODULE         em;
    /* base_chan_num - the base channel for the I2C eTPU slave app.  The channels are
     *		defined as follows:
     *			base_chan_num   - SCL_in
     *			base_chan_num+1 - SCL_out
     *			base_chan_num+2 - SDA_in
     *			base_chan_num+3 - SDA_out */
    uint8_t             base_chan_num;
    /* priority - the priority assigned to all the channels, with
     *		a range of 1 (low) to 3 (high).  The SCL_out channel is the primary
     *		that does most of the servicing and work, but they are
     *		assigned the same priority as SCL_out. */
    uint8_t             priority;
    void                *p_cpba;        /* set during initialization */
    void                *p_cpba_pse;    /* set during initialization */
};

/** A structure to represent a configuration of I2C_slave.
 *  It includes I2C_slave configuration items which can be changed in run-time. */
struct aw_i2c_slave_config_t
{
    /* address - the 7-bit address of this I2C slave instance, in bits 7-1.
     *		Works in conjunction with the address_mask value. */
    uint8_t address;
    /* address_mask - an 8-bit mask that is applied to a received header byte
     *		before being compared to the slave address setting.  Bit 0 should
     *		always be 0. */
    uint8_t address_mask;
    /* accept_general_call - if non-zero, then this slave accepts general call
     *		transfers (and responds with an ack).  When 0, general calls are
     *		ignored and nacked. */
    uint8_t accept_general_call;
    /* data_mode - must be one of two values from etpu_i2c_common.h: 
     *		ETPU_I2C_SLAVE_DATA_READY_FM0 or ETPU_I2C_SLAVE_DATA_WAIT_FM0.  This
     *		setting controls how the slave handles a read request.  In "data ready"
     *		mode, it is assumed the data that themaster is reading from the
     *		slave is already in the read buffer (pointed to by p_read_buffer) and
     *		the transaction occurs without any host intervention.  In "data wait"
     *		mode, the host receives an interrupt from the eTPU indicating it must
     *		service the read request - until it does so the I2C slave driver holds
     *		the SCL line low. */
    uint32_t data_mode;
    /* read_buffer_ptr - pointer to the buffer that holds data for a master to
     *		read from (pointer must be in eTPU data space - SDM, but can be in 
     *		host or eTPU pointer address space). */
    uint8_t* p_read_buffer;
    /* read_buffer_size - the maximum size of the read buffer in bytes.  A master 
     *		can read less than this amount, but if it reads more the slave will
     *		respond with 0x00 bytes and set an error flag. */
    uint32_t read_buffer_size;
    /* write_buffer_ptr - pointer to the buffer that will hold any incoming data
     *		from a master write (pointer must be in eTPU data space - SDM, but can
     *		be in host or eTPU pointer address space). */
    uint8_t* p_write_buffer;
    /* write_buffer_size - the maximum size of the write buffer in bytes.  If a 
     *		master write exceeds thsi size, additional bytes are dropped and an
     *		error flag is set. */
    uint32_t write_buffer_size;
    /* tSU_DAT - the data setup time, plus the rise time, in ns.  It is important
     *		to include the rise time or the data signal may not be set correctly
     *		long enough before the SCL line is released in data-wait mode. */
    uint32_t tSU_DAT;
    /* tBUF - the minimum time in ns for bus idle detection.  Transfers must be
     *		seperated by at least tBUF ns or the slave will not properly process
     *		it. */
    uint32_t tBUF;
};

/****************************************************************
 * I2C eTPU app initialization.  This one routine initializes all
 * four eTPU channels that act as an I2C slave.  Four consecutive
 * channels must be used for I2C.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_init(
    struct aw_i2c_slave_instance_t  *p_i2c_slave_instance,
    struct aw_i2c_slave_config_t    *p_i2c_slave_config);


/****************************************************************
 * Configure a new read buffer for an I2C slave instance.
 *
 * buffer_ptr - pointer to the buffer that holds data for a master to
 *		read from (pointer must be in eTPU data space - SDM, but can be in 
 *		host or eTPU pointer address space).
 * size - the maximum size of the read buffer in bytes.  A master 
 *		can read less than this amount, but if it reads more the slave will
 *		respond with 0x00 bytes and set an error flag.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_set_read_buffer(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* buffer_ptr,
    uint32_t size);

/****************************************************************
 * Notify an I2C slave that is waiting for data (data wait mode,
 * read request pending) that it is ready.  This allows the slave
 * to respond to the requesting master.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_issue_data_ready(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance);

/****************************************************************
 * Configure a new write buffer for an I2C slave instance.
 *
 * buffer_ptr - pointer to the buffer that will hold any incoming data
 *		from a master write (pointer must be in eTPU data space - SDM, but can
 *		be in host or eTPU pointer address space).
 * size - the maximum size of the write buffer in bytes.  If a 
 *		master write exceeds thsi size, additional bytes are dropped and an
 *		error flag is set.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_set_write_buffer(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* buffer_ptr,
    uint32_t size);


/****************************************************************
 * Get the key parameters set at the completion of a transfer - header
 * byte (which indicates read/write), size of transfer, and error flags.
 *
 * NOTE: the pointer parameters can be NULL, in which case the value is
 * not read from the eTPU.
 *
 * header_ptr - pointer to the location to which to write the returned
 *		header byte value.
 * size_ptr - pointer to the location at which to write the returned
 *		write data size.  In the case of a buffer overflow error, this
 *		returned value will be larger than the amount of data that is
 *		actually returned, which is limited to the write buffer size.
 * error_flags_ptr - the byte location at which to write the error flags
 *		retrieved from the eTPU.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_get_transfer_status(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* header_ptr,
    uint32_t* size_ptr,
    uint8_t* error_flags_ptr);


/****************************************************************
 * Transfers data written to an I2C slave.from the eTPU data buffer
 * to the specified buffer.  The header and data size are also
 * returned.  The user must ensure the destination buffer is large
 * enough to handle the copy.  This function is to be called after
 * a slave receives a write transfer.
 *
 * header_ptr - pointer to the location to which to write the returned
 *		header byte value.
 * dest_buffer_ptr - pointer to the buffer to which to write the
 *		received write buffer data.
 * size_ptr - pointer to the location at which to write the returned
 *		write data size.  In the case of a buffer overflow error, this
 *		returned value will be larger than the amount of data that is
 *		actually returned, which is limited to the write buffer size.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_get_write_data(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* header_ptr,
    uint8_t* dest_buffer_ptr,
    uint32_t* size_ptr);


/****************************************************************
 * Latch, clear and get the error flags associated with an I2C transfer.
 * The "latch and clear" interface does coherently latch the error
 * flags and clear the running error flags.  Otherwise the get and clear
 * are not coherent, but they generally don't need to be since
 * the user should only be calling this before or after a transfer,
 * not during.  Error flag meanings can be found in etpu_i2c_common.h.
 *
 * error_flags_ptr - the byte location at which to write the error flags
 *		retrieved from the eTPU.
 *
 * Returns failure code, or pass (0).
 ****************************************************************/
int32_t aw_etpu_i2c_slave_latch_clear_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance);
int32_t aw_etpu_i2c_slave_get_running_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* error_flags_ptr);
int32_t aw_etpu_i2c_slave_clear_running_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance);
int32_t aw_etpu_i2c_slave_get_latched_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* error_flags_ptr);
int32_t aw_etpu_i2c_slave_clear_latched_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance);


#ifdef __cplusplus
}
#endif

#endif // __ETPU_I2C_SLAVE_H
