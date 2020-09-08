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
* FILE NAME: etec_i2c_master.h  
* 
* DESCRIPTION: I2C master class declaration
* 
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial Release.     
*
* $Revision: 1.5 $
*
* Description:
*   This ETEC class, which consists of 4 entry tables and a number of threads,
* provides I2C Master capabilities for the eTPU.  It uses 4 channels/pins to 
* drive/read the I2C SCL and SDA wires.  By using the MPC5xxx output drain setting
* on the output drivers no external hardware is needed to connect to an I2C data bus.
* The I2C Master eTPU driver is configured to use 4 consecutive channels, the first
* two of which must be wired to the SCL line, and the second two to the SDA line.
* Each channel pair connected to an I2C wire consists of an input and output.
*
*   base channel   --------\______ SCL
*   base channel+1 --------/
*   base channel+2 --------\______ SDA
*   base channel+3 --------/
*
* Transfers are controlled by a set of transfer commands.  Multiple commands can be
* part of a single transfer - the driver will issue them in "combined format" with
* repeated START sequences separating each command.  Each command in the command list
* has the following form (total 8 bytes):
*   ----------------------------------------------
*   | header byte |    pointer to data buffer    |
*   ----------------------------------------------
*   |   unused    |   size of transfer in bytes  |
*   ----------------------------------------------
*
* Basic state flow:
*   State 1 (Idle) : totally quiescent, waiting for start transfer HSR to transition
*           to PulseClock.
*   State 2 (PulseClock) : generates one cycle of the clock signal and is serviced
*           on the rising clock edge.  If writing data, it also sets up the proper
*           pin action on the SDA output, or it samples the SDA input pin if reading.
*           After the final bit of a byte is processed the state transitions to
*           ProcessAck.
*   State 3 (ProcessAck) : either reads the ACK/NACK bit, or if responding to a byte
*           read from a slave device, writes the ACK/NACK bit.  To reduce WCTL, this
*           state is broken into two threads.  If there are more bytes to read/write,
*           the function returns to the PulseClock state, otherwise it moves to the
*           states to generated a repeated START or STOP.
*   State 4 (BeginStop) : setup the SDA output to go high while the SCL output
*           is already high, in order to form the STOP.  Goes to FinishStop next.
*   State 5 (FinishStop) : STOP fully complete; host can request another transfer.
*           Returns to Idle.
*   State 6 (FinishRepeatedStart) : sets up SCL and SDA outputs to generate the repeated
*           START sequence.  Goes to PulseClock state next.
*
* ------------
*
* Interfaces for the I2C class:
*
*    Host Service Requests
*
*       HSR 2 : Shutdown (all channels)
*       HSR 4 : Start transfer request (SCL_out channel)
*       HSR 4 : Latch and clear error flags (SCL_in channel)
*       HSR 7 : Initialization (all channels)
*
*    Function Modes
*
*       FM0 not used
*       FM1 not used
*
*    Flags
*
*       SCL_out, SCL_in channels. (flag0,flag1)
*          (0,0) => ready to transfer (idle), transferring data bits
*          (1,0) => process ACK bit
*          (0,1) => issue repeated START
*          (1,1) => issue STOP
*
*    Data (Channel Frame)
*
*       Inputs
*
*          unsigned int24	_tLOW;
*             Low time of the clock signal (SCL)
*          unsigned int24	_tHIGH;
*             High time of the clock signal (SCL); _tLOW + _tHIGH = bit period.  Note that _tHIGH
*             must be greater than or equal to tHD_STA as it is used to produce tHD_STA timing
*          unsigned int24	_tBUF;
*             Minimum time between transfers.
*          unsigned int24	_tSU_STA;
*             START setup time
*          unsigned int24	_tSU_STO;
*             STOP setup time
*          unsigned int24	_tHD_DAT;
*             DATA hold time
*          unsigned int24	_tr_max;
*             The maximum rise time on the SCL line.  This is used to check for and adjust for
*             any clock stretching.  If the SCL detected rising edge lags the output SCL rising 
*             edge by more than this time, the clock timing is adjusted as it is assumed a
*             slave device is stretching the clock.
*          I2C_cmd*			_p_cmd_list;
*             A pointer to the location of the transfer command buffer.  This buffer consists
*             of one or more I2C_cmd command structures each of which defines an individual
*             read or write transfer.
*          unsigned int8	_cmd_cnt;
*             The number of commands in the command buffer.  A value of 2 or mroe indicates
*             a combined format transfer will be generated.
*
*       Outputs
*
*          unsigned int8	_in_use_flag;
*             When non-zero, a transfer is in progress and the host must not request a new one.
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

