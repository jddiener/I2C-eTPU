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
* FILE NAME: etec_i2c_master.c   
* 
* DESCRIPTION: I2C Master eTPU function(s)
* 
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial Release.     
*
* $Revision: 1.5 $
*
* Description:  Implementation of the I2C master (multiple eTPU functions).
* See the header file etec_i2c_master.h for more details.
*
**************************************************************************/

// verify proper version of compiler toolset is used
#pragma verify_version GE, "2.01A", "use ETEC version 2.01A or newer"
// verify this code uses no stack
#pragma verify_memory_size STACK 0x00 bytes

#include <ETpu_Std.h>

// include common defintions
#include "etpu_i2c_common.h"
// include class declaration
#include "etec_i2c_master.h"

/* provide hint that channel frame base addr same on all chans touched by func */
#pragma same_channel_frame_base I2C_master


// in this I2C solution, the channels are defined as follows:
// base channel     = SCL_out
// base channel + 1 = SCL_in
// base channel + 2 = SDA_out
// base channel + 3 = SDA_in


// entered on SCL_out channel, HSR 7
_eTPU_thread I2C_master::InitSCL_out(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	EnableOutputBuffer();
	SetPinHigh();
	OnMatchA(NoChange);
	OnMatchB(NoChange);
	DetectADisable();
	DetectBDisable();
	MatchBOrderedSingleTransition();
	EnableEventHandling();
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
}

// entered on SCL_in channel, HSR 7
_eTPU_thread I2C_master::InitSCL_in(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	DisableOutputBuffer(); // no output
	OnMatchA(NoChange);  // Needed so output pin does not get toggled
	OnMatchB(NoChange);  // Needed so output pin does not get toggled
	DetectARisingEdge(); // for clock synch/stretching support
	DetectBDisable();
	SingleMatchSingleTransition();
	DisableEventHandling();
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
}

// entered on SDA_out channel, HSR 7
_eTPU_thread I2C_master::InitSDA_out(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	EnableOutputBuffer();
	SetPinHigh();
	OnMatchA(NoChange);
	OnMatchB(NoChange);
	DetectADisable();
	DetectBDisable();
	EitherMatchNonBlockingSingleTransition();
	DisableEventHandling(); // "handled" by SCL_out channel instead
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
}

// entered on SDA_in channnel, HSR 7
_eTPU_thread I2C_master::InitSDA_in(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	DisableOutputBuffer(); // no output
	OnMatchA(NoChange);  // Needed so output pin does not get toggled
	OnMatchB(NoChange);  // Needed so output pin does not get toggled
	DetectADisable();
	DetectBDisable();
	SingleMatchSingleTransition();
	DisableEventHandling(); // "handled" by SCL_out channel instead
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
}

// entered on all channels, HSR 2
_eTPU_thread I2C_master::Shutdown(_eTPU_matches_disabled)
{
	DisableEventHandling();
	SetPinHigh();
	DisableOutputBuffer();
}

// entered on SCL_in channel, HSR 4
_eTPU_thread I2C_master::LatchAndClearErrorFlags(_eTPU_matches_enabled)
{
	_latched_error_flags = _error_flags;
	_error_flags = 0;
}

// entered on SCL_out channel, HSR 4
//
// message transfer requested; issue a START to begin the transfer process
_eTPU_thread I2C_master::StartTransfer(_eTPU_matches_enabled)
{
	// need to pulse SDA low, bringing SCL low during SDA low pulse
	// SDA : ----\_______/--
	// SCL : --------\______

	int24 start_trans_time;

	// need to make sure a transfer is not in progress
	if (_in_use_flag)
	{
		// set busy error, issue interrupt, & exit
		_error_flags |= ETPU_I2C_MASTER_BUSY;
		SetChannelInterrupt();
		return;
	}
	_in_use_flag = 1;
	_start_flag = 1;

	_p_current_cmd = _p_cmd_list;
	_cmd_sent_cnt = 0;

	// setup header byte transfer and prepare for rest of message
	_working_byte = ((unsigned int24)(_p_current_cmd->header)) << 16;
	_working_bit_count = 8;
	_p_working_buf = _p_current_cmd->p_buffer; // always points to next byte
	_working_buf_read_write_flag = _p_current_cmd->header & 1;
	_working_buf_size = _p_current_cmd->size;
	_remaining_byte_count = _p_current_cmd->size;
	_read_write_flag = ETPU_I2C_WRITE_MESSAGE;

	// setup transfer start after (_tbuf) time
	start_trans_time = tcr1 + _tBUF;

	OnMatchA(NoChange);
	OnMatchB(NoChange);
	SetupMatchA(start_trans_time); // dummy match to jive w/ chan mode
	SetupMatchB(start_trans_time);
	_pulse_edge_next_timestamp = start_trans_time;

	// configure SCL_in channel
	chan += (ETPU_I2C_MASTER_SCL_IN_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);
	ClearTransLatch();
	EnableEventHandling();

	// now, setup SDA_out
	chan += (ETPU_I2C_MASTER_SDA_OUT_OFFSET - ETPU_I2C_MASTER_SCL_IN_OFFSET);
	OnMatchA(PinLow);
	SetupMatchA(start_trans_time);
}

