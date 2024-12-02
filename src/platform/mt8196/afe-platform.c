// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2023 MediaTek. All rights reserved.
 *
 * Author: Darren Ye <darren.ye@mediatek.com>
 */

#include <sof/common.h>
#include <errno.h>
#include <sof/drivers/afe-drv.h>
#include <mt8196-afe-reg.h>
#include <mt8196-afe-common.h>

/*
 * AFE: Audio Front-End
 *
 * frontend (memif):
 *   memory interface
 *   UL (uplink for capture)
 *   DL (downlink for playback)
 * backend:
 *   TDM In
 *   TDM Out
 *   DMIC
 *   GASRC
 *   I2S Out
 *   I2S In
 *   etc.
 * interconn:
 *   inter-connection,
 *   connect frontends and backends as DSP path
 */


static const struct mtk_base_memif_data memif_data[MT8196_MEMIF_NUM] = {
	[MT8196_MEMIF_DL1] = {
		.name = "DL1",
		.id = MT8196_MEMIF_DL1,
		.reg_ofs_base = AFE_DL1_BASE,
		.reg_ofs_cur = AFE_DL1_CUR,
		.reg_ofs_end = AFE_DL1_END,
		.reg_ofs_base_msb = AFE_DL1_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL1_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL1_END_MSB,
		.fs_reg = AFE_DL1_CON0,
		.fs_shift = DL1_SEL_FS_SFT,
		.fs_maskbit = DL1_SEL_FS_MASK,
		.mono_reg = AFE_DL1_CON0,
		.mono_shift = DL1_MONO_SFT,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_DL1_CON0,
		.enable_shift = DL1_ON_SFT,
		.hd_reg = AFE_DL1_CON0,
		//.hd_mask = DL1_HD_MODE_MASK,
		.hd_shift = DL1_HD_MODE_SFT,
		.hd_align_reg = AFE_DL1_CON0,
		.hd_align_mshift = DL1_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.ch_num_reg = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL1_CON0,
		.pbuf_mask = DL1_PBUF_SIZE_MASK,
		.pbuf_shift = DL1_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL1_CON0,
		.minlen_mask = DL1_MINLEN_MASK,
		.minlen_shift = DL1_MINLEN_SFT,
		//.maxlen_reg = AFE_DL1_CON0,
		//.maxlen_mask = DL1_MAXLEN_MASK,
		//.maxlen_shift = DL1_MAXLEN_SFT,
	},
	[MT8196_MEMIF_DL_24CH] = {
		.name = "DL_24CH",
		.id = MT8196_MEMIF_DL_24CH,
		.reg_ofs_base = AFE_DL_24CH_BASE,
		.reg_ofs_cur = AFE_DL_24CH_CUR,
		.reg_ofs_end = AFE_DL_24CH_END,
		.reg_ofs_base_msb = AFE_DL_24CH_BASE_MSB,
		.reg_ofs_cur_msb = AFE_DL_24CH_CUR_MSB,
		.reg_ofs_end_msb = AFE_DL_24CH_END_MSB,
		.fs_reg = AFE_DL_24CH_CON0,
		.fs_shift = DL_24CH_SEL_FS_SFT,
		.fs_maskbit = DL_24CH_SEL_FS_MASK,
		.mono_reg = -1,
		.mono_shift = -1,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_DL_24CH_CON0,
		.enable_shift = DL_24CH_ON_SFT,
		.hd_reg = AFE_DL_24CH_CON0,
		//.hd_mask = DL_24CH_HD_MODE_MASK,
		.hd_shift = DL_24CH_HD_MODE_SFT,
		.hd_align_reg = AFE_DL_24CH_CON0,
		.hd_align_mshift = DL_24CH_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.pbuf_reg = AFE_DL_24CH_CON0,
		.pbuf_mask = DL_24CH_PBUF_SIZE_MASK,
		.pbuf_shift = DL_24CH_PBUF_SIZE_SFT,
		.minlen_reg = AFE_DL_24CH_CON0,
		.minlen_mask = DL_24CH_MINLEN_MASK,
		.minlen_shift = DL_24CH_MINLEN_SFT,
		//.maxlen_reg = AFE_DL_24CH_CON0,
		//.maxlen_mask = DL_24CH_MAXLEN_MASK,
		//.maxlen_shift = DL_24CH_MAXLEN_SFT,
		.ch_num_reg = AFE_DL_24CH_CON0,
		.ch_num_maskbit = DL_24CH_NUM_MASK,
		.ch_num_shift = DL_24CH_NUM_SFT,
	},
	[MT8196_MEMIF_UL0] = {
		.name = "UL0",
		.id = MT8196_MEMIF_UL0,
		.reg_ofs_base = AFE_VUL0_BASE,
		.reg_ofs_cur = AFE_VUL0_CUR,
		.reg_ofs_end = AFE_VUL0_END,
		.reg_ofs_base_msb = AFE_VUL0_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL0_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL0_END_MSB,
		.fs_reg = AFE_VUL0_CON0,
		.fs_shift = VUL0_SEL_FS_SFT,
		.fs_maskbit = VUL0_SEL_FS_MASK,
		.mono_reg = AFE_VUL0_CON0,
		.mono_shift = VUL0_MONO_SFT,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_VUL0_CON0,
		.enable_shift = VUL0_ON_SFT,
		.hd_reg = AFE_VUL0_CON0,
		//.hd_mask = VUL0_HD_MODE_MASK,
		.hd_shift = VUL0_HD_MODE_SFT,
		.hd_align_reg = AFE_VUL0_CON0,
		.hd_align_mshift = VUL0_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
	[MT8196_MEMIF_UL1] = {
		.name = "UL1",
		.id = MT8196_MEMIF_UL1,
		.reg_ofs_base = AFE_VUL1_BASE,
		.reg_ofs_cur = AFE_VUL1_CUR,
		.reg_ofs_end = AFE_VUL1_END,
		.reg_ofs_base_msb = AFE_VUL1_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL1_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL1_END_MSB,
		.fs_reg = AFE_VUL1_CON0,
		.fs_shift = VUL1_SEL_FS_SFT,
		.fs_maskbit = VUL1_SEL_FS_MASK,
		.mono_reg = AFE_VUL1_CON0,
		.mono_shift = VUL1_MONO_SFT,
		.enable_reg = AFE_VUL1_CON0,
		.enable_shift = VUL1_ON_SFT,
		.hd_reg = AFE_VUL1_CON0,
		//.hd_mask = VUL1_HD_MODE_MASK,
		.hd_shift = VUL1_HD_MODE_SFT,
		.hd_align_reg = AFE_VUL1_CON0,
		.hd_align_mshift = VUL1_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},	
	[MT8196_MEMIF_UL2] = {
		.name = "UL2",
		.id = MT8196_MEMIF_UL2,
		.reg_ofs_base = AFE_VUL2_BASE,
		.reg_ofs_cur = AFE_VUL2_CUR,
		.reg_ofs_end = AFE_VUL2_END,
		.reg_ofs_base_msb = AFE_VUL2_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL2_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL2_END_MSB,
		.fs_reg = AFE_VUL2_CON0,
		.fs_shift = VUL2_SEL_FS_SFT,
		.fs_maskbit = VUL2_SEL_FS_MASK,
		.mono_reg = AFE_VUL2_CON0,
		.mono_shift = VUL2_MONO_SFT,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_VUL2_CON0,
		.enable_shift = VUL2_ON_SFT,
		.hd_reg = AFE_VUL2_CON0,
		//.hd_mask = VUL2_HD_MODE_MASK,
		.hd_shift = VUL2_HD_MODE_SFT,
		.hd_align_reg = AFE_VUL2_CON0,
		.hd_align_mshift = VUL2_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
#if 0
	[MT8196_MEMIF_UL26] = {
		.name = "UL26",
		.id = MT8196_MEMIF_UL26,
		.reg_ofs_base = AFE_VUL26_BASE,
		.reg_ofs_cur = AFE_VUL26_CUR,
		.reg_ofs_end = AFE_VUL26_END,
		.reg_ofs_base_msb = AFE_VUL26_BASE_MSB,
		.reg_ofs_cur_msb = AFE_VUL26_CUR_MSB,
		.reg_ofs_end_msb = AFE_VUL26_END_MSB,
		.fs_reg = AFE_VUL26_CON0,
		.fs_shift = VUL26_SEL_FS_SFT,
		.fs_maskbit = VUL26_SEL_FS_MASK,
		.mono_reg = AFE_VUL26_CON0,
		.mono_shift = VUL26_MONO_SFT,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_VUL26_CON0,
		.enable_shift = VUL26_ON_SFT,
		.hd_reg = AFE_VUL26_CON0,
		//.hd_mask = VUL26_HD_MODE_MASK,
		.hd_shift = VUL26_HD_MODE_SFT,
		.hd_align_reg = AFE_VUL26_CON0,
		.hd_align_mshift = VUL26_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
		.out_on_use_reg = AFE_VUL26_CON0,
		.out_on_use_mask = OUT_ON_USE_VUL26_MASK,
		.out_on_use_shift = OUT_ON_USE_VUL26_SFT,
	},
#endif
};

#if 0
static const struct mtk_base_memif_data memif_data[MT8188_MEMIF_NUM] = {
	[MT8188_MEMIF_DL2] = {
		.name = "DL2",
		.id = MT8188_MEMIF_DL2,
		.reg_ofs_base = AFE_DL2_BASE,
		.reg_ofs_cur = AFE_DL2_CUR,
		.reg_ofs_end = AFE_DL2_END,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON0,
		.fs_shift = 10,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = 0,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 18,
		.hd_reg = AFE_DL2_CON0,
		.hd_shift = 5,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 18,
		.ch_num_reg = AFE_DL2_CON0,
		.ch_num_shift = 0,
		.ch_num_maskbit = 0x1f,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 18,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 18,
	},
	[MT8188_MEMIF_DL3] = {
		.name = "DL3",
		.id = MT8188_MEMIF_DL3,
		.reg_ofs_base = AFE_DL3_BASE,
		.reg_ofs_cur = AFE_DL3_CUR,
		.reg_ofs_end = AFE_DL3_END,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON0,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = -1,
		.mono_shift = 0,
		.int_odd_flag_reg = -1,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 19,
		.hd_reg = AFE_DL3_CON0,
		.hd_shift = 5,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 19,
		.ch_num_reg = AFE_DL3_CON0,
		.ch_num_shift = 0,
		.ch_num_maskbit = 0x1f,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 19,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 19,
	},
	[MT8188_MEMIF_UL4] = {
		.name = "UL4",
		.id = MT8188_MEMIF_UL4,
		.reg_ofs_base = AFE_UL4_BASE,
		.reg_ofs_cur = AFE_UL4_CUR,
		.reg_ofs_end = AFE_UL4_END,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 15,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL4_CON0,
		.mono_shift = 1,
		.int_odd_flag_reg = AFE_UL4_CON0,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 4,
		.hd_reg = AFE_UL4_CON0,
		.hd_shift = 5,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 3,
		.ch_num_reg = -1,
		.ch_num_shift = 0,
		.ch_num_maskbit = 0,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 3,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 3,
	},
	[MT8188_MEMIF_UL5] = {
		.name = "UL5",
		.id = MT8188_MEMIF_UL5,
		.reg_ofs_base = AFE_UL5_BASE,
		.reg_ofs_cur = AFE_UL5_CUR,
		.reg_ofs_end = AFE_UL5_END,
		.fs_reg = AFE_MEMIF_AGENT_FS_CON2,
		.fs_shift = 20,
		.fs_maskbit = 0x1f,
		.mono_reg = AFE_UL5_CON0,
		.mono_shift = 1,
		.int_odd_flag_reg = AFE_UL5_CON0,
		.int_odd_flag_shift = 0,
		.enable_reg = AFE_DAC_CON0,
		.enable_shift = 5,
		.hd_reg = AFE_UL5_CON0,
		.hd_shift = 5,
		.agent_disable_reg = AUDIO_TOP_CON5,
		.agent_disable_shift = 4,
		.ch_num_reg = -1,
		.ch_num_shift = 0,
		.ch_num_maskbit = 0,
		.msb_reg = AFE_NORMAL_BASE_ADR_MSB,
		.msb_shift = 4,
		.msb2_reg = AFE_NORMAL_END_ADR_MSB,
		.msb2_shift = 4,
	},
};
#endif

struct mt8196_afe_rate {
	unsigned int rate;
	unsigned int reg_value;
};

static const struct mt8196_afe_rate mt8196_afe_rates[] = {
	{
		.rate = 8000,
		.reg_value = 0,
	},
	{
		.rate = 12000,
		.reg_value = 2,
	},
	{
		.rate = 16000,
		.reg_value = 4,
	},
	{
		.rate = 24000,
		.reg_value = 6,
	},
	{
		.rate = 32000,
		.reg_value = 8,
	},
	{
		.rate = 48000,
		.reg_value = 0x0a,
	},
	{
		.rate = 96000,
		.reg_value = 14,
	},
	{
		.rate = 192000,
		.reg_value = 18,
	},
	{
		.rate = 384000,
		.reg_value = 22,
	},
#if 0
	{
		.rate = 7350,
		.reg_value = 16,
	},
#endif
	{
		.rate = 11025,
		.reg_value = 1,
	},
#if 0
	{
		.rate = 14700,
		.reg_value = 18,
	},
#endif
	{
		.rate = 22050,
		.reg_value = 5,
	},
#if 0
	{
		.rate = 29400,
		.reg_value = 20,
	},
#endif
	{
		.rate = 44100,
		.reg_value = 9,
	},
	{
		.rate = 88200,
		.reg_value = 13,
	},
	{
		.rate = 176400,
		.reg_value = 17,
	},
	{
		.rate = 352800,
		.reg_value = 21,
	},
};

static unsigned int mt8196_afe_fs_timing(unsigned int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt8196_afe_rates); i++)
		if (mt8196_afe_rates[i].rate == rate)
			return mt8196_afe_rates[i].reg_value;

	return -EINVAL;
}

static unsigned int mt8196_afe_fs(unsigned int rate, int aud_blk)
{
	return mt8196_afe_fs_timing(rate);
}

struct mtk_base_afe_platform mtk_afe_platform = {
	.base_addr = AFE_BASE_ADDR,
	.memif_datas = memif_data,
	.memif_size = MT8196_MEMIF_NUM,
	.memif_dl_num = MT8196_MEMIF_DL_NUM,
	.memif_32bit_supported = 0,
	.irq_datas = NULL,
	.irqs_size = 0,
	.dais_size = MT8196_DAI_NUM,
	.afe_fs = mt8196_afe_fs,
	.irq_fs = mt8196_afe_fs_timing,
};
