// SPDX-License-Identifier: GPL-2.0
/*
 * fs/vdfs4/decomp_trace.c
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 * Created by: Jungseung Lee (js07.lee@samsung.com)
 *
 */
#include <linux/proc_fs.h>
#include <linux/module.h>

#include "vdfs4.h"
#include "decomp_trace.h"

#define BUF_SIZE	160
struct decomp_trace_data *DTQ;
static unsigned int decomp_trace_idx;
static unsigned int queue_is_full;
static unsigned int init_setup_needed __initdata;

static atomic_t hw_decomp_count;
static atomic_t sw_decomp_count;

static DEFINE_SPINLOCK(decomp_trace_lock);
static DECLARE_WAIT_QUEUE_HEAD(dtq_wait);

struct decomp_trace_ctrl {
	unsigned int mode : 1;
};
static struct decomp_trace_ctrl trace_ctrl = {
	.mode			= TRACE_DISABLE, /*should be DISABLE before allocate DTQ*/
};

/**
 * set_init_decomp_trace_mode() - set init setup needed flag
 * @s: user input
 *
 * set only init_setup_needed,
 * because cannot allocate buffer before kernel initialization.
 *
 * Return: Returns 0 on success, errno on failure
 */
static int __init set_init_decomp_trace_mode(char *s)
{
	if (!s)
		return 0;

	init_setup_needed = 1;
	pr_info("enable decomp trace(%s)\n", s);
	return 0;
}
early_param("flash_trace", set_init_decomp_trace_mode);

static void decomp_trace_on(void)
{
	if (trace_ctrl.mode == TRACE_ENABLE)
		return;

	if (!DTQ)
		DTQ = vmalloc(DECOMP_TRACE_LIMIT * sizeof(struct decomp_trace_data));

	if (!DTQ) {
		pr_err("allocate DTQ fail!\n");
		return;
	}

	trace_ctrl.mode = TRACE_ENABLE;
}

static void __dtq_data_to_str(struct decomp_trace_data *dtd,
			int idx, char *buf, unsigned int length)
{
	int ret;
	unsigned int _idx = idx % DECOMP_TRACE_LIMIT;
	unsigned int us = ktime_to_us(dtd[_idx].start);
	struct inode *inode = NULL;
	char file_name[25] = "";

	if (dtd[_idx].sbi && dtd[_idx].i_ino)
		inode = vdfs4_iget(dtd[_idx].sbi, dtd[_idx].i_ino);

	if (inode && virt_addr_valid(VDFS4_I(inode)->name)) {
		strncpy(file_name, VDFS4_I(inode)->name, sizeof(file_name) - 1);
		file_name[sizeof(file_name) - 1] = '\0';
	}

	iput(inode);

	if (dtd[_idx].complete)
		ret = snprintf(buf, length - 1,
			"[%6d] %5lld.%06ld F: %25s(%5ld)[%3d] SZ: %6d/%3dK t:%6lld(%6lld)us in_flight: %d/%d %s%s%s",
			idx,						/* idx */
			ktime_divns(dtd[_idx].start, NSEC_PER_SEC),	/* start time */
			us % USEC_PER_SEC,
			file_name,
			dtd[_idx].i_ino,
			dtd[_idx].chunk_idx,
			dtd[_idx].comp_sz,				/* sz */
			dtd[_idx].pages_count * 4,			/* decomp pages count */
			ktime_us_delta(dtd[_idx].end,dtd[_idx].start),	/* elapsed time */
			ktime_us_delta(dtd[_idx].end,dtd[_idx].mid),	/* elapsed time */
			dtd[_idx].hw_in_flight,
			dtd[_idx].sw_in_flight,
			dtd[_idx].type % TRACE_ERR ? "[S]" : "[H]",
			dtd[_idx].type / TRACE_ERR ? "(BUSY)" : "",
			dtd[_idx].comp_type ? "'" : "");
	else
		ret = snprintf(buf, length - 1,
			"[%6d] %5lld.%06ld F: %25s(%5ld)[%3d] SZ: %6d/%3dK t:%6lld(%6lld)us in_flight: %d/%d %s%s",
			idx,						/* idx */
			ktime_divns(dtd[_idx].start, NSEC_PER_SEC),	/* start time */
			us % USEC_PER_SEC,
			file_name,
			dtd[_idx].i_ino,
			0, 0, 0, 0LL, 0LL, 0, 0,
			"(INCOMPLETE)", "");
}