// entered on SCL_in channel, rising edge detected
// flag 0 = 0
// flag 1 = 0
//
// set up one clock cycle
// handler gets called at start of each high pulse of clock signal
// entered on SCL_in
_eTPU_thread I2C_master::PulseClock(_eTPU_matches_enabled)
{
	ClearTransLatch();
	if (_working_bit_count == 0)
		SetFlag0(); // go to setup ack mode
	if (erta - _pulse_edge_next_timestamp > _tr_max)
		_pulse_edge_next_timestamp = erta;
	chan += (ETPU_I2C_MASTER_SCL_OUT_OFFSET - ETPU_I2C_MASTER_SCL_IN_OFFSET);
	// issue one clock pulse cycle
	// also set up bit read or write, and see if we need to prep next byte

	// if all bits processed, set up for ack processing
	if (_working_bit_count == 0)
	{
		int24 bit_timestamp;

		SetFlag0(); // go to setup ack mode

		// setup clock cyle
		OnMatchA(PinLow);
		OnMatchB(PinHigh);
		SetupMatchA(_pulse_edge_next_timestamp + _tHIGH);
		bit_timestamp = erta;
		SetupMatchB(erta + _tLOW);
		_pulse_edge_next_timestamp = ertb;

		// process the final bit and/or prepare ack
		if (_read_write_flag == ETPU_I2C_WRITE_MESSAGE)
		{
			// make sure we put out high so we can read ack from receiving slave
			chan += ETPU_I2C_MASTER_SDA_OUT_OFFSET;
			OnMatchA(PinHigh);
			SetupMatchA(bit_timestamp + _tHD_DAT);
		}
		else
		{
			// first, set up ack
			chan += ETPU_I2C_MASTER_SDA_OUT_OFFSET;
			OnMatchA(PinHigh); // default to nack (last byte of read)
			if (_remaining_byte_count)
				OnMatchA(PinLow); // ack
			SetupMatchA(bit_timestamp + _tHD_DAT);

			// next, get last bit of read
			chan += (ETPU_I2C_MASTER_SDA_IN_OFFSET - ETPU_I2C_MASTER_SDA_OUT_OFFSET);

			// read bit now!
			_working_byte <<= 1;
			if (IsCurrentInputPinHigh())
				_working_byte |= 1;
		}
	}
	else
	{
		int24 bit_timestamp;

		_working_bit_count--;
		OnMatchA(PinLow);
		OnMatchB(PinHigh);
		SetupMatchA(_pulse_edge_next_timestamp + _tHIGH);
		bit_timestamp = erta;
		SetupMatchB(erta + _tLOW);
		_pulse_edge_next_timestamp = ertb;

		// setup bit read/write
		if (_read_write_flag == ETPU_I2C_WRITE_MESSAGE)
		{
			chan += ETPU_I2C_MASTER_SDA_OUT_OFFSET;
			_working_byte <<= 1;
			OnMatchA(PinLow);
			if (CC.C)
				OnMatchA(PinHigh);
			SetupMatchA(bit_timestamp + _tHD_DAT);
		}
		else
		{
			chan += (ETPU_I2C_MASTER_SDA_IN_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);

			// read bit now!
			_working_byte <<= 1;
			if (IsCurrentInputPinHigh())
				_working_byte |= 1;
		}
	}
}
// entered on SCL_out channel, match B creating rising edge
// flag 0 = 0
// flag 1 = 0
_eTPU_thread I2C_master::PulseClockIgnore(_eTPU_matches_enabled)
{
	ClearAllLatches();
	if (_start_flag)
	{
		unsigned int24 tmp = erta;
		_start_flag = 0;
		chan += (ETPU_I2C_MASTER_SCL_IN_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);
		erta = tmp;
		PulseClock(); // no return
	}
}


