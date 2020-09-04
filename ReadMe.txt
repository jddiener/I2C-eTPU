/*******************************************************************************
 * Copyright (C) 2011-2016 ASH WARE, Inc. All rights reserved. 
 * This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v20.html
 *
 * Contributors:
 *     ASH WARE, Inc. - initial implementation
 *******************************************************************************/

This is release 2.00 of the I2C eTPU Drivers.

This package contains the driver eTPU source code, host layer driver source code, 
a binary image and the auto-generated files to be used by the host for interfacing 
with the eTPU drivers.  The eTPU software has been built with the ETEC C Compiler 
from ASH WARE.


Tools Used
==========
ASH WARE eTPU2+ (& System) DevTool,    Version 2.72D
ASH WARE ETEC eTPU C Compiler Toolkit, version 2.62D


Package Contents
================
The main directory structure of the package is as follows:
.       - contains the sample eTPU module configuration files.
.\doc	- documentation
.\etpu	- eTPU code and host utilities and I2C API to interact with eTPU drivers
.\include - MCU-specific include files to support eTPU module access
.\tests	- eTPU and system simulation tests/examples for the drivers

Key files included in this repository:

.\etpu\_etpu_set\etec_i2c_master.c	// I2C master eTPU driver code
.\etpu\_etpu_set\etec_i2c_master.h
.\etpu\_etpu_set\etec_i2c_slave.c	// I2C slave eTPU driver code
.\etpu\_etpu_set\etec_i2c_slave.h
.\etpu\_etpu_set\etpu_i2c_common.h	// header file of definitions common to eTPU and host
.\etpu\_etpu_set\etpu_set_*.*           // eTPU build outputs

.\etpu-i2c\etpu_i2c.c			// host-side driver source files
.\etpu-i2c\etpu_i2c.h
.\etpu-i2c\etpu_i2c_master.c
.\etpu-i2c\etpu_i2c_master.h
.\etpu-i2c\etpu_i2c_slave.c
.\etpu-i2c\etpu_i2c_slave.h

Tests/etpu/*.*				// standalone eTPU tests
Tests/system/*.*			// System tests (tests host layer driver code)


Code Size
=========
Compile and see etpu_set.map or etpu_set_ana.html for details.


Change History
==============
Release 2.00 : (2020-Sep-04) modernize the structure and API, move to EPL2.0 and github
Release 1.11 : (2016-Aug-23) fix some minor scripting issues
Release 1.10 : (2016-Jan-06) update to use DevTool and MtDt
Release 1.00 : (2011-Dec-01) initial driver release
