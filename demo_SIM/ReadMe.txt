This simulation of an MPC5777C demonstrates the functionality of the I2C eTPU drivers
drivers and their host API code.  The demo instantiates a network of an I2C master and
2 I2C slaves, one node on each of the eTPU-A, eTPU-B and eTPU-C engines.

For simulation purposes, the SCL and SDA buses are created by ANDing the SCL_out and
SDA_out from each node, respectively.

The software initializes the eTPU modules and the 3 I2C nodes (1 master, 2 slaves).  It 
then issues a series of transfers that demonstrate many of the driver capabilities.

Executing and stepping through this simulation is by far the easiest way for users to 
familiarize themselves with how the I2C eTPU drivers work.