// entered on SCL_in channel, rising edge detected
// flag 0 = 1
// flag 1 = 0
//
// setup ack clock cyle
// this handler gets called at start of ack high clock cycle
_eTPU_thread I2C_master::ProcessAck(_eTPU_matches_enabled)
{
	// this thread does the following:
	// - if receiving ack, check for correct value (set error flag if not valid)
	// - if this was the last byte, setup a STOP
	// - "load" next byte if not the last byte of transfer

	ClearTransLatch();
	if (erta - _pulse_edge_next_timestamp > _tr_max)
		_pulse_edge_next_timestamp = erta;
	chan += (ETPU_I2C_MASTER_SCL_OUT_OFFSET - ETPU_I2C_MASTER_SCL_IN_OFFSET);

	// note : only care about ack val if NOT the last byte (or header byte)
	if ((_read_write_flag == ETPU_I2C_WRITE_MESSAGE) && (_remaining_byte_count || !_working_buf_size))
	{
		// read the ack
		chan += (ETPU_I2C_MASTER_SDA_IN_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);
		if (IsCurrentInputPinHigh())
		{
			_error_flags |= ETPU_I2C_MASTER_ACK_FAILED;
			// make sure the transfer is stopped by zeroing byte count
			_remaining_byte_count = 0;
			//_cmd_sent_cnt = _cmd_cnt; // JDD - continue to next transfer (supports START byte)
		}
		chan += (ETPU_I2C_MASTER_SCL_OUT_OFFSET - ETPU_I2C_MASTER_SDA_IN_OFFSET);
	}
	else if (_read_write_flag == ETPU_I2C_READ_MESSAGE)
		// save off newly read byte
		*_p_working_buf++ = (unsigned int8)_working_byte;
	LinkToChannel(chan);
}
// entered on SCL_out channel, link request
//
// complete the ACK/NACK processing
_eTPU_thread I2C_master::ProcessAck_Step2(_eTPU_matches_enabled)
{
	int24 timestamp;
	ClearLSRLatch();

	// handle delayed case - stretch the bit out some
	timestamp = _pulse_edge_next_timestamp + _tHIGH;
	//if ((int24)(tcr1 - timestamp) > 0)
	//	timestamp = tcr1;

	// on to next byte or repeated START or STOP
	if (_remaining_byte_count)
	{
		// setup the first clock pulse of the next byte
		ClrFlag0();
		OnMatchA(PinLow);
		OnMatchB(PinHigh);
		SetupMatchA(timestamp);
		SetupMatchB(erta + _tLOW);
		_pulse_edge_next_timestamp = ertb;

		_read_write_flag = _working_buf_read_write_flag;
		chan += (ETPU_I2C_MASTER_SDA_OUT_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);
		if (_read_write_flag == ETPU_I2C_WRITE_MESSAGE)
		{
			_working_byte = *_p_working_buf << 17;
			OnMatchA(PinLow);
			if (CC.C)
				OnMatchA(PinHigh);
			SetupMatchA(timestamp + _tHD_DAT);
			_p_working_buf++;
		}
		else
		{
			//_working_byte = 0;
			// make sure SDA_out goes high
			OnMatchA(PinHigh);
			SetupMatchA(timestamp + _tHD_DAT);
		}

		chan += (ETPU_I2C_MASTER_SCL_IN_OFFSET - ETPU_I2C_MASTER_SDA_OUT_OFFSET);
		ClrFlag0();

		_remaining_byte_count--;
		_working_bit_count = 7; // 7 because one bit will have already gone out/in

	}
	else if (++_cmd_sent_cnt < _cmd_cnt)
	{
		// combined format; issue repeated start and set up to read/write next buffer

		// create a repeated START

		// set flags to go to FinishRepeatedStart state next
		ClrFlag0();
		SetFlag1();
		OnMatchA(PinLow);
		OnMatchB(PinHigh);
		SetupMatchA(timestamp);
		SetupMatchB(erta + _tLOW);
		_pulse_edge_next_timestamp = ertb;

		// make sure SDA_out goes high
		chan += ETPU_I2C_MASTER_SDA_OUT_OFFSET;
		OnMatchA(PinHigh);
		SetupMatchA(timestamp + _tHD_DAT);

		chan += (ETPU_I2C_MASTER_SCL_IN_OFFSET - ETPU_I2C_MASTER_SDA_OUT_OFFSET);
		ClrFlag0();
		SetFlag1();

		// setup header & message for the next transfer
		_p_current_cmd++;
		_working_byte = ((unsigned int24)(_p_current_cmd->header)) << 16;
		_working_bit_count = 8;
		_p_working_buf = _p_current_cmd->p_buffer; // always points to next byte
		_working_buf_read_write_flag = _p_current_cmd->header & 1;
		_working_buf_size = _p_current_cmd->size;
		_remaining_byte_count = _p_current_cmd->size;
		_read_write_flag = ETPU_I2C_WRITE_MESSAGE;

	}
	else
	{
		// issue STOP
		// set up SCL_out for STOP
		SetFlag1();
		DisableEventHandling();
		OnMatchA(PinLow);
		OnMatchB(PinHigh);
		SetupMatchA(timestamp);
		SetupMatchB(erta + _tLOW);
		_pulse_edge_next_timestamp = ertb;

		// set up SDA_out for STOP
		chan += (ETPU_I2C_MASTER_SDA_OUT_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);
		OnMatchA(PinLow);
		//OnMatchB(PinHigh);
		SetupMatchA(timestamp + _tHD_DAT);
		//SetupMatchB(timestamp + _tLOW + _tSU_STO);

		chan += (ETPU_I2C_MASTER_SCL_IN_OFFSET - ETPU_I2C_MASTER_SDA_OUT_OFFSET);
		SetFlag1();
	}
}
// entered on SCL_out channel, match B completion
// flag 0 = 1
// flag 1 = 0
_eTPU_thread I2C_master::ProcessAckIgnore(_eTPU_matches_enabled)
{
	ClearAllLatches();
}

