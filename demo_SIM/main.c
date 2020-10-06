/* main.c
 *
 * This is the entry point for the system.  System init
 * is performed, then the main app is kicked off.
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
	uint8_t header;
	uint32_t size;
	uint8_t i2c_local_buffer[64];

	/* initialize interrupt support */
	isrLibInit();
	
	/* override some settings here before initialization in order to place one slave on 
	   eTPU-B, and the other on eTPU-C */
	i2c_slave1_instance.em = EM_AB;
	i2c_slave1_instance.base_chan_num = 64; /* eTPU-B channel 0 */
	i2c_slave2_instance.em = EM_C;
	i2c_slave2_instance.base_chan_num = 0; /* eTPU-C channel 0 */

	/* eTPU init */
	if (my_system_etpu_init())
		return 1; // init failed

	/* start the eTPU timers */
	my_system_etpu_start();

	at_time(3000);

	/* issue a transmit request */
	g_p_i2c_master_buf1[0] = 0x11;
	g_p_i2c_master_buf1[1] = 0x22;
	g_p_i2c_master_buf1[2] = 0x33;
	g_p_i2c_master_buf1[3] = 0x44;
	if (aw_etpu_i2c_master_transmit(&i2c_master_instance, 0x64, 4, g_p_i2c_master_buf1))
		return 1;

	/* need to wait for completion by polling on CISR - normally handled */
	/* asynch by channel interrupt, but can't do that in sim environment */

	// first, check for master transfer complete done
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	// next check for slave transfer done interrupt
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave1_instance))
		return 1;
	// check that received data is as expected, and no errors occurred
	if ((g_p_i2c_slave1_write_buf[0] != 0x11) || (g_p_i2c_slave1_write_buf[1] != 0x22) || 
		(g_p_i2c_slave1_write_buf[2] != 0x33) || (g_p_i2c_slave1_write_buf[3] != 0x44) ||
		(g_i2c_error_flags != 0))
		return 1;
	aw_etpu_i2c_master_clear_running_error_flags(&i2c_master_instance);
	aw_etpu_i2c_slave_clear_running_error_flags(&i2c_slave1_instance);

	// check that retrieval of write buffer api works
	if (aw_etpu_i2c_slave_get_write_data(&i2c_slave1_instance, &header, i2c_local_buffer, &size))
		return 1;
	if ((i2c_local_buffer[0] != 0x11) || (i2c_local_buffer[1] != 0x22) || 
		(i2c_local_buffer[2] != 0x33) || (i2c_local_buffer[3] != 0x44) ||
		(size != 4) || (header != 0x64))
		return 1;

	at_time(4000);

	/* issue a read request */
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

	/* need to wait for completion by polling on CISR - normally handled */
	/* asynch by channel interrupt, but can't do that in sim environment */

	// first, check for master transfer complete done
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	// next check for slave transfer done interrupt
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave1_instance))
		return 1;

	if ((g_p_i2c_master_buf1[0] != 0x01) || (g_p_i2c_master_buf1[1] != 0x23) || (g_p_i2c_master_buf1[2] != 0x45) || (g_p_i2c_master_buf1[3] != 0x67) ||
		(g_p_i2c_master_buf1[4] != 0x89) || (g_p_i2c_master_buf1[5] != 0xab) || (g_p_i2c_master_buf1[6] != 0xcd) || (g_p_i2c_master_buf1[7] != 0xef) ||
		(g_i2c_error_flags != 0))
		return 1;


	at_time(5000);

	/* issue a combined transfer request */
	g_p_i2c_master_buf1[0] = 0x40;
	g_p_i2c_master_buf1[1] = 0x20;
	g_p_i2c_slave1_read_buf[0] = 0xbe;
	g_p_i2c_slave1_read_buf[1] = 0xef;
	if (aw_etpu_i2c_master_combined_transfer(&i2c_master_instance, 0x64, 2, g_p_i2c_master_buf1,
		0x65, 2, g_p_i2c_master_buf2))
		return 1;

	// first, check for master transfer complete done
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	// next check for slave transfer done interrupt
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave1_instance))
		return 1;

	/* verify that data was transferred correctly */
	if ((g_p_i2c_slave1_write_buf[0] != 0x40) || (g_p_i2c_slave1_write_buf[1] != 0x20))
		return 1;
	if ((g_p_i2c_master_buf2[0] != 0xbe) || (g_p_i2c_master_buf2[1] != 0xef))
		return 1;
	if (g_i2c_error_flags != 0)
		return 1;


	at_time(6000);

	/* test wait for read */

    /* need to switch slave to data wait mode for this transfer */
    {
        volatile struct eTPU_struct * eTPU;
        if (i2c_slave2_instance.em == EM_AB)
        {
            eTPU = eTPU_AB;
        }
        else
        {
            eTPU = eTPU_C;
        }
        eTPU->CHAN[i2c_slave2_instance.base_chan_num + ETPU_I2C_SLAVE_SCL_IN_OFFSET].SCR.R = ETPU_I2C_SLAVE_DATA_WAIT_FM0;
    }
	
	aw_etpu_i2c_slave_clear_running_error_flags(&i2c_slave2_instance);
	((struct aw_etpu_i2c_transfer_cmd*)g_p_i2c_master_cmd_buf)->_header = 0x75;
	((struct aw_etpu_i2c_transfer_cmd*)g_p_i2c_master_cmd_buf)->_p_buffer = (uint32_t)g_p_i2c_master_buf1 & 0x3fff;
	((struct aw_etpu_i2c_transfer_cmd*)g_p_i2c_master_cmd_buf)->_size = 3;
	if (aw_etpu_i2c_master_raw_transfer(&i2c_master_instance, (struct aw_etpu_i2c_transfer_cmd*)g_p_i2c_master_cmd_buf, 1))
		return 1;

	/* wait for read interrupt, then supply read data and signal data ready */
	if (aw_etpu_i2c_slave_wait_for_data_request_int(&i2c_slave2_instance))
		return 1;
	/* write read data, and signal ready */
	g_p_i2c_slave2_read_buf[0] = 0xaa;
	g_p_i2c_slave2_read_buf[1] = 0x55;
	g_p_i2c_slave2_read_buf[2] = 0xaa;
	aw_etpu_i2c_slave_issue_data_ready(&i2c_slave2_instance);

	// check for master transfer complete done
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave2_instance))
		return 1;

	/* verify that data was transferred correctly */
	if ((g_p_i2c_master_buf1[0] != 0xaa) || (g_p_i2c_master_buf1[1] != 0x55) || (g_p_i2c_master_buf1[2] != 0xaa))
		return 1;
	if (g_i2c_error_flags != 0)
		return 1;

    /* switch slave back to data ready mode */
    {
        volatile struct eTPU_struct * eTPU;
        if (i2c_slave2_instance.em == EM_AB)
        {
            eTPU = eTPU_AB;
        }
        else
        {
            eTPU = eTPU_C;
        }
        eTPU->CHAN[i2c_slave2_instance.base_chan_num + ETPU_I2C_SLAVE_SCL_IN_OFFSET].SCR.R = ETPU_I2C_SLAVE_DATA_READY_FM0;
    }


	at_time(7000);

	/* test general call */
	g_p_i2c_master_buf3[0] = 0xcc;
	if (aw_etpu_i2c_master_transmit(&i2c_master_instance, 0x00, 1, g_p_i2c_master_buf3))
		return 1;

	/* need to wait for completion by polling on CISR - normally handled */
	/* asynch by channel interrupt, but can't do that in sim environment */

	// first, check for master transfer complete done
	g_i2c_error_flags = 0;
	if (aw_etpu_i2c_master_wait_for_done_int(&i2c_master_instance))
		return 1;
	// next check for slave transfer done interrupt (both slaves)
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave1_instance))
		return 1;
	if (aw_etpu_i2c_slave_wait_for_done_int(&i2c_slave2_instance))
		return 1;

	/* verify that both slaves received the general call */
	if (aw_etpu_i2c_slave_get_write_data(&i2c_slave1_instance, &header, i2c_local_buffer, &size))
		return 1;
	if ((i2c_local_buffer[0] != 0xcc) || (size != 1) || (header != 0x00))
		return 1;
	if (aw_etpu_i2c_slave_get_write_data(&i2c_slave2_instance, &header, i2c_local_buffer, &size))
		return 1;
	if ((i2c_local_buffer[0] != 0xcc) || (size != 1) || (header != 0x00))
		return 1;



	at_time(8000);

	/* shutdown the I2C drivers */
	if (aw_etpu_i2c_shutdown(i2c_master_instance.em, i2c_master_instance.base_chan_num))
		return 1;
	if (aw_etpu_i2c_shutdown(i2c_slave1_instance.em, i2c_slave1_instance.base_chan_num))
		return 1;
	if (aw_etpu_i2c_shutdown(i2c_slave2_instance.em, i2c_slave2_instance.base_chan_num))
		return 1;



	/* TESTING DONE */


	g_complete_flag = 1;
	while (1)
		;

	return 0;
}
