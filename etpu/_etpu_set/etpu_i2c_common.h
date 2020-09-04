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
* FILE NAME: etpu_i2c_common.h   
* 
* DESCRIPTION: I2C eTPU function(s) common definitions across host and eTPU
* 
* 
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial Release.     
*
* $Revision: 1.5 $
*
* Description:  This header file contains definitions common to both eTPU code 
* and host-side interface code.
*
**************************************************************************/

#ifndef __ETPU_I2C_COMMON_H
#define __ETPU_I2C_COMMON_H

///////////////////////////////////
// host service requests
///////////////////////////////////

#define ETPU_I2C_INIT_HSR					7 // same for all I2C channels
#define ETPU_I2C_SHUTDOWN_HSR				2 // same for all I2C channels
#define ETPU_I2C_MASTER_START_TRANSFER_HSR	4 // SCL_out channel (master)
#define ETPU_I2C_SLAVE_DATA_READY			4 // SCL_out channel (slave)
#define ETPU_I2C_LATCH_CLEAR_ERRORS_HSR		4 // SCL_in channel (master & slave)

///////////////////////////////////
// function modes
///////////////////////////////////

// master
// none used yet

// slave
// SCL_in (slave)
#define ETPU_I2C_SLAVE_DATA_READY_FM0		0
#define ETPU_I2C_SLAVE_DATA_WAIT_FM0		1

// all channels

///////////////////////////////////
// helpful I2C constants
///////////////////////////////////

#define ETPU_I2C_CHANNELS_USED		4

// channel offsets
// I2C master channel layout
#define ETPU_I2C_MASTER_SCL_OUT_OFFSET	0
#define ETPU_I2C_MASTER_SCL_IN_OFFSET	1
#define ETPU_I2C_MASTER_SDA_OUT_OFFSET	2
#define ETPU_I2C_MASTER_SDA_IN_OFFSET	3
// I2C slave channel layout
#define ETPU_I2C_SLAVE_SCL_IN_OFFSET	0
#define ETPU_I2C_SLAVE_SCL_OUT_OFFSET	1
#define ETPU_I2C_SLAVE_SDA_IN_OFFSET	2
#define ETPU_I2C_SLAVE_SDA_OUT_OFFSET	3

// transfer type (last bit of header byte)
#define ETPU_I2C_RW_MASK			0x01
#define ETPU_I2C_READ_MESSAGE		0x01
#define ETPU_I2C_WRITE_MESSAGE		0x00

// errors
#define ETPU_I2C_MASTER_ACK_FAILED		0x1
#define ETPU_I2C_MASTER_BUSY			0x2

#define ETPU_I2C_SLAVE_INVALID_START	0x10
#define ETPU_I2C_SLAVE_BUFFER_OVERFLOW	0x20
#define ETPU_I2C_SLAVE_STOP_FAILED		0x40


// enable/disable parameter checks in the host interface code
// It is recommended the checks be enabled during development in order
// to catch problems with call parameters, however, for production they may
// be disabled in order to reduce overhead.  To disable, comment out
// the macro defintion below.
#define ETPU_I2C_PARAMETER_CHECK	1

#endif
