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
* FILE NAME: etec_i2c_slave.c   
* 
* DESCRIPTION: I2C Slave eTPU function(s)
* 
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial Release.     
*
* $Revision: 1.6 $
*
* Description:  Implementation of the I2C slave (multiple eTPU functions).
* See the header file etec_i2c_slave.h for more details.
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
#include "etec_i2c_slave.h"

/* provide hint that channel frame base addr same on all chans touched by func */
#pragma same_channel_frame_base I2C_slave


// master channel   = SCL_in
// master channel+1 = SCL_out
// master channel+2 = SDA_in
// master channel+3 = SDA_out


// interrupts
// if from SCL_in, it is a request to the host to (optionally) update the read buffer in wait-
//    for-data mode.  In data-ready mode, this interrupt is not generated.  However, it may also
//    indicate an invalid STOP, or invalid NACK/STOP combo has been detected - check error flags.
// if from SDA_in, it is notifying the host that a single transfer has completed (read or write)

// entered on SCL_in channel, HSR 7
_eTPU_thread I2C_slave::InitSCL_in(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	DisableOutputBuffer(); // no output
	OnMatchA(NoChange);  // Needed so output pin does not get toggled
	OnMatchB(NoChange);  // Needed so output pin does not get toggled
	DetectAAnyEdge();
	DetectBDisable();
	SingleMatchSingleTransition();
	EnableEventHandling();
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
	// detect idle
	_state = I2C_SLAVE_MODE_FIND_IDLE;
	_idle_detect = 0;
	erta = tcr1 + _tBUF;
	WriteErtAToMatchAAndEnable();
}

// entered on SCL_out channel, HSR 7
_eTPU_thread I2C_slave::InitSCL_out(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	EnableOutputBuffer();
	SetPinHigh();
	OnMatchA(NoChange);
	OnMatchB(NoChange);
	DetectADisable();
	DetectBDisable();
	SingleMatchSingleTransition();
	DisableEventHandling();
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
}

// entered on SDA_in channel, HSR 7
_eTPU_thread I2C_slave::InitSDA_in(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	DisableOutputBuffer(); // no output
	OnMatchA(NoChange);  // Needed so output pin does not get toggled
	OnMatchB(NoChange);  // Needed so output pin does not get toggled
	DetectAAnyEdge();
	DetectBDisable();
	SingleMatchSingleTransition();
	EnableEventHandling();
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
	// detect idle
	_state = I2C_SLAVE_MODE_FIND_IDLE;
	_idle_detect = 0;
	erta = tcr1 + _tBUF;
	WriteErtAToMatchAAndEnable();
}

// entered on SDA_out channel, HSR 7
_eTPU_thread I2C_slave::InitSDA_out(_eTPU_matches_disabled)
{
	DisableMatch(); // end any pending matches
	EnableOutputBuffer();
	SetPinHigh();
	OnMatchA(NoChange);
	OnMatchB(NoChange);
	DetectADisable();
	DetectBDisable();
	SingleMatchSingleTransition();
	DisableEventHandling(); // "handled" by SCL_in channel instead
	ClearAllLatches();
	ClrFlag0();
	ClrFlag1();
}

// entered on all channels, HSR 2
_eTPU_thread I2C_slave::Shutdown(_eTPU_matches_disabled)
{
	DisableEventHandling();
	SetPinHigh();
	DisableOutputBuffer();
}

// entered on SCL_out channel, HSR 4
_eTPU_thread I2C_slave::ReadDataReady(_eTPU_matches_enabled)
{
	// get the new buffer ready
	_working_bit_cnt = 0;
	_p_working_buf = _read_buffer;
	_working_byte = (((unsigned int24)(*_p_working_buf++)) << 16) | 0x8000;
	// data ready, quit holding off master (go high after data hold time)
	OnMatchA(PinHigh);
	erta = tcr1 + _tSU_DAT;
	ClearMatchALatch();
	WriteErtAToMatchAAndEnable();
	chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SCL_OUT_OFFSET);
	OutputDataBit_fragment();
}

