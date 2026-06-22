#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <gtest/gtest.h>
#include <errno.h>

#include "info.h"
#include "volume_info.h"
#include "file_info.h"

TEST(INFO, UnitTest_HelpOption)
{
	int ret;
	const char *params[3] = {"info", "-h", NULL};

	ret = info(2, (char **)params);
	EXPECT_EQ(0, ret);
}

TEST(INFO, UnitTest_Volume_IsVdfs4Volume)
{
	bool ret;

	ret = is_vdfs4_volume("test/info/SampleData/systemrw.img");
	EXPECT_EQ(true, ret);

	ret = is_vdfs4_volume("test/info/SampleData/testdir.sqsh");
	EXPECT_EQ(false, ret);
}

TEST(INFO, UnitTest_File_GetFileAttr)
{
	int ret;

	ret = get_file_attributes("nopath/nofile", NULL);
	EXPECT_EQ(-EINVAL, ret);
}

TEST(INFO, UnitTest_File_IsSkipDirName)
{
	bool ret;

	ret = is_skip_dir_name((char *)".");
	EXPECT_EQ(true, ret);
	ret = is_skip_dir_name((char *)"..");
	EXPECT_EQ(true, ret);
	ret = is_skip_dir_name((char *)"...");
	EXPECT_EQ(false, ret);
	ret = is_skip_dir_name((char *)"abc");
	EXPECT_EQ(false, ret);
	ret = is_skip_dir_name((char *)"");
	EXPECT_EQ(false, ret);
}