#ifndef __ETEC_I2C_MASTER_H
#define __ETEC_I2C_MASTER_H

typedef struct
{
	unsigned int8 header;
	unsigned int8* p_buffer;
	unsigned int24 size;
} I2C_cmd;

// I2C class declaration

_eTPU_class I2C_master
{
	// channel frame

private:

	// internal state

	unsigned int24		_working_bit_count;
	unsigned int24		_working_byte;
	unsigned int8		_read_write_flag; // current byte read/write flag
	// used for timing of the clock, this timestamp will take into
	// account clock stretching if necessary
	unsigned int24		_pulse_edge_next_timestamp; // master sets

	I2C_cmd*			_p_current_cmd;
	unsigned int8		_cmd_sent_cnt;
	unsigned int24		_remaining_byte_count;
	unsigned int8*		_p_working_buf;
	unsigned int24		_working_buf_size;
	unsigned int8		_working_buf_read_write_flag;

	unsigned int24		_start_flag; // used for state control when issuing START

public:

	// user inputs

	// timing parameters

	// tlow + thigh = bit period
	unsigned int24		_tLOW;
	unsigned int24		_tHIGH; // note: must also meet _tHD_STA (start hold time) requirement

	// min time between transfers
	unsigned int24		_tBUF;

	// various setup/hold times
	unsigned int24		_tSU_STA;
	//unsigned int24		_tHD_STA; // start (or re-start) hold time (_tHIGH must >= this as _tHIGH is used to generate it)
	unsigned int24		_tSU_STO;
	//unsigned int24		_tSU_DAT; // not needed
	unsigned int24		_tHD_DAT;

	unsigned int24		_tr_max; // maximum rise time

	// data interfaces

	I2C_cmd*			_p_cmd_list;
	unsigned int8		_cmd_cnt;


	// user outputs

	unsigned int8		_in_use_flag;
	unsigned int8		_error_flags;
	unsigned int8		_latched_error_flags;


	// methods/fragments

    _eTPU_fragment PulseClock_fragment();

	// threads

	// initialize/shutdown
	_eTPU_thread InitSCL_out(_eTPU_matches_disabled);
	_eTPU_thread InitSCL_in(_eTPU_matches_disabled);
	_eTPU_thread InitSDA_out(_eTPU_matches_disabled);
	_eTPU_thread InitSDA_in(_eTPU_matches_disabled);
	_eTPU_thread Shutdown(_eTPU_matches_disabled);

	// host request to perform a transfer (SCL_out)
	_eTPU_thread StartTransfer(_eTPU_matches_enabled);
	// host request to latch errors and clear the running error flag state
	_eTPU_thread LatchAndClearErrorFlags(_eTPU_matches_enabled);

	// work threads
	_eTPU_thread PulseClock(_eTPU_matches_enabled);
	_eTPU_thread PulseClockIgnore(_eTPU_matches_enabled);
	_eTPU_thread ProcessAck(_eTPU_matches_enabled);
	_eTPU_thread ProcessAck_Step2(_eTPU_matches_enabled);
	_eTPU_thread ProcessAckIgnore(_eTPU_matches_enabled);
	_eTPU_thread BeginStop(_eTPU_matches_enabled);
	_eTPU_thread FinishStop(_eTPU_matches_enabled);
	_eTPU_thread FinishRepeatedStart(_eTPU_matches_enabled);
	_eTPU_thread FinishRepeatedStartIgnore(_eTPU_matches_enabled);


	// entry tables

	_eTPU_entry_table I2C_SCL_out;
	_eTPU_entry_table I2C_SCL_in;
	_eTPU_entry_table I2C_SDA_out;
	_eTPU_entry_table I2C_SDA_in;
};

#endif