// entered on SCL_in channel, HSR 4
_eTPU_thread I2C_slave::LatchAndClearErrorFlags(_eTPU_matches_enabled)
{
	_latched_error_flags = _error_flags;
	_error_flags = 0;
}


// IDLE detection code
// NOTE: for eTPU2, could use PRSS in order to work properly under greater latency conditions

// entered on SCL_in and SDA_in channels, on match A completion with pin high
// flag 0 = 0
// flag 1 = 0
_eTPU_thread I2C_slave::IdleDetectPass_SDA(_eTPU_matches_enabled)
{
	ClearMatchALatch();
	chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_IN_OFFSET);
	IdleDetectPass();
}
_eTPU_thread I2C_slave::IdleDetectPass_SCL(_eTPU_matches_enabled)
{
	ClearMatchALatch();
	IdleDetectPass();
}
// chan must be on SLC_IN when this is called
_eTPU_fragment I2C_slave::IdleDetectPass()
{
	_idle_detect++;
	if (_idle_detect >= 2)
	{
		// if both SDA and SCL idle detected, move to IDLE state
		_state = I2C_SLAVE_MODE_IDLE;
		DetectAFallingEdge();
		ClearTransLatch();
		chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
		DetectAFallingEdge();
		ClearTransLatch();
	}
}

// entered on SDA_in channel, on match A completion with pin low
// flag 0 = 0
// flag 1 = 0
_eTPU_thread I2C_slave::IdleDetectFail_SDA(_eTPU_matches_enabled)
{
    IdleDetectFail_SDA_fragment();
}
_eTPU_fragment I2C_slave::IdleDetectFail_SDA_fragment()
{
	unsigned int24 tmp;
	ClearMatchALatch();
	_state = I2C_SLAVE_MODE_FIND_IDLE;
	_idle_detect = 0;
	erta += _tBUF;
	WriteErtAToMatchAAndEnable();
	DetectAAnyEdge();
	tmp = erta;
	chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_IN_OFFSET);
	erta = tmp;
	ClearMatchALatch();
	WriteErtAToMatchAAndEnable();
	DetectAAnyEdge();
}

// entered on SCL_in channel, on match A completion with pin low
// flag 0 = 0
// flag 1 = 0
_eTPU_thread I2C_slave::IdleDetectFail_SCL(_eTPU_matches_enabled)
{
    IdleDetectFail_SCL_fragment();
}
_eTPU_fragment I2C_slave::IdleDetectFail_SCL_fragment()
{
	unsigned int24 tmp;
	ClearMatchALatch();
	_state = I2C_SLAVE_MODE_FIND_IDLE;
	_idle_detect = 0;
	erta += _tBUF;
	WriteErtAToMatchAAndEnable();
	DetectAAnyEdge();
	tmp = erta;
	chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
	erta = tmp;
	ClearMatchALatch();
	WriteErtAToMatchAAndEnable();
	DetectAAnyEdge();
}


// entered on SDA_in channel, falling edge detected
// flag 0 = 0
// flag 1 = 0
_eTPU_thread I2C_slave::TransferStart_SDA(_eTPU_matches_enabled)
{
	DisableMatch();
	DetectADisable();
	ClearTransLatch();
	ClearMatchALatch();
	chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_IN_OFFSET);
	if ((CurrentInputPin == 0) || (_state != I2C_SLAVE_MODE_IDLE))
		IdleDetectFail_SCL_fragment(); // no return
	//_start_timestamp = erta;
	_state = I2C_SLAVE_MODE_START_SDA_LOW;
}

// entered on SCL_in channel, falling edge detected
// flag 0 = 0
// flag 1 = 0
_eTPU_thread I2C_slave::TransferStart_SCL(_eTPU_matches_enabled)
{
	ClearTransLatch();
	DisableMatch();
	ClearMatchALatch();
	if (_state != I2C_SLAVE_MODE_START_SDA_LOW)
	{
		if (_state == I2C_SLAVE_MODE_IDLE)
			// only set invalid start error if in idle mode
			_error_flags |= ETPU_I2C_SLAVE_INVALID_START;
		IdleDetectFail_SCL_fragment(); // no return
	}
	DetectARisingEdge();
	_last_ack = 0; // clear last ack status
	_state = I2C_SLAVE_MODE_WRITE_HEADER;
	_working_bit_cnt = 0;
	_working_byte_cnt = 0;
	_working_byte = 0;
	SetFlag0();
}

