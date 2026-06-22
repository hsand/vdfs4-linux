#include <kunit/test.h>

#include "vdfs4.h"

struct vdfs4_vfs_inode_flags_param_t {
	const char *test_case_name;
	struct vdfs4_inode_info inode_info;
	unsigned int expected;
};

static void vdfs4_vfs_inode_flags_test_data_to_desc(
		const struct vdfs4_vfs_inode_flags_param_t *t, char *desc)
{
	strscpy(desc, t->test_case_name, KUNIT_PARAM_DESC_SIZE);
}

static const struct
vdfs4_vfs_inode_flags_param_t vdfs4_set_vfs_inode_flags_data[] = {
	{
		.test_case_name = "immutable enabled(1)",
		.inode_info = {
			.flags = BIT(VDFS4_IMMUTABLE),
			.vfs_inode = {
				.i_flags = S_SYNC | S_NOATIME | S_IMMUTABLE,
			},
		},
		.expected = S_SYNC | S_NOATIME | S_IMMUTABLE,
	},
	{
		.test_case_name = "immutable enabled(2)",
		.inode_info = {
			.flags = BIT(VDFS4_IMMUTABLE),
			.vfs_inode = {
				.i_flags = 0,
			},
		},
		.expected = S_IMMUTABLE,
	},
	{
		.test_case_name = "immutable disabled",
		.inode_info = {
			.flags = 0,
			.vfs_inode = {
				.i_flags = S_SYNC | S_NOATIME | S_IMMUTABLE,
			},
		},
		.expected = S_SYNC | S_NOATIME,
	},
};

KUNIT_ARRAY_PARAM(vdfs4_set_vfs_inode_flags, vdfs4_set_vfs_inode_flags_data,
		  vdfs4_vfs_inode_flags_test_data_to_desc);

static void vdfs4_set_vfs_inode_flags_test(struct kunit *test)
{
	struct vdfs4_vfs_inode_flags_param_t *param =
		(struct vdfs4_vfs_inode_flags_param_t *)(test->param_value);

	vdfs4_set_vfs_inode_flags(&param->inode_info.vfs_inode);

	KUNIT_EXPECT_EQ_MSG(test, param->expected,
		    param->inode_info.vfs_inode.i_flags,
		    "expected 0x%08x but mode value is 0x%08x vfs inode",
		    param->expected, param->inode_info.vfs_inode.i_flags);
}

static const struct
vdfs4_vfs_inode_flags_param_t vdfs4_get_vfs_inode_flags_data[] = {
	{
		.test_case_name = "immutable enabled(1)",
		.inode_info = {
			.flags = 0,
			.vfs_inode = {
				.i_flags = S_SYNC | S_NOATIME | S_IMMUTABLE,
			},
		},
		.expected = BIT(VDFS4_IMMUTABLE),
	},
	{
		.test_case_name = "immutable enabled(2)",
		.inode_info = {
			.flags = BIT(VDFS4_IMMUTABLE),
			.vfs_inode = {
				.i_flags = S_SYNC | S_NOATIME | S_IMMUTABLE,
			},
		},
		.expected = BIT(VDFS4_IMMUTABLE),
	},
	{
		.test_case_name = "immutable disabled",
		.inode_info = {
			.flags = BIT(VDFS4_IMMUTABLE),
			.vfs_inode = {
				.i_flags = 0,
			},
		},
		.expected = 0,
	},
};

KUNIT_ARRAY_PARAM(vdfs4_get_vfs_inode_flags, vdfs4_get_vfs_inode_flags_data,
		  vdfs4_vfs_inode_flags_test_data_to_desc);

static void vdfs4_get_vfs_inode_flags_test(struct kunit *test)
{
	struct vdfs4_vfs_inode_flags_param_t *param =
		(struct vdfs4_vfs_inode_flags_param_t *)(test->param_value);

	vdfs4_get_vfs_inode_flags(&param->inode_info.vfs_inode);

	KUNIT_EXPECT_EQ_MSG(test, param->expected,
		    (unsigned int)param->inode_info.flags,
		    "expected 0x%08x but mode value is 0x%08lx in vdfs4 inode",
		    param->expected, param->inode_info.flags);
}