// entered on SCL_in channel, rising edge detected
// flag 0 = 1
// flag 1 = 1
//
// create the end of transfer STOP
_eTPU_thread I2C_master::BeginStop(_eTPU_matches_enabled)
{
	unsigned int24 st_timestamp;

	DisableEventHandling();
	ClearTransLatch();
	ClrFlag0();
	ClrFlag1();
	_pulse_edge_next_timestamp = erta;
	chan += (ETPU_I2C_MASTER_SCL_OUT_OFFSET - ETPU_I2C_MASTER_SCL_IN_OFFSET);

	// STOP will be done within _tSU_STO
	ClearMatchALatch();
	ClearMatchBLatch();
	EnableEventHandling();

	// wait additional time until SDA_out signal has completed the stop
	OnMatchA(PinHigh);
	OnMatchB(PinHigh);
	SetupMatchA(_pulse_edge_next_timestamp + _tSU_STO);
	SetupMatchB(erta);
	st_timestamp = erta;
	// finish off SDA_out
	chan += (ETPU_I2C_MASTER_SDA_OUT_OFFSET - ETPU_I2C_MASTER_SCL_OUT_OFFSET);
	OnMatchA(PinHigh);
	SetupMatchA(st_timestamp);
}
// entered on SCL_out channel, match B complete
// flag 0 = 1
// flag 1 = 1
//
// exactly coincides with when SDA_out pin goes high to complete STOP
_eTPU_thread I2C_master::FinishStop(_eTPU_matches_enabled)
{
	ClearMatchALatch();
	ClearMatchBLatch();
	// now fully done with transfer, can issue interrupt
	_in_use_flag = 0;
	ClrFlag0();
	ClrFlag1();
	SetChannelInterrupt();
}

