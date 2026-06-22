#ifndef _DECOMP_TRACE_H
#define _DECOMP_TRACE_H

#include <linux/time.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/console.h>
#include <linux/sched.h>

#define DECOMP_TRACE_LIMIT 25000

struct decomp_trace_data {
	struct vdfs4_sb_info *sbi;	/* sbi */
	unsigned long i_ino;		/* i_ino */
	unsigned int chunk_idx;		/* chunk idx */
	unsigned int comp_sz;		/* compressed size */
	unsigned int pages_count;	/* decompress page count */
	ktime_t start;			/* decomp start time */
	ktime_t mid;			/* mid time (S/W -> decomp start) */
	ktime_t end;			/* decomp end time */
	unsigned char hw_in_flight;	/* number of in in_flight works */
	unsigned char sw_in_flight;	/* number of in in_flight works */
	unsigned char type;		/* H/W or S/W */
	unsigned char comp_type;	/* comp_type : hw_iovec_comp_type */
	unsigned char complete;
};

enum {
	TRACE_HW_DECOMP,
	TRACE_SW_DECOMP,
	TRACE_ERR = 10,
	TRACE_HW_DECOMP_ERR = 10,
	TRACE_SW_DECOMP_ERR
};

#define TRACE_DISABLE	0
#define TRACE_ENABLE	1

#if defined(CONFIG_VDFS4_DECOMP_TRACE)
int trace_decomp_start(struct inode *inode, unsigned char type);
void trace_decomp_mid(int idx, struct inode *inode);
void trace_decomp_finish(int idx, struct inode *inode, unsigned int chunk_idx,
		unsigned int comp_size, unsigned char type, int ret, int uncomp);
#else
static inline int trace_decomp_start(struct inode *inode, unsigned char type)
{
	 return 0;
};
static inline void trace_decomp_mid(int idx, struct inode *inode){};
static inline void trace_decomp_finish(int idx, struct inode *inode, unsigned int chunk_idx,
		unsigned int comp_size, unsigned char type, int ret, int uncomp){};
#endif

#endif
