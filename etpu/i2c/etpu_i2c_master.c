/**************************************************************************
* FILE NAME: etpu_i2c_master.c
* 
* DESCRIPTION: Implementation of API for initializing and controlling the 
* I2C Master eTPU function.  See the .h file for API documentation.
*
*========================================================================
* REV      AUTHOR      DATE        DESCRIPTION OF CHANGE                 
* ---   -----------  ----------    ---------------------                 
* 1.0     J Diener   01/Dec/11     Initial version.     
* 2.0     J Diener   04/Sep/20     Modernize API.
*
**************************************************************************/

#include "etpu_util_ext.h"
#include "etpu_i2c_master.h"
#include "etpu_i2c_common.h"
#include "etpu_set_defines.h"


int32_t aw_etpu_i2c_master_init(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    struct aw_i2c_master_config_t   *p_i2c_master_config)
{
    volatile struct eTPU_struct * eTPU;
	uint32_t *pba;	/* parameter base address for channel */
	uint32_t tcr1_freq;
	uint32_t bit_time_tcr1_cnt;
	uint32_t i2c_master_cpba;
	uint8_t channel = p_i2c_master_instance->base_chan_num;
	uint8_t priority = p_i2c_master_instance->priority;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!priority || (priority > 3))
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
        if (p_i2c_master_instance->base_chan_num < 32)
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
	fs_etpu_disable_ext(p_i2c_master_instance->em, channel );
	fs_etpu_disable_ext(p_i2c_master_instance->em, channel + 1 );
	fs_etpu_disable_ext(p_i2c_master_instance->em, channel + 2 );
	fs_etpu_disable_ext(p_i2c_master_instance->em, channel + 3 );

	/* allocate a channel frame if not already done so */
	/* NOTE: this means that re-initialization of this channel (2 channels) */
	/* must re-use the same channel frame allocation */
	if (eTPU->CHAN[channel].CR.B.CPBA == 0 )
	{
		/* get parameter RAM
		number of parameters passed from eTPU C code */
		pba = fs_etpu_malloc_ext(p_i2c_master_instance->em, _FRAME_SIZE_I2C_master_);
		if (pba == 0)
			return (FS_ETPU_ERROR_MALLOC);
	}
	else /*set pba to what is in the CR register*/
	{
		pba = fs_etpu_get_cpba_ext(p_i2c_master_instance->em, channel);
	}
	// the channel group shares the same channel frame
	i2c_master_cpba = ((uint32_t)pba & 0x3fff)>>3;
	eTPU->CHAN[channel  ].CR.B.CPBA = i2c_master_cpba;
	eTPU->CHAN[channel+1].CR.B.CPBA = i2c_master_cpba;
	eTPU->CHAN[channel+2].CR.B.CPBA = i2c_master_cpba;
	eTPU->CHAN[channel+3].CR.B.CPBA = i2c_master_cpba;

    p_i2c_master_instance->p_cpba = (void*)pba;
    p_i2c_master_instance->p_cpba_pse = (void*)((uint32_t)pba + (fs_etpu_data_ram_ext - fs_etpu_data_ram_start));

	/* initialize the parameter values */
	fs_memset32_ext(pba, 0, _FRAME_SIZE_I2C_master_); // zero everything

	// calc the bit time first, then calc the individual timing constraints
	bit_time_tcr1_cnt = tcr1_freq / (p_i2c_master_config->bit_rate_khz * 1000);

	// configure clock signal to be symmetric
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tLOW_, (uint24_t)(bit_time_tcr1_cnt / 2));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tHIGH_, (uint24_t)(bit_time_tcr1_cnt / 2));

	// use the half bit time for START/STOP timing as well
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tBUF_, (uint24_t)(bit_time_tcr1_cnt / 2));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tSU_STA_, (uint24_t)(bit_time_tcr1_cnt / 2));
	// NOTE: tHD_STA currently not used; tHIGH is used in its place.  Thus tHIGH must be >= to tHD_STA.
	//fs_etpu_set_chan_local_24(channel, _CPBA24_I2C__tHD_STA_, (uint24_t)(bit_time_tcr1_cnt / 2));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tSU_STO_, (uint24_t)(bit_time_tcr1_cnt / 2));

	// data hold timing - make it a tenth of the low time
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tHD_DAT_, (uint24_t)(bit_time_tcr1_cnt / 20));

	// maximum signal rise time (used primarily for clock stretch detection)
	// make it a tenth of the bit time
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tr_max_, (uint24_t)(bit_time_tcr1_cnt / 10));

	// set the cmd buffer ptr
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__p_cmd_list_, ((uint32_t)p_i2c_master_config->p_cmd_buffer & 0x3fff) );

	/* write FM (function mode) bits (not used currently) */
	eTPU->CHAN[channel  ].SCR.R = 0;
	eTPU->CHAN[channel+1].SCR.R = 0;
	eTPU->CHAN[channel+2].SCR.R = 0;
	eTPU->CHAN[channel+3].SCR.R = 0;

	/* write hsr to init the channels */
	eTPU->CHAN[channel  ].HSRR.R = ETPU_I2C_INIT_HSR;
	eTPU->CHAN[channel+1].HSRR.R = ETPU_I2C_INIT_HSR;
	eTPU->CHAN[channel+2].HSRR.R = ETPU_I2C_INIT_HSR;
	eTPU->CHAN[channel+3].HSRR.R = ETPU_I2C_INIT_HSR;

	/* fully write channel configuration register */
	/* channel   = SCL_out */
	/* channel+1 = SCL_in */
	/* channel+2 = SDA_out */
	/* channel+3 = SDA_in */
	/* this has the side-effect of starting the function running */
	eTPU->CHAN[channel  ].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_master_I2C_SCL_out_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_master_I2C_SCL_out_ << 24) +
		(_FUNCTION_NUM_I2C_master_I2C_SCL_out_ << 16) +
		i2c_master_cpba;
	eTPU->CHAN[channel+1].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_master_I2C_SCL_in_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_master_I2C_SCL_in_ << 24) +
		(_FUNCTION_NUM_I2C_master_I2C_SCL_in_ << 16) +
		i2c_master_cpba;
	eTPU->CHAN[channel+2].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_master_I2C_SDA_out_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_master_I2C_SDA_out_ << 24) +
		(_FUNCTION_NUM_I2C_master_I2C_SDA_out_ << 16) +
		i2c_master_cpba;
	eTPU->CHAN[channel+3].CR.R = (priority << 28) + 
		(_ENTRY_TABLE_PIN_DIR_I2C_master_I2C_SDA_in_ << 25) +
		(_ENTRY_TABLE_TYPE_I2C_master_I2C_SDA_in_ << 24) +
		(_FUNCTION_NUM_I2C_master_I2C_SDA_in_ << 16) +
		i2c_master_cpba;

	return 0;
}