// entered on SCL_in channel, rising edge detected
// flag 0 = 0
// flag 1 = 1
//
// create a repeated START
// entered when SCL_out goes high at beginning of the repeated start.  Sets up
// SDA_out to go low to generated the start, with next thread being a normal
// clock pulse / data bit
_eTPU_thread I2C_master::FinishRepeatedStart(_eTPU_matches_enabled)
{
	ClearTransLatch();
	ClrFlag0();
	ClrFlag1();
	_pulse_edge_next_timestamp = erta;
	chan += (ETPU_I2C_MASTER_SCL_OUT_OFFSET - ETPU_I2C_MASTER_SCL_IN_OFFSET);

	int24 rs_timestamp;
	rs_timestamp = _pulse_edge_next_timestamp + _tSU_STA;

	// switch to normal bit mode (PulseClock handler)
	ClrFlag0();
	ClrFlag1();

	// extend SCL_out high pulse
	OnMatchA(NoChange);
	OnMatchB(NoChange);
	SetupMatchA(rs_timestamp); // dummy match to jive w/ chan mode
	SetupMatchB(rs_timestamp);
	_pulse_edge_next_timestamp = rs_timestamp;
	_start_flag = 1;

	// now, setup SDA_out
	chan += ETPU_I2C_MASTER_SDA_OUT_OFFSET;
	OnMatchA(PinLow);
	SetupMatchA(rs_timestamp);
}
// entered on SCL_out channel, match B complete
// flag 0 = 0
// flag 1 = 1
_eTPU_thread I2C_master::FinishRepeatedStartIgnore(_eTPU_matches_enabled)
{
	ClearAllLatches();
}


// define entry table for I2C clock out channel
// note: ETPD is a don't care, and is set to input to be compatible with
// all MCUs
DEFINE_ENTRY_TABLE(I2C_master, I2C_SCL_out, alternate, inputpin, autocfsr)
{
	//           HSR    LSR M1 M2 PIN F0 F1 vector
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  0, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  1, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  0, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  1, x, Shutdown),
	ETPU_VECTOR3(1,4,5, x,  x, x, x,  x, x, StartTransfer),
	ETPU_VECTOR2(6,7,   x,  x, x, x,  x, x, InitSCL_out),
	ETPU_VECTOR1(0,     1,  0, 0, 0,  x, x, ProcessAck_Step2),
	ETPU_VECTOR1(0,     1,  0, 0, 1,  x, x, ProcessAck_Step2),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 0, PulseClockIgnore),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 0, ProcessAckIgnore),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 1, FinishRepeatedStartIgnore),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 1, FinishStop),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 0, PulseClockIgnore),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 0, ProcessAckIgnore),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 1, FinishRepeatedStartIgnore),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 1, FinishStop),
};

// define entry table for I2C clock in channel
DEFINE_ENTRY_TABLE(I2C_master, I2C_SCL_in, alternate, inputpin, autocfsr)
{
	//           HSR    LSR M1 M2 PIN F0 F1 vector
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  0, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  1, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  0, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  1, x, Shutdown),
	ETPU_VECTOR3(1,4,5, x,  x, x, x,  x, x, LatchAndClearErrorFlags),
	ETPU_VECTOR2(6,7,   x,  x, x, x,  x, x, InitSCL_in),
	ETPU_VECTOR1(0,     1,  0, 0, 0,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     1,  0, 0, 1,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 0, PulseClock),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 0, ProcessAck),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 1, FinishRepeatedStart),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 1, BeginStop),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 0, PulseClock),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 0, ProcessAck),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 1, FinishRepeatedStart),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 1, BeginStop),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 1, _Error_handler_entry),
};

// define entry table for I2C data out channel
// note: ETPD is a don't care, and is set to input to be compatible with
// all MCUs
DEFINE_ENTRY_TABLE(I2C_master, I2C_SDA_out, standard, inputpin, autocfsr)
{
	//           HSR LSR M1 M2 PIN F0 F1 vector
	ETPU_VECTOR1(1,  x,  x, x, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(2,  x,  x, x, x,  x, x, Shutdown),
	ETPU_VECTOR1(3,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(4,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(5,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(6,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(7,  x,  x, x, x,  x, x, InitSDA_out),
	ETPU_VECTOR1(0,  1,  1, 1, x,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  1, 1, x,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 1, x,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 1, x,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  1, 0, x,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  1, 0, x,  1, x, _Error_handler_entry),
};

// define entry table for I2C data in channel
DEFINE_ENTRY_TABLE(I2C_master, I2C_SDA_in, standard, inputpin, autocfsr)
{
	//           HSR LSR M1 M2 PIN F0 F1 vector
	ETPU_VECTOR1(1,  x,  x, x, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(2,  x,  x, x, x,  x, x, Shutdown),
	ETPU_VECTOR1(3,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(4,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(5,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(6,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(7,  x,  x, x, x,  x, x, InitSDA_in),
	ETPU_VECTOR1(0,  1,  1, 1, x,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  1, 1, x,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  0, 1, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 0, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  0,  1, 1, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 0, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 1, x,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  0, 1, x,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  1, 0, x,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(0,  1,  1, 0, x,  1, x, _Error_handler_entry),
};