// entered on SCL_in channel, rising edge detected
// flag 0 = 1
// flag 1 = 0
_eTPU_thread I2C_slave::DataBitReady(_eTPU_matches_enabled)
{
	ClearTransLatch();
	if (_state == I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP)
		DetectAFallingEdge();
	else if (_state == I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP2)
	{
		_state = I2C_SLAVE_MODE_WRITE_BYTE;
		DetectARisingEdge();
		// end detect on SDA_in chan
		chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
		DetectADisable();
		ClrFlag0();
		ClearTransLatch();
		return;
	}
	chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET); // switch to SDA_in
	// read SDA_in
	_working_byte <<= 1;
	if (IsCurrentInputPinHigh())
		_working_byte |= 1;
	_working_bit_cnt++;
	if (_state == I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP)
	{
		SetFlag0();
		DetectAAnyEdge();
		ClearTransLatch();
		_state = I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP2;
	}
	if (_working_bit_cnt == 8)
	{
		chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_IN_OFFSET); // switch to SCL_in
		DetectAFallingEdge();
		if (_state == I2C_SLAVE_MODE_WRITE_HEADER)
		{
			// need to set for ack next, if the address matches this address
			if (((_working_byte & _address_mask) == _address) ||
				(!_working_byte && _accept_general_call)) // also handle general call, if configured to accept
			{
				// provide ACK as this slave is the recipient of this message
				SetFlag1();
				_state = I2C_SLAVE_MODE_ACK_OUT;
				_header = (unsigned int8)_working_byte;
				_read_write_message = _working_byte & ETPU_I2C_RW_MASK;
				if (_read_write_message)
					_p_working_buf = _read_buffer;
				else
					_p_working_buf = _write_buffer;
			}
			else if (_working_byte == 0x01) // START byte
			{
				SetFlag1();
				_header = (unsigned int8)_working_byte;
				_read_write_message = 1; // "read"
				_state = I2C_SLAVE_MODE_ACK_IN; // will get a NACK, which will trigger search for STOP/rSTART
			}
			else
			{
				// need to ignore this message; it is destined for some other slave
				ClrFlag0();
				// start up IDLE detection
				IdleDetectFail_SCL_fragment(); // no return
			}
		}
		else // _state == I2C_SLAVE_MODE_WRITE_BYTE
		{
			if (++_working_byte_cnt <= _write_buffer_size)
				*_p_working_buf++ = (unsigned int8)_working_byte;
			else
				_error_flags |= ETPU_I2C_SLAVE_BUFFER_OVERFLOW; // set error, but keep processing
			// provide ACK as this slave is the recipient of this message
			SetFlag1();
			_state = I2C_SLAVE_MODE_ACK_OUT;
		}
	}
}

// entered on SCL_in channel (falling edge, but rising on FIND_STOP check)
// flag 0 = 0
// flag 1 = 1
_eTPU_thread I2C_slave::OutputDataBit(_eTPU_matches_enabled)
{
    OutputDataBit_fragment();
}
_eTPU_fragment I2C_slave::OutputDataBit_fragment()
{
	ClearTransLatch();
	if (_state == I2C_SLAVE_MODE_READ_FIND_STOP)
	{
		DetectAFallingEdge();
		_state = I2C_SLAVE_MODE_READ_FIND_STOP2;
		chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
		SetFlag0();
		DetectAAnyEdge();
		ClearTransLatch();
		return;
	}
	else if (_state == I2C_SLAVE_MODE_READ_FIND_STOP2)
	{
		_error_flags |= ETPU_I2C_SLAVE_STOP_FAILED;
		ClrFlag1();
		chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
		ClrFlag0();
		SetChannelInterrupt(); // from SCL_in channel
		// re-start IDLE detection
		IdleDetectFail_SDA_fragment(); // no return
	}
	chan += (ETPU_I2C_SLAVE_SDA_OUT_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
	_working_byte <<= 1;
	if (CC.C)
		SetPinHigh();
	else
		SetPinLow();
	_working_bit_cnt++;
	if (_working_bit_cnt == 9)
	{
		// prepare to read ACK
		chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_OUT_OFFSET);
		SetFlag0();
		_state = I2C_SLAVE_MODE_ACK_IN;
		DetectARisingEdge();
	}
}

