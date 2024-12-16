#include <sof/lib/dai-legacy.h>
#include <ipc/dai.h>
#include <sof/drivers/afe-drv.h>

extern const struct dai_driver afe_dai_driver;

////////////////////////////////////////////////////////////////////////
// Cribbed from mt8196, need to source from DTS
//
// MONSTER 7MB HEADER, MUST FIX
#include "../../mt8196/include/platform/mt8196-afe-reg.h"

#define DAI_HANDSHAKE(dai, irq, chan) ((chan << 16) | (irq << 8) | dai)

#define DAI_DEFINE(dai, irq, chan)		\
	{ .index = dai, .drv = &afe_dai_driver,	\
	  .plat_data = { .fifo = {		\
		{ .handshake = DAI_HANDSHAKE(dai, irq, chan), } } } }

// Note that there are two sets of enumerants, and the order is
// mismatched! (DL1 and DL_24CH are swapped).  The DAI numbering is
// the master and what the host uses, from there you can unpack the
// "channel" from the handshake value which is the memif index.  I
// think the variant ordering of memif_data[] is just a mistake...  I
// don't think the handshake value exists on the host, I only see it
// being parsed here in SOF.

enum {
        MT8196_DAI_I2S_OUT4, /* speaker */
        MT8196_DAI_I2S_OUT6, /* headset out */
        MT8196_DAI_AP_DMIC, /* DMIC */
        MT8196_DAI_I2S_IN6, /* headset mic */
        MT8196_DAI_AP_DMIC_CH34,
};

// Indexes that identify an afe driver
enum {
        MT8196_MEMIF_DL1,
        MT8196_MEMIF_DL_24CH,
        MT8196_MEMIF_UL0,
        MT8196_MEMIF_UL1,
        MT8196_MEMIF_UL2,
};

// Note: the IRQ's used are always equal to their enumerants, not sure
// if that's guaranteed.  Maybe academic as no interrupts currently
static struct dai mtk_dais_0[] = {
        DAI_DEFINE(MT8196_DAI_I2S_OUT4,      0, MT8196_MEMIF_DL_24CH),
        DAI_DEFINE(MT8196_DAI_I2S_OUT6,      0, MT8196_MEMIF_DL1),
        DAI_DEFINE(MT8196_DAI_AP_DMIC,       0, MT8196_MEMIF_UL0),
        DAI_DEFINE(MT8196_DAI_I2S_IN6,       0, MT8196_MEMIF_UL1),
        DAI_DEFINE(MT8196_DAI_AP_DMIC_CH34,  0, MT8196_MEMIF_UL2),
};

static const struct mtk_base_memif_data mtk_memif_data[] = {
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
		.hd_shift = VUL2_HD_MODE_SFT,
		.hd_align_reg = AFE_VUL2_CON0,
		.hd_align_mshift = VUL2_HALIGN_SFT,
		.agent_disable_reg = -1,
		.agent_disable_shift = -1,
		.msb_reg = -1,
		.msb_shift = -1,
	},
};
#define MTK_DL_NUM 2

#define MTK_AFE_BASE 0x1a110000

////////////////////////////////////////////////////////////////////////
// Alternate Devicetree-based DAI/AFE config mechanism.  Maintain in
// parallel for a bit.

/* Bitfield register: address, number of bits, and left shift amount */
struct afe_bitfld {
	uint32_t reg;
	uint8_t shift;
	uint8_t bits;
};

/* Pair of registers to store a 64 bit host/bus address */
struct afe_busreg {
	uint32_t hi;
	uint32_t lo;
};

struct afe_cfg {
	char afe_name[32];
	int dai_id;
	bool downlink;
	bool mono_invert;
	struct afe_busreg base;
	struct afe_busreg end;
	struct afe_busreg cur;
	struct afe_bitfld fs;
	struct afe_bitfld hd;
	struct afe_bitfld enable;
	struct afe_bitfld mono;
	struct afe_bitfld quad_ch;
	struct afe_bitfld int_odd;
	struct afe_bitfld msb;
	struct afe_bitfld msb2;
	struct afe_bitfld agent_disable;
	struct afe_bitfld ch_num;
};

