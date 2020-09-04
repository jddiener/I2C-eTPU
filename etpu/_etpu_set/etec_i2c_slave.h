/*******************************************************************************
 * Copyright (C) 2015 ASH WARE, Inc.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     ASH WARE, Inc. - initial implementation
 *******************************************************************************/

/**************************************************************************
* FILE NAME: etec_i2c_slave.h  
* 
* DESCRIPTION: I2C slave class declaration
* 
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial Release.     
*
* $Revision: 1.6 $
*
* Description:
*   This ETEC class, which consists of 4 entry tables and a number of threads,
* provides I2C Slave capabilities for the eTPU.  It uses 4 channels/pins to 
* drive/read the I2C SCL and SDA wires.  By using the MPC5xxx output drain setting
* on the output drivers no external hardware is needed to connect to an I2C data bus.
* The I2C Slave eTPU driver is configured to use 4 consecutive channels, the first
* two of which must be wired to the SCL line, and the second two to the SDA line.
* Each channel pair connected to an I2C wire consists of an input and output.
*
*   base channel   --------\______ SCL
*   base channel+1 --------/
*   base channel+2 --------\______ SDA
*   base channel+3 --------/
*
* Basic state flow:
*   State 1 (IdleDetect) : Detect an idle I2C bus (SDA and SCL lines high for at least
*           _tBUF time).  Go to Idle mode once detected.
*   State 2 (Idle, TransferStart_SDA) : Totally quiescent waiting for a falling edge 
*           on the SDA line.  Go to TransferStart_SCL.
*   State 3 (TransferStart_SCL) : Falling edge on SCL detected indicating a START has 
*           been issued on the bus.  Go to DataBitReady next.
*   State 4 (DataBitReady) : entered on rising edges of the SCL line.  Reads a data bit.
*           If this is the first bit of a data word, it also preps for a STOP and
*           repeated START check (next state could be FoundStop or FoundRepeatedStart).
*           If it is the last bit of a data byte do an address check, and if there is a
*           match go to the HandleAck state, else go to IdleDetect.
*   State 5 (OutputDataBit) : entered on a falling edge of the SCL line (exception - 
*           rising edge if a STOP or repeated START is expected).  It outputs the next
*           bit on the SDA line.  If it is on the last bit, the state transitions to
*           HandleAck.
*   State 6 (HandleAck) :  entered on both the rising and falling edge of the ACK/NACK
*           SCL high pulse if the slave is receiving the ACK, or is entered on the falling
*           edge of the last bit clock pulse and the falling edge following ACK/NACK clock
*           pulse.  In the former case, the ACK value is sampled after the rising edge.
*           In the latter case the ACK (pin low) is set at the firt entry and ended at the
*           second.  Once ACK processing is complete, the state transitions to
*           DataBitReady or OutputDataBit (potentially looking for a STOP or repeated
*           START).
*   State 7 (FoundStop) : entered when an SDA rising edge is found while SCL is high, all
*           just after a data byte and ACK have completed.  Goes to Idle state.
*   State 8 (FoundRepeatedStart) : entered when an SDA falling edge is found while SCL is
*           high, all just after a data byte and ACK have completed.  Goes to
*           TransferStart_SCL state.
*
* ------------
*
* Interfaces for the I2C class:
*
*    Host Service Requests
*
*       HSR 2 : Shutdown (all channels)
*       HSR 4 : Read data ready (SCL_out channel)
*       HSR 4 : Latch and clear error flags (SCL_in channel)
*       HSR 7 : Initialization (all channels)
*
*    Function Modes
*
*       FM0 - read data mode (SCL_in channel)
*             0 : read data is presumed to be ready as soon as read command is
*                 received from the master.
*             1 : read requests are to be serviced by the host; the eTPU slave
*                 driver will hold the clock signal low until the host has filled
*                 the read data buffer.
*       FM1 not used
*
*    Flags
*
*       SCL_in channel. (flag0,flag1)
*          (0,0) => idle, or detecting idle
*          (1,0) => read a data bit (also check for STOP, repeated START)
*          (0,1) => output/write a data bit (also check for STOP, repeated START)
*          (1,1) => handle the ACK/NACK bit
*       SDA_in channel (flag0,flag1)
*          (0,x) => everything else
*          (1,x) => detect STOP or repeated START
*
*    Data (Channel Frame)
*
*       Inputs
*
*          unsigned int8	_address;
*             The address of this slave device (last bit is R/W, should be don't care)
*          unsigned int8	_address_mask;
*             A bit mask that is and'ed against the received address to check for a match
*             with _address.  In almost all cases the R/W bit should be masked off, so this
*             value may typically be 0xfe.  If a slave device with address 0x5X needs to be 
*             supported, then _address_mask would be 0x50.
*          unsigned int24	_accept_general_call;
*             If 0, then the general call adress is ignored, otherwise it is accepted and
*             the message received.
*          unsigned int24	_read_buffer_size;
*             The maximum amount of read data available.  If a master device reads more than
*             this number of bytes, 0 is sent out for the remaining data reads and the buffer
*             overflow error is set on this slave.
*          unsigned int24	_write_buffer_size;
*             The maximum amount of write data space available.  If a master device writes 
*             more than this number of bytes the data is ignored and the buffer overflow 
*             error is set on this slave.
*          unsigned int8*	_read_buffer;
*             Buffer from which read transfer data is pulled.
*          unsigned int8*	_write_buffer;
*             Buffer into which write transfer data is written.
*          unsigned int24	_tSU_DAT;
*             Minimum data setup time.  Used in wait-for-read-data mode after read data
*             is ready.  MUST include SDA rise time (tSU_DAT + tr).
*          unsigned int24	_tBUF;
*             The minimum time between transfers.  Used to detect a bus idle condition.
*
*       Outputs
*
*          unsigned int24	_header;
*             A copy of the header (in lower byte) of the last transfer request accepted
*             by this slave.
*          unsigned int24	_byte_cnt;
*             The number of bytes in the last message read or written from/to this slave.
*             Does not include the header byte.  This is a count from an individual
*             transfer.  E.g. a combined format transfer is actually made up of 2 or more
*             individual transfers.
*          unsigned int8	_error_flags;
*             Set of error flags (0 if none) - internal copy.  Use the latch and clear HSR
*             to clear the errors.
*          unsigned int8	_latched_error_flags;
*             Set of error flags (0 if none).  This is a copy of the running _error_flags
*             made when requested by HSR.  The HSR provides a method of coherently reading
*             and clearing the running _error_flags variable from the host.
*
*       Internal State
*
*          (see below)
*
*
**************************************************************************/

