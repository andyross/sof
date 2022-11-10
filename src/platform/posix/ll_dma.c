#include <sof/audio/component.h>
#include <sof/schedule/ll_schedule_domain.h>

static int lld_domain_register(struct ll_schedule_domain *domain, struct task *task,
			   void handler(void *arg), void *arg)
{
	printk("=== %s()\n", __func__);
	return 0;
}

static int lld_domain_unregister(struct ll_schedule_domain *domain, struct task *task,
			     uint32_t num_tasks)
{
	printk("=== %s()\n", __func__);
	return 0;
}

static void lld_domain_enable(struct ll_schedule_domain *domain, int core)
{
	printk("=== %s()\n", __func__);
}

static void lld_domain_disable(struct ll_schedule_domain *domain, int core)
{
	printk("=== %s()\n", __func__);
}

static void lld_domain_set(struct ll_schedule_domain *domain, uint64_t start)
{
	printk("=== %s()\n", __func__);
}

static void lld_domain_clear(struct ll_schedule_domain *domain)
{
	printk("=== %s()\n", __func__);
}

static bool lld_domain_is_pending(struct ll_schedule_domain *domain, struct task *task,
			      struct comp_dev **comp)
{
	printk("=== %s()\n", __func__);
	return 0;
}

static void lld_domain_task_cancel(struct ll_schedule_domain *domain, uint32_t num_tasks)
{
	printk("=== %s()\n", __func__);
}

static const struct ll_schedule_domain_ops ops = {
	.domain_register = lld_domain_register,
	.domain_unregister = lld_domain_unregister,
	.domain_enable = lld_domain_enable,
	.domain_disable = lld_domain_disable,
	.domain_set = lld_domain_set,
	.domain_clear = lld_domain_clear,
	.domain_is_pending = lld_domain_is_pending,
	.domain_task_cancel = lld_domain_task_cancel,
};

static struct ll_schedule_domain dom;

struct ll_schedule_domain *ll_dma_init(void)
{
	printk("=== %s()\n", __func__);
	dom.ops = &ops;
	return &dom;
}
