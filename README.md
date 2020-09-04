# I2C-eTPU
This project is an I2C eTPU driver that supports all the major features of the standard. A single master or slave instance uses 4 eTPU channels/pins.  The master driver support includes the following:
- up to 400 KHz operation, or better.  The actual limit depends upon the eTPU clock rate and other functions in the eTPU.
- read, write and combined format transfers
- 7-bit addressing
- START byte via combined format
- unexpected NACKs reported
- clock stretching (synchronization) by slave devices
- interrupt on transfer completion
The slave support includes:
- up to 400 KHz operation, or better.  The actual limit depends upon the eTPU clock rate and other functions in the eTPU.
- programmable 7-bit address
- read, write and combined format transfers
- programmable acceptance of general calls
- handles START bytes
- interrupt on read request and transfer completion
- supports a wait-for-read-data mode wherein the slave driver holds the SCL wire low when a read request is received until the host has filled the read data buffer and alerted that eTPU that the data is ready.

This software is built and simulated/tested by the following tools:
- ETEC C Compiler for eTPU/eTPU2/eTPU2+, version 2.62D, ASH WARE Inc. (older versions ok, but not tested)
- eTPU2+ Development Tool, version 2.72D, ASH WARE Inc.
- System Development Tool, version 2.72D, ASH WARE Inc.

Use of or collaboration on this project is welcomed. For any questions please contact:

ASH WARE Inc. John Diener john.diener@ashware.com
