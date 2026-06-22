#include <stdio.h>
#include <gtest/gtest.h>
#include <ApprovalTests.hpp>

auto directoryDisposer = ApprovalTests::Approvals::useApprovalsSubdirectory("ApprovedData");

#ifdef __cplusplus
extern "C" {
int fsck(int argc, char **argv);
}
#endif

TEST(FSCK, ApprovalTest_VolumeChecking)
{
	int ret;
	char buffer[4096] = {0,};
	const char *params[4] = {
		"fsck", "-v", "test/fsck/SampleData/sample.img", NULL
	};

	setbuffer(stdout, buffer, sizeof(buffer));
	ret = fsck(3, (char **)params);
	setbuffer(stdout, NULL, 0);

	EXPECT_EQ(0, ret);
	ApprovalTests::Approvals::verify(buffer);
}

TEST(FSCK, ApprovalTest_CategoryNodeDump)
{
	int ret;
	char buffer[4096] = {0,};
	const char *params[5] = {
		"fsck", "-d", "3", "test/fsck/SampleData/sample.img", NULL
	};

	setbuffer(stdout, buffer, sizeof(buffer));
	ret = fsck(4, (char **)params);
	setbuffer(stdout, NULL, 0);

	EXPECT_EQ(0, ret);
	ApprovalTests::Approvals::verify(buffer);
}

TEST(FSCK, ApprovalTest_ExtentNodeDump)
{
	int ret;
	char buffer[4096] = {0,};
	const char *params[5] = {
		"fsck", "-e", "1", "test/fsck/SampleData/sample.img", NULL
	};

	setbuffer(stdout, buffer, sizeof(buffer));
	ret = fsck(4, (char **)params);
	setbuffer(stdout, NULL, 0);

	EXPECT_EQ(0, ret);
	ApprovalTests::Approvals::verify(buffer);
}