int32_t aw_etpu_i2c_master_set_timing(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    struct aw_i2c_master_config_t   *p_i2c_master_config)
{
	uint32_t tcr1_freq;
	uint8_t channel = p_i2c_master_instance->base_chan_num;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_master_instance->em == EM_AB)
    {
        if (p_i2c_master_instance->base_chan_num < 32)
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
        tcr1_freq = etpu_c_tcr1_freq;
    }
	// put it into counts/us (hz => mhz)
	tcr1_freq /= 1000000;

	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tLOW_, (uint24_t)((tcr1_freq * p_i2c_master_config->tLOW) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tHIGH_, (uint24_t)((tcr1_freq * p_i2c_master_config->tHIGH) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tBUF_, (uint24_t)((tcr1_freq * p_i2c_master_config->tBUF) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tSU_STA_, (uint24_t)((tcr1_freq * p_i2c_master_config->tSU_STA) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tSU_STO_, (uint24_t)((tcr1_freq * p_i2c_master_config->tSU_STO) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tHD_DAT_, (uint24_t)((tcr1_freq * p_i2c_master_config->tHD_DAT) / 1000));
	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__tr_max_, (uint24_t)((tcr1_freq * p_i2c_master_config->tr_max) / 1000));

	return 0;
}

int32_t aw_etpu_i2c_master_transmit(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    uint8_t slave_address,
    uint32_t buffer_size,
    uint8_t* buffer_ptr)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t in_use_flag;
	struct aw_etpu_i2c_transfer_cmd* p_cmd;
	uint8_t channel = p_i2c_master_instance->base_chan_num;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (buffer_size && !buffer_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	// OK, set up transmit

	// check ready flag first
	in_use_flag = fs_etpu_get_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__in_use_flag_);
	if (in_use_flag)
		return FS_ETPU_ERROR_NOT_READY;

	p_cmd = (struct aw_etpu_i2c_transfer_cmd*)(fs_etpu_get_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__p_cmd_list_) + 
		fs_etpu_data_ram_start);
	p_cmd->_header = (slave_address & ~ETPU_I2C_RW_MASK) | ETPU_I2C_WRITE_MESSAGE;
	p_cmd->_p_buffer = ((uint32_t)buffer_ptr & 0x3fff);
	p_cmd->_size = buffer_size;

	// set one cmd and go
	fs_etpu_set_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__cmd_cnt_, 1 );
	eTPU->CHAN[channel].HSRR.R = ETPU_I2C_MASTER_START_TRANSFER_HSR;

	return 0;
}


int32_t aw_etpu_i2c_master_receive(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    uint8_t slave_address,
    uint32_t buffer_size,
    uint8_t* buffer_ptr)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t in_use_flag;
	struct aw_etpu_i2c_transfer_cmd* p_cmd;
	uint8_t channel = p_i2c_master_instance->base_chan_num;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (buffer_size && !buffer_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	// OK, set up receive

	// check ready flag first
	in_use_flag = fs_etpu_get_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__in_use_flag_);
	if (in_use_flag)
		return FS_ETPU_ERROR_NOT_READY;

	p_cmd = (struct aw_etpu_i2c_transfer_cmd*)(fs_etpu_get_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__p_cmd_list_) + 
		fs_etpu_data_ram_start);
	p_cmd->_header = (slave_address & ~ETPU_I2C_RW_MASK) | ETPU_I2C_READ_MESSAGE;
	p_cmd->_p_buffer = ((uint32_t)buffer_ptr & 0x3fff);
	p_cmd->_size = buffer_size;

	// set one cmd and go
	fs_etpu_set_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__cmd_cnt_, 1 );
	eTPU->CHAN[channel].HSRR.R = ETPU_I2C_MASTER_START_TRANSFER_HSR;

	return 0;
}


