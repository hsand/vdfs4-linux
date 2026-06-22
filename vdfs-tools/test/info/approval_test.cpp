#include <stdio.h>
#include <gtest/gtest.h>
#include <ApprovalTests.hpp>

#include "info.h"

auto directoryDisposer = ApprovalTests::Approvals::useApprovalsSubdirectory("ApprovedData");

TEST(INFO, ApprovalTest_VolumeInfo)
{
	int ret;
	char buffer[4096] = {0,};
	const char *params[4] = {
		"info", "-v", "test/info/SampleData/systemrw.img", NULL
	};

	setbuffer(stdout, buffer, sizeof(buffer));
	ret = info(3, (char **)params);
	setbuffer(stdout, NULL, 0);

	EXPECT_EQ(0, ret);
	ApprovalTests::Approvals::verify(buffer);
}

TEST(INFO, ApprovalTest_DirectoryIterate)
{
	int ret;
	char buffer[4096] = {0,};
	const char *params[3] = {
		"info", "test/info/SampleData/testdir", NULL
	};

	setbuffer(stdout, buffer, sizeof(buffer));
	ret = info(2, (char **)params);
	setbuffer(stdout, NULL, 0);

	EXPECT_EQ(0, ret);
	ApprovalTests::Approvals::verify(buffer);
}

