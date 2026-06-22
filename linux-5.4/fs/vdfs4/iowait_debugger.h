#ifndef __IOWAIT_DEBUGGER_H__

enum iowait_ops_type {
	IOWAIT_OPS_READ,
	IOWAIT_OPS_WRITE,
	IOWAIT_OPS_LOOKUP,
	IOWAIT_OPS_FSYNC,
	IOWAIT_OPS_FDSYNC,
	IOWAIT_OPS_FALLOCATE,
	IOWAIT_OPS_MAX,
};

#ifdef CONFIG_VDFS4_IOWAIT_DEBUGGER
struct iowait_dbg_info {
	u64 start_ns;
	u64 blkio_delay_start;
};

int vdfs4_iowait_debugger_init(void);
void vdfs4_iowait_debugger_destroy(void);
void vdfs4_iowait_debugger_start(struct iowait_dbg_info *info);
void vdfs4_iowait_debugger_finish(struct iowait_dbg_info *info,
		  enum iowait_ops_type ops, struct inode *inode);
void vdfs4_iowait_debugger_rw_finish(struct iowait_dbg_info *info,
		  enum iowait_ops_type ops, struct inode *inode, ssize_t sz);
#else
struct iowait_dbg_info {};

static inline int vdfs4_iowait_debugger_init(void) {return 0;}
static inline void vdfs4_iowait_debugger_destroy(void) {}
static inline void vdfs4_iowait_debugger_start(struct iowait_dbg_info *info) {}
static inline void vdfs4_iowait_debugger_finish(struct iowait_dbg_info *info,
		enum iowait_ops_type ops, struct inode *inode) {}
static inline void vdfs4_iowait_debugger_rw_finish(struct iowait_dbg_info *info,
		enum iowait_ops_type ops, struct inode *inode, ssize_t sz) {}
#endif

#endif
