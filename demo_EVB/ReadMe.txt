This MPC5777C S32DS project demonstrates the functionality of the I2C eTPU drivers and
their associated host code. The demo instantiates a network of an I2C master and
2 I2C slaves, one node on each of the eTPU-A, eTPU-B and eTPU-C engines.

All the output lines are configured as open drain, and so all SCL input and output lines 
are directly tied together and attached to +5V via a pullup resistor.  Same with all of
SDA I/O.  The demo uses the following channels/pads:

I2C master:
	ETPUA0	- SCL_out
	ETPUA1	- SCL_in
	ETPUA2	- SDA_out
	ETPUA3	- SDA_in

I2C slave 1:
	ETPUB0	- SCL_in
	ETPUB1	- SCL_out
	ETPUB2	- SDA_in
	ETPUB3	- SDA_out

I2C slave 2:
	ETPUC4	- SCL_in
	ETPUC5	- SCL_out
	ETPUC6	- SDA_in
	ETPUC7	- SDA_out

SCL bus connections:
	ETPUA0/ETPUA1/ETPUB0/ETPUB1/ETPUC4/ETPUC5
	PN1/PN0/PR1/PR0/PV5/PV4 (MPC57xx motherboard ports)

SDA bus connections:
	ETPUA2/ETPUA3/ETPUB2/ETPUB3/ETPUC6/ETPUC7
	PN3/PN2/PR3/PR2/PV7/PV6 (MPC57xx motherboard ports)

After initializing the eTPU modules, the demo software loops continually doing the following:
- initializes the 3 I2C nodes, master for 100kHz operation
- a series of transfers of different types are performed and checked for correctness
- shutdown the 3 nodes and pause for a short time (approx. 1ms)