#define MTK_AFE_BASE 0x1a110000

/* Converts the DTS_derived afe_cfg struct to a runtime memif_data for
 * use by the legacy driver.  This is temporary, pending a
 * Zephyrization port that will get the driver using the config struct
 * directly.
 *
 * Note the preprocessor trickery to help mapping the regularized DTS
 * data to the "almost but not quite convention-conforming" original
 * naming.  Mostly just some naming quirks.  The only semantic
 * differences are that the register addresses in DTS become offsets
 * from MTK_AFE_BASE, that default/unset register addresses are stored
 * as -1 and not NULL.
 */
static void cfg_convert(const struct afe_cfg *src, struct mtk_base_memif_data *dst)
{
#define REGCVT(R) (((R) > 0) ? ((R) - MTK_AFE_BASE) : -1)

#define COPYBIT(S, Dr, Ds) do {		\
	dst->Dr = REGCVT(src->S.reg);	\
	dst->Ds = src->S.shift;		\
	} while (0)

#define COPYFLD(S, Dr, Ds, Dm) do {	\
	COPYBIT(S, Dr, Ds);		\
	dst->Dm = BIT(src->S.bits) - 1;	\
	} while (0)

#define COPY2(F) COPYBIT(F, F##_reg, F##_shift)
#define COPY3(F) COPYFLD(F, F##_reg, F##_shift, F##_mask)

	dst->name = src->afe_name; /* Assumes it's a string literal! */
	dst->reg_ofs_base = REGCVT(src->base.lo);
	dst->reg_ofs_cur = REGCVT(src->cur.lo);
	dst->reg_ofs_end = REGCVT(src->end.lo);
	dst->reg_ofs_base_msb = REGCVT(src->base.hi);
	dst->reg_ofs_cur_msb = REGCVT(src->cur.hi);
	dst->reg_ofs_end_msb = REGCVT(src->end.hi);
	dst->mono_invert = src->mono_invert;

	COPYFLD(fs, fs_reg, fs_shift, fs_maskbit);
	COPY2(mono);
	COPY3(quad_ch);
	COPYBIT(int_odd, int_odd_flag_reg, int_odd_flag_shift);
	COPY2(enable);
	COPY2(hd);
	COPY2(msb);
	COPY2(msb2);
	COPY2(agent_disable);
	COPYFLD(ch_num, ch_num_reg, ch_num_shift, ch_num_maskbit);

#undef REGCVT
#undef COPYBIT
#undef COPYFLD
#undef COPY2
#undef COPY3
}

/* Validation utility, compares a converted memif_data struct with an
 * original/upstream one. Note that some mismatches are expected and
 * benign, as unused/default values in the originals aren't always
 * represented the same way.
 */
static void cfg_cmp(const struct mtk_base_memif_data *a, struct mtk_base_memif_data *b)
{
#define CHK(F) if(a->F != b->F) printk(" !!%s: want 0x%x got 0x%x\n", #F, a->F, b->F)
	CHK(id);
	CHK(reg_ofs_base);
	CHK(reg_ofs_cur);
	CHK(reg_ofs_end);
	CHK(reg_ofs_base_msb);
	CHK(reg_ofs_cur_msb);
	CHK(reg_ofs_end_msb);
	CHK(fs_reg);
	CHK(fs_shift);
	CHK(fs_maskbit);
	CHK(mono_reg);
	CHK(mono_shift);
	CHK(mono_invert);
	CHK(quad_ch_reg);
	CHK(quad_ch_mask);
	CHK(quad_ch_shift);
	CHK(int_odd_flag_reg);
	CHK(int_odd_flag_shift);
	CHK(enable_reg);
	CHK(enable_shift);
	CHK(hd_reg);
	CHK(hd_shift);
	CHK(msb_reg);
	CHK(msb_shift);
	CHK(msb2_reg);
	CHK(msb2_shift);
	CHK(agent_disable_reg);
	CHK(agent_disable_shift);
	CHK(ch_num_reg);
	CHK(ch_num_shift);
	CHK(ch_num_maskbit);
	// These fields are unused in the current driver, can't test dead code
	//CHK(hd_align_reg);
	//CHK(hd_align_mshift);
	//CHK(pbuf_reg);
	//CHK(pbuf_mask);
	//CHK(pbuf_shift);
	//CHK(minlen_reg);
	//CHK(minlen_mask);
	//CHK(minlen_shift);
}

