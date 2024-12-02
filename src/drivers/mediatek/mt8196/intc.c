/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2023. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <rtos/interrupt.h>
#include <sof/lib/memory.h>
#include <sof/platform.h>
#include <sof/lib/uuid.h>
#include <platform/drivers/interrupt.h>
#include <platform/drivers/intc.h>
#include <errno.h>
#include <stdint.h>

SOF_DEFINE_REG_UUID(mtk_intc);
DECLARE_TR_CTX(intc_tr, SOF_UUID(mtk_intc_uuid), LOG_LEVEL_INFO);

static struct intc_desc_t intc_desc;

#ifdef CFG_CORE_OFF_SUPPORT
#endif
#ifdef CFG_TICKLESS_SUPPORT
#endif

void intc_init(void)
{
	uint32_t word, group, irq;

	for (group = 0; group < INTC_GRP_NUM; group++) {
		for (word = 0; word < INTC_GRP_LEN; word++)
			intc_desc.grp_irqs[group][word] = 0x0;
	}

	for (word = 0; word < INTC_GRP_LEN; word++)
		intc_desc.int_en[word] = 0x0;

	for (irq = 0; irq < IRQ_MAX_CHANNEL; irq++) {
		intc_desc.irqs[irq].id = irq;
		intc_desc.irqs[irq].group = irq2grp_map[irq];
		intc_desc.irqs[irq].pol = INTC_POL_LOW;
	}

	for (word = 0; word < INTC_GRP_LEN; word++) {
		io_reg_write(INTC_IRQ_EN(word), 0x0);
		io_reg_write(INTC_IRQ_WAKE_EN(word), 0x0);
		io_reg_write(INTC_IRQ_STAGE1_EN(word), 0x0);
		io_reg_write(INTC_IRQ_POL(word), 0xFFFFFFFF);
	}

	for (group = 0; group < INTC_GRP_NUM; group++) {
		for (word = 0; word < INTC_GRP_LEN; word++)
			io_reg_write(INTC_IRQ_GRP(group, word), 0x0);
	}
}

void intc_irq_unmask(IRQn_Type irq)
{
	uint32_t word; //, group;

	if (irq < IRQ_MAX_CHANNEL && intc_desc.irqs[irq].group < INTC_GRP_NUM) {
		word = INTC_WORD(irq);
		//group = intc_desc.irqs[irq].group;
		io_reg_update_bits(INTC_IRQ_EN(word), INTC_BIT(irq), INTC_BIT(irq));
		//xt_ints_on(1 << grp2hifi_irq_map[group]);
	} else
		tr_err(&intc_tr, "INTC fail to unmask irq\n");
}

void intc_irq_mask(IRQn_Type irq)
{
	uint32_t word;

	if (irq < IRQ_MAX_CHANNEL) {
		word = INTC_WORD(irq);
		io_reg_update_bits(INTC_IRQ_EN(word), INTC_BIT(irq), 0);
	} else
		tr_err(&intc_tr, "INTC fail to mask irq\n");
}

int intc_irq_enable(IRQn_Type irq)
{
	uint32_t word, irq_b, group, pol;

	if (irq < IRQ_MAX_CHANNEL && intc_desc.irqs[irq].group < INTC_GRP_NUM &&
	    intc_desc.irqs[irq].pol < INTC_POL_NUM) {
		word = INTC_WORD(irq);
		irq_b = INTC_BIT(irq);
		group = intc_desc.irqs[irq].group;
		pol = intc_desc.irqs[irq].pol;

		intc_desc.int_en[word] |= irq_b;
		intc_desc.grp_irqs[group][word] |= irq_b;
		io_reg_update_bits(INTC_IRQ_EN(word), irq_b, 0);
		if (pol == INTC_POL_HIGH)
			io_reg_update_bits(INTC_IRQ_POL(word), irq_b, 0);
		else
			io_reg_update_bits(INTC_IRQ_POL(word), irq_b, irq_b);
		io_reg_update_bits(INTC_IRQ_GRP(group, word), irq_b, irq_b);
		io_reg_update_bits(INTC_IRQ_EN(word), irq_b, irq_b);
		return 1;
	} else {
		tr_err(&intc_tr, "INTC fail to enable irq %u\n", irq);
		return 0;
	}
}

int intc_irq_disable(IRQn_Type irq)
{
	uint32_t word, irq_b, group;

	if (irq < IRQ_MAX_CHANNEL && intc_desc.irqs[irq].group < INTC_GRP_NUM) {
		word = INTC_WORD(irq);
		irq_b = INTC_BIT(irq);
		group = intc_desc.irqs[irq].group;

		//xt_ints_off(1 << grp2hifi_irq_map[group]); //sw disable irq
		intc_desc.int_en[word] &= ~irq_b;
		intc_desc.grp_irqs[group][word] &= ~irq_b;
		io_reg_update_bits(INTC_IRQ_EN(word), irq_b, 0);
		io_reg_update_bits(INTC_IRQ_GRP(group, word), irq_b, 0);

		return 1;
	} else {
		tr_err(&intc_tr, "INTC fail to disable irq %u\n", irq);
		return 0;
	}
}