int32_t aw_etpu_i2c_master_combined_transfer(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    uint8_t header1,
    uint32_t buf1_size,
    uint8_t* buf1_ptr,
    uint8_t header2,
    uint32_t buf2_size,
    uint8_t* buf2_ptr)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t in_use_flag;
	struct aw_etpu_i2c_transfer_cmd* p_cmd;
	uint8_t channel = p_i2c_master_instance->base_chan_num;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if ((buf1_size && !buf1_ptr) || (buf2_size && !buf2_ptr))
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	// OK, set up combined format transfer

	// check ready flag first
	in_use_flag = fs_etpu_get_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__in_use_flag_);
	if (in_use_flag)
		return FS_ETPU_ERROR_NOT_READY;

	p_cmd = (struct aw_etpu_i2c_transfer_cmd*)(fs_etpu_get_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__p_cmd_list_) + 
		fs_etpu_data_ram_start);

	p_cmd->_header = header1;
	p_cmd->_p_buffer = ((uint32_t)buf1_ptr & 0x3fff);
	p_cmd->_size = buf1_size;
	p_cmd++;
	p_cmd->_header = header2;
	p_cmd->_p_buffer = ((uint32_t)buf2_ptr & 0x3fff);
	p_cmd->_size = buf2_size;

	// set two cmds and go
	fs_etpu_set_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__cmd_cnt_, 2 );
	eTPU->CHAN[channel].HSRR.R = ETPU_I2C_MASTER_START_TRANSFER_HSR;

	return 0;
}

int32_t aw_etpu_i2c_master_raw_transfer(
    struct aw_i2c_master_instance_t *p_i2c_master_instance,
    struct aw_etpu_i2c_transfer_cmd* cmd_buffer_ptr,
    uint32_t cmd_cnt)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t in_use_flag;
	uint8_t channel = p_i2c_master_instance->base_chan_num;

#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!cmd_buffer_ptr || !cmd_cnt)
		return FS_ETPU_ERROR_VALUE;
#endif

    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

	// check ready flag first
	in_use_flag = fs_etpu_get_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__in_use_flag_);
	if (in_use_flag)
		return FS_ETPU_ERROR_NOT_READY;

	// OK, then kick off transfer

	fs_etpu_set_chan_local_24_ext(p_i2c_master_instance->em, channel, _CPBA24_I2C_master__p_cmd_list_, ((uint32_t)cmd_buffer_ptr & 0x3fff) );
	fs_etpu_set_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__cmd_cnt_, cmd_cnt );
	eTPU->CHAN[channel].HSRR.R = ETPU_I2C_MASTER_START_TRANSFER_HSR;

	return 0;
}


int32_t aw_etpu_i2c_master_latch_clear_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance)
{
    volatile struct eTPU_struct * eTPU;
	uint8_t channel = p_i2c_master_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
    if (p_i2c_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }
	eTPU->CHAN[channel+ETPU_I2C_MASTER_SCL_IN_OFFSET].HSRR.R = ETPU_I2C_LATCH_CLEAR_ERRORS_HSR;
	return 0;
}
int32_t aw_etpu_i2c_master_get_running_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance,
												   uint8_t* error_flags_ptr)
{
	uint8_t channel = p_i2c_master_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!error_flags_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif
	*error_flags_ptr = fs_etpu_get_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__error_flags_);
	return 0;
}
int32_t aw_etpu_i2c_master_clear_running_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance)
{
	uint8_t channel = p_i2c_master_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	fs_etpu_set_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__error_flags_, 0);
	return 0;
}
int32_t aw_etpu_i2c_master_get_latched_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance,
												   uint8_t* error_flags_ptr)
{
	uint8_t channel = p_i2c_master_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
	if (!error_flags_ptr)
		return FS_ETPU_ERROR_VALUE;
#endif
	*error_flags_ptr = fs_etpu_get_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__latched_error_flags_);
	return 0;
}
int32_t aw_etpu_i2c_master_clear_latched_error_flags(struct aw_i2c_master_instance_t *p_i2c_master_instance)
{
	uint8_t channel = p_i2c_master_instance->base_chan_num;
#ifdef ETPU_I2C_PARAMETER_CHECK
	if (((channel > (32 - ETPU_I2C_CHANNELS_USED)) && (channel < 64)) || (channel > 96 - (ETPU_I2C_CHANNELS_USED)))
		return FS_ETPU_ERROR_VALUE;
#endif
	fs_etpu_set_chan_local_8_ext(p_i2c_master_instance->em, channel, _CPBA8_I2C_master__latched_error_flags_, 0);
	return 0;
}