/* Some properties may be skipped/defaulted in DTS */
#define COND_PROP(n, prop) \
	IF_ENABLED(DT_NODE_HAS_PROP(n, prop), (.prop = DT_PROP(n, prop),))

#define GENAFE(n) { \
	.afe_name = DT_PROP(n, afe_name), \
	.dai_id = DT_PROP(n, dai_id), \
	.downlink = DT_PROP(n, downlink), \
	.mono_invert = DT_PROP(n, mono_invert), \
	.base = DT_PROP(n, base), \
	.end = DT_PROP(n, end), \
	.cur = DT_PROP(n, cur), \
	.fs = DT_PROP(n, fs), \
	.hd = DT_PROP(n, hd), \
	.enable = DT_PROP(n, enable), \
	COND_PROP(n, mono) \
	COND_PROP(n, quad_ch) \
	COND_PROP(n, int_odd) \
	COND_PROP(n, msb) \
	COND_PROP(n, msb2) \
	COND_PROP(n, agent_disable) \
	COND_PROP(n, ch_num) \
	},

static const struct afe_cfg afes[] = {
	DT_FOREACH_STATUS_OKAY(mediatek_afe, GENAFE)
};

// Create an uninitialized array of memif structs to be initialized
// from the data in afes[]
#define EMPTY_STRUCT(n) {},
static struct mtk_base_memif_data afe_memifs[] = {
	DT_FOREACH_STATUS_OKAY(mediatek_afe, EMPTY_STRUCT)
};

static struct dai mtk_dais[] = {
	DT_FOREACH_STATUS_OKAY(mediatek_afe, EMPTY_STRUCT)
};

static void afe_check(void)
{
	int n = ARRAY_SIZE(mtk_memif_data);

	__ASSERT_NO_MSG(ARRAY_SIZE(afes) == n);

	for (int i = 0; i < n; i++) {
		printk("\nCheck AFE %d:\n", i);
		cfg_cmp(&mtk_memif_data[i], &afe_memifs[i]);
	}
}

////////////////////////////////////////////////////////////////////////

extern const struct dma_ops memif_ops;

/* FIXME: this field is mostly dead code, nothing outside the legacy
 * platform layers uses it beyond logging and (maybe?) assuming
 * uniqueness.
 */
enum dma_id {
	DMA_ID_AFE_MEMIF,
	DMA_ID_HOST,
};

extern const struct dma_ops dummy_dma_ops;

static struct dma mtk_dma[] = {
	{
		.plat_data = {
			.id		= DMA_ID_HOST,
			.dir		= DMA_DIR_HMEM_TO_LMEM | DMA_DIR_LMEM_TO_HMEM,
			.devs		= DMA_DEV_HOST,
			.channels	= 16,
		},
		.ops	= &dummy_dma_ops,
	},
	{
		.plat_data = {
			.id =  DMA_ID_AFE_MEMIF,
			.dir = DMA_DIR_MEM_TO_DEV | DMA_DIR_DEV_TO_MEM,
			.devs = DMA_DEV_AFE_MEMIF,
			.base = MTK_AFE_BASE,
			.channels = ARRAY_SIZE(mtk_dais),
		},
		.ops = &memif_ops,
	},
};

static const struct dma_info mtk_dma_info = {
	.dma_array = mtk_dma,
	.num_dmas = ARRAY_SIZE(mtk_dma),
};

static const struct dai_type_info mtk_dai_types[] = {
        { .type = SOF_DAI_MEDIATEK_AFE,
	  .dai_array = mtk_dais,
	  .num_dais = ARRAY_SIZE(mtk_dais), },
};