static inline int chunk_page_count(struct inode *inode,
		int comp_extent_idx)
{
	struct vdfs4_inode_info *inode_i = VDFS4_I(inode);
	loff_t i_size;

	/* size of all chunks except last one is
	 * 1 << inode_i->fbc->log_chunk_size */
	if (comp_extent_idx < (inode_i->fbc->comp_extents_n - 1))
		goto log_chunk_size;

	i_size = i_size_read(inode);
	/* get size of last chunk */
	i_size &= (loff_t)((1<<inode_i->fbc->log_chunk_size) - 1);

	/* i_size is aligned to (1<<inode_i->fbc->log_chunk_size) */
	if (!i_size)
		goto log_chunk_size;

	/* div round up by PAGE_SIZE */
	return (i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

log_chunk_size:
	return (1 << (inode_i->fbc->log_chunk_size - PAGE_SHIFT));
}

int trace_decomp_start(struct inode *inode, unsigned char type) {

	int idx;
	u64 ts_start_nsec;

	if (trace_ctrl.mode == TRACE_DISABLE)
		return -1;

	ts_start_nsec = ktime_get_ns();

	spin_lock(&decomp_trace_lock);

	idx = decomp_trace_idx % DECOMP_TRACE_LIMIT;

	memset(&DTQ[idx], 0x00, sizeof(struct decomp_trace_data));

	decomp_trace_idx++;

	if (unlikely(decomp_trace_idx == DECOMP_TRACE_LIMIT))
		queue_is_full = 1;

	spin_unlock(&decomp_trace_lock);

	DTQ[idx].start = ts_start_nsec;
	DTQ[idx].sbi = VDFS4_SB(inode->i_sb);
	DTQ[idx].i_ino = inode->i_ino;
	DTQ[idx].type = TRACE_ERR;

	if (type == TRACE_HW_DECOMP)
		atomic_inc(&hw_decomp_count);
	else
		atomic_inc(&sw_decomp_count);

	DTQ[idx].hw_in_flight = atomic_read(&hw_decomp_count);
	DTQ[idx].sw_in_flight = atomic_read(&sw_decomp_count);

	return idx;
}

void trace_decomp_mid(int idx, struct inode *inode)
{
	u64 ts_mid_nsec;

	if (trace_ctrl.mode == TRACE_DISABLE)
		return;

	ts_mid_nsec = ktime_get_ns();

	DTQ[idx].mid = ts_mid_nsec;
}

void trace_decomp_finish(int idx, struct inode *inode, unsigned int chunk_idx,
			unsigned int comp_sz, unsigned char type, int ret, int uncomp)
{
	u64 ts_end_nsec;

	if (trace_ctrl.mode == TRACE_DISABLE)
		return;

	if (idx == -1)
		return;

	ts_end_nsec = ktime_get_ns();

	DTQ[idx].end = ts_end_nsec;
	DTQ[idx].chunk_idx = chunk_idx;
	DTQ[idx].comp_sz = comp_sz;
	DTQ[idx].pages_count = chunk_page_count(inode, chunk_idx);
	DTQ[idx].comp_type = uncomp;
	DTQ[idx].complete = 1;	/* complete */

	if (ret)
		DTQ[idx].type = type + TRACE_ERR;
	else
		DTQ[idx].type = type;

	if (type == TRACE_HW_DECOMP) {
		atomic_dec(&hw_decomp_count);
		DTQ[idx].mid = DTQ[idx].end;	/* just clear */
	} else {
		atomic_dec(&sw_decomp_count);
	}

	wake_up_interruptible(&dtq_wait);

	return;
}

static int _decomp_trace_control(char *pbuffer, size_t count)
{
	int ret = 0;
	char* name = NULL;
	char* value = NULL;
	char* help_msg = "[usage : echo options > decomp_trace]\n"
			" - option list -\n"
			"    on|off		: disable or enable trace \n"
			"    clear		: clear mmc trace queue(stop read before clear)\n"
			"    state		: show current config\n";

	#define IS_SAME(value, str) ((value)&&(str)  \
				&& (strlen(value) == strlen(str))  \
				&& !(strncasecmp((value),(str),sizeof((str)))))

	name = strim(strsep(&pbuffer, "="));
	if (pbuffer)
		value = strim(strsep(&pbuffer, "="));
	pr_debug("decomp_trace control : %s(%s-%s)\n", pbuffer, name, value);

	if (IS_SAME(name, "on")) {
		pr_debug( "decomp_trace enabled\n");
		decomp_trace_on();
		goto out;
	}

	if (IS_SAME(name, "off")) {
		pr_debug( "decomp_trace disabled\n");
		trace_ctrl.mode = TRACE_DISABLE;
		goto out;
	}

	if (IS_SAME(name, "clear")) {
		pr_debug( "decomp_trace queue clear\n");
		spin_lock(&decomp_trace_lock);
		decomp_trace_idx = 0;
		queue_is_full = 0;
		spin_unlock(&decomp_trace_lock);
		goto out;
	}

	if (IS_SAME(name, "state")) {
		pr_err("[decomp_trace state]\n"
			"\t - decomp_trace mode : %s\n",
			((trace_ctrl.mode==TRACE_ENABLE)?"on":"off"));
		goto out;
	}

	ret = -EINVAL;
	pr_err("Unknown or Invalid vdfs trace option(%s:%s).\n",
		       name, value);
	pr_err("%s\n", help_msg);
	#undef IS_SAME
	return ret;

out:
	#undef IS_SAME
	return 0;
}


