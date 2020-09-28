/* main_master_test.c
 *
 * This contains the entry point for the system -ash_main().  System init
 * is performed, then the main app is kicked off.
 *
 * This code performs various tests of the I2C master API.
 */

// for sim environment
#include "isrLib.h"
#include "ScriptLib.h"

// for eTPU/I2C
#include "etpu_util_ext.h"
#include "etpu_gct.h"
#include "etpu_i2c.h"
#include "etpu_i2c_master.h"
#include "etpu_i2c_slave.h"
#include "etpu_i2c_common.h"

/* set to 1 on succesful completion */
uint32_t g_complete_flag = 0;

uint8_t g_i2c_error_flags;

uint8_t* g_p_i2c_master_cmd_buf;
uint8_t* g_p_i2c_master_buf1;
uint8_t* g_p_i2c_master_buf2;
uint8_t* g_p_i2c_master_buf3;
uint8_t* g_p_i2c_master_buf4;
uint8_t* g_p_i2c_slave1_read_buf;
uint8_t* g_p_i2c_slave1_write_buf;
uint8_t* g_p_i2c_slave2_read_buf;
uint8_t* g_p_i2c_slave2_write_buf;


/* helper functions for interrupt polling */
uint32_t aw_etpu_i2c_master_wait_for_done_int(struct aw_i2c_master_instance_t *p_i2c_master_instance)
{
    volatile struct eTPU_struct * eTPU;
    uint8_t channel = p_i2c_master_instance->base_chan_num;
	uint8_t error_flags;

    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	while (1)
	{
		uint32_t cisr;
		if (p_i2c_master_instance->base_chan_num < 32)
    		cisr = eTPU->CISR_A.R & (1 << (channel+ETPU_I2C_MASTER_SCL_OUT_OFFSET)); // look at master SCL_out interrupt bit
        else
    		cisr = eTPU->CISR_B.R & (1 << (channel+ETPU_I2C_MASTER_SCL_OUT_OFFSET)); // look at master SCL_out interrupt bit
		if (cisr)
		{
			/* clear handled interrupts */
            if (p_i2c_master_instance->base_chan_num < 32)
                eTPU->CISR_A.R = cisr;
            else
                eTPU->CISR_B.R = cisr;
			/* get error flags */
			if (aw_etpu_i2c_master_get_running_error_flags(p_i2c_master_instance, &error_flags))
				return 1;
			g_i2c_error_flags |= error_flags;
			break;
		}
	}
	return 0;
}
uint32_t aw_etpu_i2c_slave_wait_for_done_int(struct aw_i2c_slave_instance_t *p_i2c_slave_instance)
{
    volatile struct eTPU_struct * eTPU;
    uint8_t channel = p_i2c_slave_instance->base_chan_num;
	uint8_t error_flags;

    if (p_i2c_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	while (1)
	{
		uint32_t cisr;
		if (p_i2c_slave_instance->base_chan_num < 32)
            cisr = eTPU->CISR_A.R & (1 << (channel+ETPU_I2C_SLAVE_SDA_IN_OFFSET)); // look at slave SDA_in interrupt bit
		else
            cisr = eTPU->CISR_B.R & (1 << (channel+ETPU_I2C_SLAVE_SDA_IN_OFFSET)); // look at slave SDA_in interrupt bit
		if (cisr)
		{
			/* clear handled interrupts */
		    if (p_i2c_slave_instance->base_chan_num < 32)
                eTPU->CISR_A.R = cisr;
			else
                eTPU->CISR_B.R = cisr;
			/* get error flags */
			if (aw_etpu_i2c_slave_get_running_error_flags(p_i2c_slave_instance, &error_flags))
				return 1;
			g_i2c_error_flags |= error_flags;
			break;
		}
	}
	return 0;
}
uint32_t aw_etpu_i2c_slave_wait_for_data_request_int(struct aw_i2c_slave_instance_t *p_i2c_slave_instance)
{
    volatile struct eTPU_struct * eTPU;
    uint8_t channel = p_i2c_slave_instance->base_chan_num;

    if (p_i2c_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	while (1)
	{
		uint32_t cisr;
		if (p_i2c_slave_instance->base_chan_num < 32)
            cisr = eTPU->CISR_A.R & (1 << (channel+ETPU_I2C_SLAVE_SCL_IN_OFFSET)); // look at slave SCL_in interrupt bit
		else
            cisr = eTPU->CISR_B.R & (1 << (channel+ETPU_I2C_SLAVE_SCL_IN_OFFSET)); // look at slave SCL_in interrupt bit
		if (cisr)
		{
			/* clear handled interrupts */
		    if (p_i2c_slave_instance->base_chan_num < 32)
                eTPU->CISR_A.R = cisr;
			else
                eTPU->CISR_B.R = cisr;
			/* don't get errors at this time */
			break;
		}
	}
	return 0;
}


/* main application entry point */
/* w/ GNU, if we name this main, it requires linking with the libgcc.a
   run-time support.  This may be useful with C++ because this extra
   code initializes static C++ objects.  However, this C demo will
   skip it */
int user_main()
{

	/* initialize interrupt support */
	isrLibInit();

    /* for this test, override some of the I2C configuration */
    i2c_master_config.bit_rate_khz = 400;
    i2c_slave1_config.tSU_DAT = 400;
    i2c_slave1_config.tBUF = 1300;

	/* eTPU init */
	if (my_system_etpu_init())
		return 1; // init failed

	/* start the eTPU timers */
	my_system_etpu_start();


	at_time(3000);

	/* test unexpected nacks & buffer overflow */

	/* issue a write request to slave addres that does not exist */
	g_p_i2c_master_buf1[0] = 0x11;
	g_p_i2c_master_buf1[1] = 0x22;
	g_p_i2c_master_buf1[2] = 0x33;
	g_p_i2c_master_buf1[3] = 0x44;
	if (aw_etpu_i2c_master_transmit(&i2c_master_instance, 0x53, 4, g_p_i2c_master_buf1))
		return 1;
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	if (g_i2c_error_flags != ETPU_I2C_MASTER_ACK_FAILED)
		return 1;
	aw_etpu_i2c_master_clear_running_error_flags(&i2c_master_instance);

	at_time(3500);

	/* issue a write request that overflows the slave's write buffer */
	if (aw_etpu_i2c_slave_set_write_buffer(&i2c_slave1_instance, g_p_i2c_slave1_write_buf, 2))
		return 1;
	if (aw_etpu_i2c_master_transmit(&i2c_master_instance, 0x64, 4, g_p_i2c_master_buf1))
		return 1;
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	// buffer overflow on slave does not result in untimely nack
	//if (g_i2c_error_flags != ETPU_I2C_MASTER_ACK_FAILED)
	if (g_i2c_error_flags != 0)
		return 1;
	aw_etpu_i2c_master_clear_running_error_flags(&i2c_master_instance);
	g_i2c_error_flags = 0;
	// next check for slave transfer done interrupt
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave1_instance))
		return 1;
	if (g_i2c_error_flags != ETPU_I2C_SLAVE_BUFFER_OVERFLOW)
		return 1;
	aw_etpu_i2c_slave_clear_running_error_flags(&i2c_slave1_instance);

	at_time(4000);

	/* test master busy */
	g_p_i2c_slave1_read_buf[0] = 0x01;
	g_p_i2c_slave1_read_buf[1] = 0x23;
	g_p_i2c_slave1_read_buf[2] = 0x45;
	g_p_i2c_slave1_read_buf[3] = 0x67;
	g_p_i2c_slave1_read_buf[4] = 0x89;
	g_p_i2c_slave1_read_buf[5] = 0xab;
	g_p_i2c_slave1_read_buf[6] = 0xcd;
	g_p_i2c_slave1_read_buf[7] = 0xef;
	if (aw_etpu_i2c_master_receive(&i2c_master_instance, 0x65, 8, g_p_i2c_master_buf1))
		return 1;
	if (aw_etpu_i2c_master_transmit(&i2c_master_instance, 0x53, 4, g_p_i2c_master_buf1) != FS_ETPU_ERROR_NOT_READY)
		return 1;
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	if ((g_p_i2c_master_buf1[0] != 0x01) || (g_p_i2c_master_buf1[1] != 0x23) || (g_p_i2c_master_buf1[2] != 0x45) || (g_p_i2c_master_buf1[3] != 0x67) ||
		(g_p_i2c_master_buf1[4] != 0x89) || (g_p_i2c_master_buf1[5] != 0xab) || (g_p_i2c_master_buf1[6] != 0xcd) || (g_p_i2c_master_buf1[7] != 0xef) ||
		(g_i2c_error_flags != 0))
		return 1;

	at_time(4500);

	/* test invalid START detection */
	wait_time(100);
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_slave_get_running_error_flags(&i2c_slave1_instance, &g_i2c_error_flags))
		return 1;
	if (g_i2c_error_flags != ETPU_I2C_SLAVE_INVALID_START)
		return 1;
	aw_etpu_i2c_slave_clear_running_error_flags(&i2c_slave1_instance);

	at_time(5000);

	/* test STOP failed detection */
	if (aw_etpu_i2c_master_receive(&i2c_master_instance, 0x65, 1, g_p_i2c_master_buf1))
		return 1;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave1_instance))
		return 1;
	if (g_i2c_error_flags != ETPU_I2C_SLAVE_STOP_FAILED)
		return 1;


	/* TESTING DONE */


	g_complete_flag = 1;
	while (1)
		;

	return 0;
}