static const struct dai_info mtk_dai_info = {
        .dai_type_array = mtk_dai_types,
        .num_dai_types = ARRAY_SIZE(mtk_dai_types),
};

static unsigned int mtk_afe_fs_timing(unsigned int rate)
{
	/* Static table of fs register values.  TODO: sort it and binary search */
	static const struct { int hz, reg; } rate2reg[] = {
		{   8000,  0 },
		{  11025,  1 },
		{  12000,  2 },
		{  16000,  4 },
		{  22050,  5 },
		{  24000,  6 },
		{  32000,  8 },
		{  44100,  9 },
		{  48000, 10 },
		{  88200, 13 },
		{  96000, 14 },
		{ 176400, 17 },
		{ 192000, 18 },
		{ 352800, 21 },
		{ 384000, 22 },
	};

	for (int i = 0; i < ARRAY_SIZE(rate2reg); i++)
		if (rate2reg[i].hz == rate)
			return rate2reg[i].reg;
	return -EINVAL;
}

static unsigned int mtk_afe_fs(unsigned int rate, int aud_blk)
{
	return mtk_afe_fs_timing(rate);
}

/* Global symbol referenced by AFE driver */
struct mtk_base_afe_platform mtk_afe_platform = {
	.base_addr = MTK_AFE_BASE,
	.memif_datas = mtk_memif_data,
	.memif_size = ARRAY_SIZE(mtk_memif_data),
	.memif_dl_num = MTK_DL_NUM,
	.memif_32bit_supported = 0,
	.irq_datas = NULL,
	.irqs_size = 0,
	.dais_size = ARRAY_SIZE(mtk_dais),
	.afe_fs = mtk_afe_fs,
	.irq_fs = mtk_afe_fs_timing,
};

int mtk_dai_init(struct sof *sof)
{
	int i;

	/* Note: assumes that the order of entries in DTS matches the
	 * legacy mtk_dais[] array defined above.  We should construct
	 * the former from DTS too.
	 */
	for (i = 0; i < ARRAY_SIZE(afes); i++) {
		afe_memifs[i].id = i;
		cfg_convert(&afes[i], &afe_memifs[i]);

		/* Also initialize the dais array */
		mtk_dais[i].index = i;
		mtk_dais[i].drv = &afe_dai_driver;

		/* Also construct the mtk_dais[] array, which is the
		 * mapping from the host-visible DAI index to a driver
		 * defined in afe_memifs[].  The mapping is ad-hoc,
		 * and stored, bitpacked, in the "handshake" variable
		 * in plat data.  The DAI index is the low byte, the
		 * AFE index is in the third byte.  There is an IRQ
		 * traditionally defined in the middle byte but unused
		 * here because the driver doesn't support
		 * interrupts.
		 */
		int di = afes[i].dai_id;
		int hs = (i << 16) | di;

		mtk_dais[di].plat_data.fifo[0].handshake = hs;
	}

	/* The legacy driver wants all the DL devices at the start of
	 * the array, validate and compute dl_num.
	 */
	int num_dl = 0;
	for (i = 0; i < ARRAY_SIZE(afes); i++) {
		if (!afes[i].downlink) {
			num_dl = i;
			break;
		}
	}
	for (/**/; i < ARRAY_SIZE(afes); i++) {
		__ASSERT_NO_MSG(!afes[i].downlink);
	}

	printk("ANDY afe dl_num %d\n", num_dl);
	mtk_afe_platform.memif_dl_num = num_dl;
	mtk_afe_platform.memif_datas = afe_memifs;

	afe_check();
	for (int i = 0; i < ARRAY_SIZE(mtk_dais); i++)
		printk("ANDY dai %d handshake 0x%8.8x\n", i,
		       mtk_dais[i].plat_data.fifo[0].handshake);

        sof->dai_info = &mtk_dai_info;
	sof->dma_info = &mtk_dma_info;
        return 0;
}
