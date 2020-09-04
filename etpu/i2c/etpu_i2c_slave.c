/**************************************************************************
* FILE NAME: etpu_i2c_slave.c
* 
* DESCRIPTION: Implementation of API for initializing and controlling the 
* I2C Slave eTPU function.  See the .h file for API documentation.
*
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial version.     
* 2.0     J Diener   04/Sep/20     Modernize API.
*
**************************************************************************/

#include "etpu_util_ext.h"
#include "etpu_i2c_slave.h"
#include "etpu_i2c_common.h"
#include "etpu_set_defines.h"


int32_t aw_etpu_i2c_slave_init(
    struct aw_i2c_slave_instance_t  *p_i2c_slave_instance,
    struct aw_i2c_slave_config_t    *p_i2c_slave_config)
{
    volatile struct eTPU_struct * eTPU;
	uint32_t *pba;	/* parameter base address for channel */
	uint32_t tcr1_freq;
	uint32_t i2c_slave_cpba;
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
	uint8_t priority = p_i2c_slave_instance->priority;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!priority || (priority > 3))
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
        if (p_i2c_slave_instance->base_chan_num < 32)
        {
            tcr1_freq = etpu_a_tcr1_freq;
        }
        else
        {
            tcr1_freq = etpu_b_tcr1_freq;
        }
    }
    else
    {
        eTPU = eTPU_C;
        tcr1_freq = etpu_c_tcr1_freq;
    }

	/* Disable channels to assign function safely */
	fs_etpu_disable_ext(p_i2c_slave_instance->em, channel );
	fs_etpu_disable_ext(p_i2c_slave_instance->em, channel + 1 );
	fs_etpu_disable_ext(p_i2c_slave_instance->em, channel + 2 );
	fs_etpu_disable_ext(p_i2c_slave_instance->em, channel + 3 );

	/* allocate a channel frame if not already done so */
	/* NOTE: this means that re-initialization of this channel (2 channels) */
	/* must re-use the same channel frame allocation */
	if (eTPU->CHAN[channel].CR.B.CPBA == 0 )
	{
		/* get parameter RAM
		number of parameters passed from eTPU C code */
		pba = fs_etpu_malloc_ext(p_i2c_slave_instance->em, _FRAME_SIZE_I2C_slave_);
		if (pba == 0)
			return (FS_ETPU_ERROR_MALLOC);
	}
	else /*set pba to what is in the CR register*/
	{
		pba = fs_etpu_get_cpba_ext(p_i2c_slave_instance->em, channel);
	}
	// the channel group shares the same channel frame
	i2c_slave_cpba = ((uint32_t)pba & 0x3fff)>>3;
	eTPU->CHAN[channel  ].CR.B.CPBA = i2c_slave_cpba;
	eTPU->CHAN[channel+1].CR.B.CPBA = i2c_slave_cpba;
	eTPU->CHAN[channel+2].CR.B.CPBA = i2c_slave_cpba;
	eTPU->CHAN[channel+3].CR.B.CPBA = i2c_slave_cpba;

	/* initialize the parameter values */
	fs_memset32_ext(pba, 0, _FRAME_SIZE_I2C_slave_); // zero everything

	// put tcr1 freq into counts/us (hz => mhz)
	tcr1_freq /= 1000000;
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__tSU_DAT_, (uint24_t)((tcr1_freq * p_i2c_slave_config->tSU_DAT) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__tBUF_, (uint24_t)((tcr1_freq * p_i2c_slave_config->tBUF) / 1000));

	// set up other chan frame parameters
	fs_etpu_set_chan_local_8_ext (p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__address_, p_i2c_slave_config->address);
	fs_etpu_set_chan_local_8_ext (p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__address_mask_, p_i2c_slave_config->address_mask);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__accept_general_call_, p_i2c_slave_config->accept_general_call);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__read_buffer_, (uint24_t)p_i2c_slave_config->p_read_buffer & 0x3fff);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__read_buffer_size_, p_i2c_slave_config->read_buffer_size);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__write_buffer_, (uint24_t)p_i2c_slave_config->p_write_buffer & 0x3fff);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__write_buffer_size_, p_i2c_slave_config->write_buffer_size);

	/* write FM (function mode) bits (only used on SCL_in) */
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SCL_IN_OFFSET].SCR.R = p_i2c_slave_config->data_mode;
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SCL_OUT_OFFSET].SCR.R = 0;
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SDA_IN_OFFSET].SCR.R = 0;
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SDA_OUT_OFFSET].SCR.R = 0;

	/* write hsr to init the channels */
	eTPU->CHAN[channel  ].HSRR.R = ETPU_I2C_INIT_HSR;
	eTPU->CHAN[channel+1].HSRR.R = ETPU_I2C_INIT_HSR;
	eTPU->CHAN[channel+2].HSRR.R = ETPU_I2C_INIT_HSR;
	eTPU->CHAN[channel+3].HSRR.R = ETPU_I2C_INIT_HSR;

	/* fully write channel configuration register */
	/* channel   = SCL_in */
	/* channel+1 = SCL_out */
	/* channel+2 = SDA_in */
	/* channel+3 = SDA_out */
	/* this has the side-effect of starting the function running */
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SCL_IN_OFFSET].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_slave_I2C_SCL_in_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_slave_I2C_SCL_in_ << 24) +
		(_FUNCTION_NUM_I2C_slave_I2C_SCL_in_ << 16) +
		i2c_slave_cpba;
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SCL_OUT_OFFSET].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_slave_I2C_SCL_out_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_slave_I2C_SCL_out_ << 24) +
		(_FUNCTION_NUM_I2C_slave_I2C_SCL_out_ << 16) +
		i2c_slave_cpba;
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SDA_IN_OFFSET].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_slave_I2C_SDA_in_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_slave_I2C_SDA_in_ << 24) +
		(_FUNCTION_NUM_I2C_slave_I2C_SDA_in_ << 16) +
		i2c_slave_cpba;
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SDA_OUT_OFFSET].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_slave_I2C_SDA_out_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_slave_I2C_SDA_out_ << 24) +
		(_FUNCTION_NUM_I2C_slave_I2C_SDA_out_ << 16) +
		i2c_slave_cpba;

	return 0;
}


