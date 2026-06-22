#include <linux/debugfs.h>
#include <linux/spinlock.h>
#include <linux/delayacct.h>
#include <linux/vlogger.h>

#include "vdfs4.h"
#include "iowait_debugger.h"

static char *ops_name[IOWAIT_OPS_MAX] = {
	"read",
	"write",
	"lookup",
	"fsync",
	"fdsync",
	"falloc",
};

static struct dentry *droot = NULL;

__read_mostly static bool enable = true;
__read_mostly static u32 iowait_us = 100000;
__read_mostly static bool tasks_rt = true;
__read_mostly static u32 tasks_high_prio = 102;
__read_mostly static bool tasks_all = false;
__read_mostly static bool ops_sync = true;

void vdfs4_iowait_debugger_start(struct iowait_dbg_info *info)
{
	info->start_ns = ktime_get_ns();

	if (current->delays)
		info->blkio_delay_start = current->delays->blkio_delay;
	else
		info->blkio_delay_start = 0;
}

static inline bool check_conditions(struct iowait_dbg_info *iow_info,
			    enum iowait_ops_type ops, u64 end, u64 blk_end)
{
	if (!enable)
		return false;

	if (ktime_us_delta(blk_end, iow_info->blkio_delay_start) <= iowait_us
	    && ktime_us_delta(end, iow_info->start_ns) <= iowait_us)
		return false;

	if (ops == IOWAIT_OPS_FALLOCATE)
		return true;

	if (ktime_us_delta(blk_end, iow_info->blkio_delay_start) <= iowait_us)
		return false;

	if (tasks_all)
		return true;

	if (tasks_rt && current->prio < MAX_RT_PRIO)
		return true;

	if (tasks_high_prio && current->prio <= tasks_high_prio)
		return true;

	if (ops_sync && strstr(ops_name[ops], "sync"))
		return true;

	return false;
}

#define VLOG_MSG_LENGTH (256)
void vdfs4_iowait_debugger_rw_finish(struct iowait_dbg_info *iow_info,
		  enum iowait_ops_type ops, struct inode *inode, ssize_t sz)
{
	u64 syscfs, end_ns, blkio_delay_end;
	uint64_t start, end;
	uint32_t start_rem, end_rem;
	char *filename;

	end_ns = ktime_get_ns();
	blkio_delay_end = current->delays ? current->delays->blkio_delay : 0;

	if (!check_conditions(iow_info, ops, end_ns, blkio_delay_end))
		return;

	if (!IS_ERR_OR_NULL(inode) && VDFS4_I(inode)->name)
		filename = VDFS4_I(inode)->name;
	else
		filename = "(null)";

	start = (uint64_t)ktime_divns(iow_info->start_ns, NSEC_PER_USEC);
	start_rem = do_div(start, USEC_PER_SEC);
	end = (uint64_t)ktime_divns(end_ns, NSEC_PER_USEC);
	end_rem = do_div(end, USEC_PER_SEC);

	syscfs = current->ioac.syscfs;

	vlog_info("VTRACE", "[VIO] blkdelay:%7lld us (%12.12s,P%4d,T%4d,f:%12.12s,ops:%-6s,sz:%5zd)(%4llu.%03u~%4llu.%03u,%4lldms,s:%4llu) #[PTC]",
		  ktime_us_delta(blkio_delay_end, iow_info->blkio_delay_start),
		  current->comm, current->tgid, current->pid, filename,
		  ops_name[ops], DIV_ROUND_UP(sz, 4096),
		  start, start_rem / USEC_PER_MSEC,
		  end, end_rem / USEC_PER_MSEC,
		  ktime_ms_delta(end_ns, iow_info->start_ns), syscfs);
	return;
}

void vdfs4_iowait_debugger_finish(struct iowait_dbg_info *iow_info,
		  enum iowait_ops_type ops, struct inode *inode)
{
	vdfs4_iowait_debugger_rw_finish(iow_info, ops, inode, 0);
}


__init int vdfs4_iowait_debugger_init(void)
{
	int ret = 0;
	struct dentry *dent, *dtasks;

	droot = debugfs_create_dir("iow", NULL);
	if (!droot)
		return -EFAULT;

	dent = debugfs_create_bool("enable", 0644, droot, &enable);
	if (!dent) {
		ret = -EFAULT;
		goto out;
	}

	dent = debugfs_create_u32("iowait_us", 0644, droot, &iowait_us);
	if (!dent) {
		ret = -EFAULT;
		goto out;
	}

	dtasks = debugfs_create_dir("tasks", droot);
	if (!dtasks) {
		ret = -EFAULT;
		goto out;
	}

	dent = debugfs_create_bool("real_time", 0644, dtasks, &tasks_rt);
	if (!dent) {
		ret = -EFAULT;
		goto out;
	}

	dent = debugfs_create_u32("high_prio", 0644, dtasks, &tasks_high_prio);
	if (!dent) {
		ret = -EFAULT;
		goto out;
	}

	dent = debugfs_create_bool("all", 0644, dtasks, &tasks_all);
	if (!dent) {
		ret = -EFAULT;
		goto out;
	}

	dent = debugfs_create_bool("ops_sync", 0644, dtasks, &ops_sync);
	if (!dent) {
		ret = -EFAULT;
		goto out;
	}

	return ret;

out:
	pr_err("Failed to init iow debugger\n");
	debugfs_remove_recursive(droot);

	return ret;
}

void vdfs4_iowait_debugger_destroy(void)
{
	if (!droot)
		return;

	debugfs_remove_recursive(droot);
}

