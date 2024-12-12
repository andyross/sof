#ifndef _SOF_PLATFORM_MTK_DMA_H
#define _SOF_PLATFORM_MTK_DMA_H

/* Only needed in dma_multi_chan_domain.c */
#define PLATFORM_NUM_DMACS 2
#define PLATFORM_MAX_DMA_CHAN 32

#define dma_chan_irq(dma, chan)		dma_irq(dma)
#define dma_chan_irq_name(dma, chan)	dma_irq_name(dma)

#endif /* _SOF_PLATFORM_MTK_DMA_H */