#ifndef __ETEC_I2C_SLAVE_H
#define __ETEC_I2C_SLAVE_H

enum I2C_SLAVE_MODE
{
	I2C_SLAVE_MODE_FIND_IDLE,
	I2C_SLAVE_MODE_IDLE,
	I2C_SLAVE_MODE_START_SDA_LOW,
	I2C_SLAVE_MODE_WRITE_HEADER,
	I2C_SLAVE_MODE_WRITE_BYTE,
	I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP,
	I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP2,
	I2C_SLAVE_MODE_READ_BYTE,
	I2C_SLAVE_MODE_READ_FIND_STOP,
	I2C_SLAVE_MODE_READ_FIND_STOP2,
	I2C_SLAVE_MODE_ACK_OUT,
	I2C_SLAVE_MODE_ACK_IN,
	I2C_SLAVE_MODE_ACK_COMPLETE,
	I2C_SLAVE_MODE_IGNORE,
};

_eTPU_class I2C_slave
{
	// channel frame

private:

	// internal state

	enum I2C_SLAVE_MODE	_state;

	unsigned int24		_working_byte;
	unsigned int24		_working_bit_cnt;
	unsigned int24		_working_byte_cnt;
	unsigned int24		_read_write_message;
	unsigned int8*		_p_working_buf;
	unsigned int24		_last_ack;

	unsigned int24		_idle_detect;
	//unsigned int24		_start_timestamp;

public:

	// user inputs

	// slave address settings
	unsigned int8		_address;
	unsigned int8		_address_mask;
	unsigned int24		_accept_general_call;

	// max buffer sizes
	unsigned int24		_read_buffer_size;
	unsigned int24		_write_buffer_size;

	// buffer pointers (read/write nomeclature is from master perspective)
	unsigned int8*		_read_buffer;
	unsigned int8*		_write_buffer;

	unsigned int24		_tSU_DAT; // data setup time
	unsigned int24		_tBUF;    // minimum bus quiesence to be considered in IDLE


	// user outputs

	unsigned int24		_header;
	unsigned int24		_byte_cnt;

	unsigned int8		_error_flags;
	unsigned int8		_latched_error_flags;


	// methods/fragments

	_eTPU_fragment IdleDetectPass();

	// threads

	// initialize/shutdown
	_eTPU_thread InitSCL_out(_eTPU_matches_disabled);
	_eTPU_thread InitSCL_in(_eTPU_matches_disabled);
	_eTPU_thread InitSDA_out(_eTPU_matches_disabled);
	_eTPU_thread InitSDA_in(_eTPU_matches_disabled);
	_eTPU_thread Shutdown(_eTPU_matches_disabled);

	// host request indicating read data has been loaded (SCL_out)
	_eTPU_thread ReadDataReady(_eTPU_matches_enabled);
	// host request to latch errors and clear the running error flag state
	_eTPU_thread LatchAndClearErrorFlags(_eTPU_matches_enabled);

	// common threads

	// SDA_in threads
	_eTPU_thread IdleDetectPass_SDA(_eTPU_matches_enabled);
	_eTPU_thread IdleDetectFail_SDA(_eTPU_matches_enabled);
	_eTPU_thread TransferStart_SDA(_eTPU_matches_enabled);
	_eTPU_thread FoundStop(_eTPU_matches_enabled);
	_eTPU_thread FoundRepeatedStart(_eTPU_matches_enabled);

	// SCL_in threads
	_eTPU_thread IdleDetectPass_SCL(_eTPU_matches_enabled);
	_eTPU_thread IdleDetectFail_SCL(_eTPU_matches_enabled);
	_eTPU_thread TransferStart_SCL(_eTPU_matches_enabled);
	_eTPU_thread DataBitReady(_eTPU_matches_enabled);
	_eTPU_thread OutputDataBit(_eTPU_matches_enabled);
	_eTPU_thread HandleAck(_eTPU_matches_enabled);


	// entry tables

	_eTPU_entry_table I2C_SCL_in;
	_eTPU_entry_table I2C_SCL_out;
	_eTPU_entry_table I2C_SDA_in;
	_eTPU_entry_table I2C_SDA_out;
};

#endif