struct fill_hlink_param_t {
	const char *test_case_name;
	struct inode inode;
	__le16 expected;
};

static const struct fill_hlink_param_t fill_hlink_test_data[] = {
	{
		.test_case_name = "regular type for hlink with U(rwx),G(rwx),O(rwx)",
		.inode.i_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO,
		.expected = 00100777,
	},
	{
		.test_case_name = "regular type for hlink",
		.inode.i_mode = S_IFREG,
		.expected = 00100000,
	},
	{
		.test_case_name = "block type for hlink",
		.inode.i_mode = S_IFBLK,
		.expected = 00060000,
	},
	{
		.test_case_name = "directory type for hlink",
		.inode.i_mode = S_IFDIR,
		.expected = 00040000,
	},
	{
		.test_case_name = "character Device type for hlink",
		.inode.i_mode = S_IFCHR,
		.expected = 00020000,
	},
	{
		.test_case_name = "fifo type for hlink",
		.inode.i_mode = S_IFIFO,
		.expected = 00010000,
	},
	{
		.test_case_name = "link type for hlink",
		.inode.i_mode = S_IFLNK,
		.expected = 00120000,
	},
	{
		.test_case_name = "fill_hlink: permission Read",
		.inode.i_mode = S_IRUSR | S_IRGRP | S_IROTH,
		.expected = 00444,
	},
	{
		.test_case_name = "fill_hlink: permission Write",
		.inode.i_mode = S_IWUSR | S_IWGRP | S_IWOTH,
		.expected = 00222,
	},
	{
		.test_case_name = "fill_hlink: permission Execute",
		.inode.i_mode = S_IXUSR | S_IXGRP | S_IXOTH,
		.expected = 00111,
	},
};

static void fill_hlink_test_data_to_desc(const struct fill_hlink_param_t *t,
					 char *desc)
{
	strscpy(desc, t->test_case_name, KUNIT_PARAM_DESC_SIZE);
}

KUNIT_ARRAY_PARAM(vdfs4_fill_hlink_value, fill_hlink_test_data,
		  fill_hlink_test_data_to_desc);

static void vdfs4_fill_hlink_value_test(struct kunit *test)
{
	struct fill_hlink_param_t *param =
		(struct fill_hlink_param_t *)(test->param_value);
	struct vdfs4_catalog_hlink_record hlink_record;

	vdfs4_fill_hlink_value(&param->inode, &hlink_record);

	KUNIT_EXPECT_EQ_MSG(test, param->expected, hlink_record.file_mode,
			    "expected 0x%04x but mode value is 0x%04x",
			    param->test_case_name,
			    param->expected, hlink_record.file_mode);
}