static int proc_decomp_trace_open(struct inode *inode, struct file *file)
{
	/* If queue is full, Ignore oldest record. It might use the buffer for next cmd */
	if (queue_is_full)
		file->f_pos = (loff_t)(unsigned int)(decomp_trace_idx - DECOMP_TRACE_LIMIT + 1);
	else
		file->f_pos = (loff_t)(0);

	return 0;
}

static ssize_t proc_decomp_trace_read(struct file *file, char __user *buf,
			size_t size, loff_t *ppos)
{
	int ret;
	size_t written = 0;
	char str_buf[BUF_SIZE] = {0,};
	unsigned int idx = (uint32_t)(*ppos);

	if (size < sizeof(str_buf))
		return -ENOBUFS;

	if (!DTQ)
		return -EPERM;

	ret = wait_event_interruptible(dtq_wait, idx != decomp_trace_idx);

	if (ret)
		return ret;

	do {
		__dtq_data_to_str(DTQ, idx, str_buf, sizeof(str_buf));
		strncat(str_buf, "\n", sizeof(str_buf) - 1 -strlen(str_buf));
		ret = strlen(str_buf);
		copy_to_user(buf + written, str_buf, ret);
		written += ret;
		idx++;
	} while (idx != decomp_trace_idx && (written + sizeof(str_buf)) < size);

	*ppos = (loff_t)idx;

	return written;
}

static ssize_t proc_decomp_trace_write(struct file *file, const char __user *buf,
				    size_t count, loff_t *ppos)
{
	ssize_t ret = -EINVAL;
	char kbuf[64];

	if (count >= sizeof(kbuf))
		goto out;

	if (copy_from_user(kbuf, buf, count))
		goto out;

	kbuf[count] = '\0';

	ret = _decomp_trace_control(kbuf ,count);
	if (ret)
		goto out;

	/* Report a successful write */
	ret = count;
out:
	return ret;
}

static const struct file_operations proc_decomp_trace_fops = {
	.open	= proc_decomp_trace_open,
	.read	= proc_decomp_trace_read,
	.write	= proc_decomp_trace_write,
};

static int __init decomp_trace_init(void)
{
	proc_create("decomp_trace", S_IRUSR, NULL, &proc_decomp_trace_fops);

	if (init_setup_needed)
		decomp_trace_on();

	return 0;
}
module_init(decomp_trace_init);