int32_t aw_etpu_i2c_slave_set_read_buffer(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* buffer_ptr,
    uint32_t size)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__read_buffer_, (uint24_t)buffer_ptr & 0x3fff);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__read_buffer_size_, size);
	return 0;
}

int32_t aw_etpu_i2c_slave_issue_data_ready(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
    if (p_i2c_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }
	eTPU->CHAN[channel + ETPU_I2C_SLAVE_SCL_OUT_OFFSET].HSRR.R = ETPU_I2C_SLAVE_DATA_READY;
	return 0;
}


int32_t aw_etpu_i2c_slave_set_write_buffer(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* buffer_ptr,
    uint32_t size)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__write_buffer_, (uint24_t)buffer_ptr & 0x3fff);
	fs_etpu_set_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__write_buffer_size_, size);
	return 0;
}


int32_t aw_etpu_i2c_slave_get_transfer_status(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* header_ptr,
    uint32_t* size_ptr,
    uint8_t* error_flags_ptr)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	if (header_ptr)
		*header_ptr = (uint8_t)fs_etpu_get_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__header_);
	if (size_ptr)
		*size_ptr = fs_etpu_get_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__byte_cnt_);
	if (error_flags_ptr)
		*error_flags_ptr = fs_etpu_get_chan_local_8_ext(p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__error_flags_);
	return 0;
}


int32_t aw_etpu_i2c_slave_get_write_data(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* header_ptr,
    uint8_t* dest_buffer_ptr,
    uint32_t* size_ptr)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
	uint32_t i;
	uint8_t* src_buffer_ptr;
	uint32_t size;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!size_ptr || !header_ptr || !dest_buffer_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif
	*header_ptr = (uint8_t)fs_etpu_get_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__header_);
	*size_ptr = fs_etpu_get_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__byte_cnt_);
	// make sure not to exceed max buffer size
	// it is up to the host to check for a buffer overflow fault
	size = fs_etpu_get_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__write_buffer_size_);
	if (*size_ptr < size)
		size = *size_ptr;
	src_buffer_ptr = (uint8_t*)(fs_etpu_get_chan_local_24_ext(p_i2c_slave_instance->em, channel, _CPBA24_I2C_slave__write_buffer_) +
        (p_i2c_slave_instance->em == EM_AB ? fs_etpu_data_ram_start : fs_etpu_c_data_ram_start));
	for (i = 0; i < size; i++)
		*dest_buffer_ptr++ = *src_buffer_ptr++;
	return 0;
}


int32_t aw_etpu_i2c_slave_latch_clear_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
    if (p_i2c_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }
	eTPU->CHAN[channel+ETPU_I2C_SLAVE_SCL_IN_OFFSET].HSRR.R = ETPU_I2C_LATCH_CLEAR_ERRORS_HSR;
	return 0;
}

int32_t aw_etpu_i2c_slave_get_running_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* error_flags_ptr)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!error_flags_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif
	*error_flags_ptr = fs_etpu_get_chan_local_8_ext(p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__error_flags_);
	return 0;
}

int32_t aw_etpu_i2c_slave_clear_running_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	fs_etpu_set_chan_local_8_ext(p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__error_flags_, 0);
	return 0;
}

int32_t aw_etpu_i2c_slave_get_latched_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance,
    uint8_t* error_flags_ptr)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!error_flags_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif
	*error_flags_ptr = fs_etpu_get_chan_local_8_ext(p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__latched_error_flags_);
	return 0;
}

int32_t aw_etpu_i2c_slave_clear_latched_error_flags(
    struct aw_i2c_slave_instance_t *p_i2c_slave_instance)
{
	uint8_t channel = p_i2c_slave_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	fs_etpu_set_chan_local_8_ext(p_i2c_slave_instance->em, channel, _CPBA8_I2C_slave__latched_error_flags_, 0);
	return 0;
}