// entered on SCL_in channel
//   rising edge: I2C_SLAVE_MODE_ACK_OUT or I2C_SLAVE_MODE_ACK_COMPLETE
//   falling edge: I2C_SLAVE_MODE_ACK_COMPLETE
// flag 0 = 1
// flag 1 = 1
_eTPU_thread I2C_slave::HandleAck(_eTPU_matches_enabled)
{
	ClearTransLatch();
	if (_state == I2C_SLAVE_MODE_ACK_OUT)
	{
		chan += (ETPU_I2C_SLAVE_SDA_OUT_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
		SetPinLow();
		_state = I2C_SLAVE_MODE_ACK_COMPLETE;
	}
	else if (_state == I2C_SLAVE_MODE_ACK_IN)
	{
		// TBD
		// NOTE: if NACK received, then a STOP or repeated START is expected
		DetectAFallingEdge();
		chan += (ETPU_I2C_SLAVE_SDA_IN_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
		_last_ack = CurrentInputPin;
		_state = I2C_SLAVE_MODE_ACK_COMPLETE;
	}
	else // if (_state == I2C_SLAVE_MODE_ACK_COMPLETE)
	{
		// if in wait for data mode, hold SCL low...
		// note: do no hold on START byte
		if ((FunctionMode0 == ETPU_I2C_SLAVE_DATA_WAIT_FM0) && !_working_byte_cnt && _read_write_message && (_header != 0x01))
		{
			// interrupt only generated in data-wait mode, on SCL_in channel
			// host must fill data buffer and issue a "data ready" host service request
			SetChannelInterrupt(); // interrupt the CPU on read as it needs to update the read data buffer and release SCL
			chan += (ETPU_I2C_SLAVE_SCL_OUT_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
			SetPinLow();
			chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SCL_OUT_OFFSET);
		}
		if (_read_write_message == ETPU_I2C_WRITE_MESSAGE)
		{
			DetectARisingEdge();
			ClrFlag1();
			_state = I2C_SLAVE_MODE_WRITE_BYTE_CHECK_STOP; // check for STOP/START
			_working_bit_cnt = 0;
			_working_byte = 0;
			chan += (ETPU_I2C_SLAVE_SDA_OUT_OFFSET - ETPU_I2C_SLAVE_SCL_IN_OFFSET);
			SetPinHigh();
		}
		else
		{
			ClrFlag0();
			if (_last_ack) // NACK detected
			{
				// if in READ mode and just got a NACK, then a STOP or repeated START is expected next
				DetectARisingEdge();
				_state = I2C_SLAVE_MODE_READ_FIND_STOP;
				return;
			}
			else
			{
				_state = I2C_SLAVE_MODE_READ_BYTE;
				// go right to setup for first bit...
				_working_bit_cnt = 0;
				if (++_working_byte_cnt <= _read_buffer_size)
					_working_byte = (((unsigned int24)(*_p_working_buf++)) << 16) | 0x8000;
				else
				{
					_working_byte = 0x8000; // do not interfere with ACK/NACK from master; just return 0
					_error_flags |= ETPU_I2C_SLAVE_BUFFER_OVERFLOW; // set error, but keep processing
				}
				OutputDataBit_fragment(); // no return
			}
		}
	}
}

// entered on SDA_in channel, rising edge
// flag 0 = 1
// flag 1 = 0
_eTPU_thread I2C_slave::FoundStop(_eTPU_matches_enabled)
{
	// STOP detected
	ClrFlag0();
	_byte_cnt = _working_byte_cnt;
	SetChannelInterrupt(); // from SDA_in channel
	DetectAFallingEdge();
	ClearTransLatch();
	chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_IN_OFFSET);
	ClrFlag0();
	ClrFlag1();
	if (CurrentInputPin == 0)
	{
		_error_flags |= ETPU_I2C_SLAVE_STOP_FAILED;
		IdleDetectFail_SCL_fragment(); // no return
	}
	_state = I2C_SLAVE_MODE_IDLE;
	DetectAFallingEdge();
	ClearTransLatch();
}

// entered on SDA_in channel, falling edge
// flag 0 = 1
// flag 1 = 0
_eTPU_thread I2C_slave::FoundRepeatedStart(_eTPU_matches_enabled)
{
	// repeated START detected
	ClrFlag0();
	DetectADisable();
	ClearTransLatch();
	_byte_cnt = _working_byte_cnt;
	SetChannelInterrupt(); // from SDA_in channel
	chan += (ETPU_I2C_SLAVE_SCL_IN_OFFSET - ETPU_I2C_SLAVE_SDA_IN_OFFSET);
	ClrFlag0();
	ClrFlag1();
	if (CurrentInputPin == 0)
	{
		_error_flags |= ETPU_I2C_SLAVE_INVALID_START;
		IdleDetectFail_SCL_fragment(); // no return
	}
	//_start_timestamp = erta;
	_state = I2C_SLAVE_MODE_START_SDA_LOW;
	DetectAFallingEdge();
}


// define entry table for I2C clock in channel
DEFINE_ENTRY_TABLE(I2C_slave, I2C_SCL_in, alternate, inputpin, autocfsr)
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
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 0, IdleDetectFail_SCL),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 0, IdleDetectPass_SCL),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 0, TransferStart_SCL),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 0, DataBitReady),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 1, OutputDataBit),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 1, HandleAck),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 0, TransferStart_SCL),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 0, DataBitReady),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 1, OutputDataBit),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 1, HandleAck),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 0, TransferStart_SCL),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 0, TransferStart_SCL),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 1, _Error_handler_entry),
};