static void vdfs4_form_fork_test(struct kunit *test)
{
	struct vdfs4_fork fork;
	struct vdfs4_file_based_info fbc;
	struct vdfs4_inode_info *inode_info;
	struct inode *inode;

	inode_info = kunit_kzalloc(test, sizeof(struct vdfs4_inode_info),
				   GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, inode_info);

	inode_info->fbc = &fbc;
	inode_info->fork.total_block_count = 0x321;
	inode_info->fork.used_extents = 2;
	inode_info->fork.extents[0].first_block = 11;
	inode_info->fork.extents[0].block_count = 22;
	inode_info->fork.extents[0].iblock = 33;
	inode_info->fork.extents[1].first_block = 55;
	inode_info->fork.extents[1].block_count = 66;
	inode_info->fork.extents[1].iblock = 77;
	inode = &inode_info->vfs_inode;

	inode->i_mode = S_IFBLK;
	inode->i_rdev = 0x1010;
	vdfs4_form_fork(&fork, inode);
	KUNIT_EXPECT_EQ(test, fork.size_in_bytes, 0x1010);

	inode->i_mode = S_IFREG;
	inode->i_size = 0x4321;
	VDFS4_I(inode)->fbc->comp_size = 0x1234;

	set_vdfs4_inode_flag(inode, VDFS4_COMPRESSED_FILE);
	vdfs4_form_fork(&fork, inode);
	KUNIT_EXPECT_EQ(test, fork.size_in_bytes, 0x1234);

	clear_vdfs4_inode_flag(inode, VDFS4_COMPRESSED_FILE);
	vdfs4_form_fork(&fork, inode);
	KUNIT_EXPECT_EQ(test, fork.size_in_bytes, 0x4321);

	KUNIT_EXPECT_EQ(test, fork.total_blocks_count, 0x321);

	KUNIT_EXPECT_EQ(test, fork.extents[0].extent.begin, 11);
	KUNIT_EXPECT_EQ(test, fork.extents[0].extent.length, 22);
	KUNIT_EXPECT_EQ(test, fork.extents[0].iblock, 33);

	KUNIT_EXPECT_EQ(test, fork.extents[1].extent.begin, 55);
	KUNIT_EXPECT_EQ(test, fork.extents[1].extent.length, 66);
	KUNIT_EXPECT_EQ(test, fork.extents[1].iblock, 77);
}

int vdfs4_calc_compext_table_crc(void *data, int offset, size_t table_len,
				 u32 *calc_crc);
