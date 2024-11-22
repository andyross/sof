#include <sof/lib/dai-legacy.h>
#include <ipc/dai.h>

#define DAI_HANDSHAKE(dai, irq, chan) ((chan << 16) | (irq << 8) | dai)

#define DAI_DEFINE(dai, irq, chan)		\
	{ .index = dai, .drv = &afe_dai_driver,	\
	  .plat_data = { .fifo = {		\
		{ .handshake = DAI_HANDSHAKE(dai, irq, chan), } } } }

extern const struct dai_driver afe_dai_driver;

////////////////////////////////////////////////////////////////////////
// Cribbed from mt8196, need to source from DTS
//
// Notes: basically there is only one "type" of DAI in mtk, each dai
// represents one "device" equivalent, which is parametrized by an
// "index" (corresponds to the memif used) an irq number and a channel
// enumerant.
//
// FIXME: I *think* these enumerants exist only within SOF and aren't
// hardware config or understood by the kernel?  Maybe it matches on
// handshake?
enum {
        MT8196_DAI_I2S_OUT4, /* speaker */
        MT8196_DAI_I2S_OUT6, /* headset out */
        MT8196_DAI_AP_DMIC, /* DMIC */
        MT8196_DAI_I2S_IN6, /* headset mic */
        MT8196_DAI_AP_DMIC_CH34,
        MT8196_DAI_NUM,
};

enum {
        MT8196_MEMIF_DL1,
        MT8196_MEMIF_DL_24CH,
        MT8196_MEMIF_UL0,
        MT8196_MEMIF_UL1,
        MT8196_MEMIF_UL2,
};

static struct dai mtk_dais[] = {
        DAI_DEFINE(MT8196_DAI_I2S_OUT4,     12, MT8196_MEMIF_DL_24CH),
        DAI_DEFINE(MT8196_DAI_I2S_OUT6,      1, MT8196_MEMIF_DL1),
        DAI_DEFINE(MT8196_DAI_AP_DMIC,      13, MT8196_MEMIF_UL0),
        DAI_DEFINE(MT8196_DAI_I2S_IN6,       0, MT8196_MEMIF_UL1),
        DAI_DEFINE(MT8196_DAI_AP_DMIC_CH34, 15, MT8196_MEMIF_UL2),
};

////////////////////////////////////////////////////////////////////////

static const struct dai_type_info mtk_dai_types[] = {
        { .type = SOF_DAI_MEDIATEK_AFE,
	  .dai_array = mtk_dais,
	  .num_dais = ARRAY_SIZE(mtk_dais), },
};

static const struct dai_info mtk_dai_info = {
        .dai_type_array = mtk_dai_types,
        .num_dai_types = ARRAY_SIZE(mtk_dai_types),
};

int mtk_dai_init(struct sof *sof)
{
	printk("ANDY %s:%d\n", __func__, __LINE__);
        sof->dai_info = &mtk_dai_info;
        return 0;
}