// define entry table for I2C clock out channel
// note: ETPD is a don't care, and is set to input to be compatible with
// all MCUs
DEFINE_ENTRY_TABLE(I2C_slave, I2C_SCL_out, standard, inputpin, autocfsr)
{
	//           HSR LSR M1 M2 PIN F0 F1 vector
	ETPU_VECTOR1(1,  x,  x, x, 0,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 0,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 1,  0, x, _Error_handler_entry),
	ETPU_VECTOR1(1,  x,  x, x, 1,  1, x, _Error_handler_entry),
	ETPU_VECTOR1(2,  x,  x, x, x,  x, x, Shutdown),
	ETPU_VECTOR1(3,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(4,  x,  x, x, x,  x, x, ReadDataReady),
	ETPU_VECTOR1(5,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(6,  x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(7,  x,  x, x, x,  x, x, InitSCL_out),
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
DEFINE_ENTRY_TABLE(I2C_slave, I2C_SDA_in, alternate, inputpin, autocfsr)
{
	//           HSR    LSR M1 M2 PIN F0 F1 vector
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  0, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  1, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  0, x, Shutdown),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  1, x, Shutdown),
	ETPU_VECTOR3(1,4,5, x,  x, x, x,  x, x, _Error_handler_entry),
	ETPU_VECTOR2(6,7,   x,  x, x, x,  x, x, InitSDA_in),
	ETPU_VECTOR1(0,     1,  0, 0, 0,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     1,  0, 0, 1,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 0, IdleDetectFail_SDA),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 0, IdleDetectPass_SDA),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 0, TransferStart_SDA),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 0, FoundRepeatedStart),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 0, TransferStart_SDA),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 0, FoundStop),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 0, TransferStart_SDA),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 0, TransferStart_SDA),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 1, _Error_handler_entry),
};

// define entry table for I2C data out channel
// note: ETPD is a don't care, and is set to input to be compatible with
// all MCUs
DEFINE_ENTRY_TABLE(I2C_slave, I2C_SDA_out, standard, inputpin, autocfsr)
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