static unsigned char vdfs4_calc_compext_table_crc_test_data[344] = {
	0x58, 0x54, 0x00, 0x00, 0x40, 0x17, 0x01, 0x00, 0xC0, 0x5E, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x58, 0x54, 0x00, 0x00, 0x40, 0x0C, 0x01, 0x00,
	0x80, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x54, 0x00, 0x00,
	0x80, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x33, 0xC0, 0x04, 0x3E, 0xFD, 0x3C, 0xA9, 0xEC, 0xCC, 0xED, 0x02, 0xC4,
	0x98, 0xDE, 0x09, 0xF1, 0xDD, 0x57, 0xA4, 0xFF, 0x36, 0x15, 0x4E, 0x23,
	0x8D, 0x52, 0x13, 0xBF, 0x9A, 0xEE, 0x3E, 0x94, 0x68, 0xA8, 0xA5, 0x82,
	0xA1, 0x96, 0x30, 0xDA, 0x59, 0x71, 0x85, 0x55, 0x51, 0x24, 0xF7, 0x7D,
	0xA8, 0x95, 0x19, 0x24, 0xEA, 0xD6, 0x29, 0x56, 0x0F, 0xF1, 0xDD, 0x2A,
	0xF8, 0xAC, 0xC4, 0xF3, 0x68, 0xA7, 0x7E, 0x9C, 0xAF, 0x44, 0x56, 0x94,
	0xA6, 0x1C, 0x98, 0xE6, 0x2D, 0x38, 0x9B, 0x74, 0x30, 0xE5, 0x2B, 0x77,
	0x1E, 0x12, 0x5B, 0xEA, 0x2C, 0x75, 0xC3, 0x59, 0x02, 0xBE, 0xB2, 0x96,
	0x1D, 0xE8, 0xFA, 0xF9, 0x9A, 0xF5, 0x4F, 0xC4, 0x7D, 0xB1, 0x65, 0x68,
	0xA3, 0x82, 0x96, 0x67, 0xDB, 0x17, 0xB1, 0x7F, 0x19, 0x31, 0xE1, 0x50,
	0x6A, 0x26, 0x03, 0xF6, 0x63, 0x30, 0x36, 0x23, 0x22, 0x31, 0x96, 0xF4,
	0x42, 0x09, 0x40, 0xF7, 0x92, 0x81, 0xE0, 0x74, 0x18, 0x4D, 0x8A, 0xA2,
	0xEE, 0xDE, 0x41, 0x82, 0x00, 0xF8, 0xBB, 0xE8, 0x4D, 0xA4, 0x6A, 0xB1,
	0x5A, 0x95, 0xF4, 0xE1, 0xF8, 0x79, 0xB4, 0x32, 0xDE, 0x85, 0xC1, 0x29,
	0x77, 0xEB, 0x73, 0x8C, 0x53, 0xAA, 0xE2, 0xA0, 0x78, 0x20, 0x60, 0xA0,
	0xDE, 0x83, 0xBD, 0xE9, 0x70, 0xF3, 0x63, 0x1C, 0x7B, 0x5A, 0x6B, 0x4B,
	0x27, 0x89, 0x5B, 0x85, 0x81, 0x7E, 0xE7, 0x29, 0x8C, 0x07, 0xAB, 0x3B,
	0x68, 0x50, 0xE8, 0x7A, 0xAB, 0x6F, 0x59, 0x0D, 0x43, 0x09, 0x07, 0x29,
	0x1C, 0x59, 0xB5, 0x9A, 0x39, 0x09, 0x3A, 0x24, 0x39, 0x17, 0xD9, 0x1A,
	0x77, 0x6F, 0x32, 0x26, 0x6D, 0x8C, 0x2C, 0x5B, 0x3C, 0xC2, 0xB0, 0x43,
	0x30, 0x0D, 0x4E, 0xD2, 0xFE, 0xF7, 0xB6, 0x21, 0x9C, 0xAC, 0xCF, 0x7D,
	0x21, 0xE8, 0x48, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x68, 0x5A, 0x69, 0x70, 0x03, 0x00, 0x06, 0x00, 0x8C, 0xB4, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1E, 0x1A, 0xB1, 0x8A, 0x11, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void vdfs4_calc_compext_table_crc_test(struct kunit *test)
{
	unsigned char *data = vdfs4_calc_compext_table_crc_test_data;
	size_t table_len = sizeof(vdfs4_calc_compext_table_crc_test_data);
	int ret;
	u32 calc_crc;

	ret = vdfs4_calc_compext_table_crc(data, 0, table_len, &calc_crc);

	KUNIT_EXPECT_EQ(test, 0, ret);
	KUNIT_EXPECT_EQ(test, 0x8ab11a1e, calc_crc);
}

static void vdfs4_calc_compext_table_crc_inval_test(struct kunit *test)
{
	unsigned char *data = vdfs4_calc_compext_table_crc_test_data;
	size_t table_len = sizeof(vdfs4_calc_compext_table_crc_test_data);
	int ret;
	u32 calc_crc;

	ret = vdfs4_calc_compext_table_crc(data, 0, table_len, NULL);
	KUNIT_EXPECT_EQ(test, -EINVAL, ret);

	ret = vdfs4_calc_compext_table_crc(data, 0,
			 sizeof(struct vdfs4_comp_file_descr) - 1, &calc_crc);
	KUNIT_EXPECT_EQ(test, -EINVAL, ret);
}

static struct kunit_case vdfs4_test_cases[] = {
	KUNIT_CASE_PARAM(vdfs4_fill_hlink_value_test,
			 vdfs4_fill_hlink_value_gen_params),
	KUNIT_CASE_PARAM(vdfs4_set_vfs_inode_flags_test,
			 vdfs4_set_vfs_inode_flags_gen_params),
	KUNIT_CASE_PARAM(vdfs4_get_vfs_inode_flags_test,
			 vdfs4_get_vfs_inode_flags_gen_params),
	KUNIT_CASE(vdfs4_form_fork_test),
	KUNIT_CASE(vdfs4_calc_compext_table_crc_test),
	KUNIT_CASE(vdfs4_calc_compext_table_crc_inval_test),
	{}
};

static struct kunit_suite vdfs4_test_suite = {
	.name = "vdfs4 test suite",
	.test_cases = vdfs4_test_cases,
};

kunit_test_suites(&vdfs4_test_suite);

MODULE_LICENSE("GPL v2");

