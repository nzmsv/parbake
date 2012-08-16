#include <stdio.h>
#include "parbake.h"


BEGIN_TEST(test_pass)
{
	int i = 1;
	printf("this test will pass\n");
	ASSERT(i == 1);
}
END_TEST

BEGIN_TEST(test_fail)
{
	int i = 1;
	printf("this test will fail due to a false assert\n");
	ASSERT(i == 0);
}
END_TEST

BEGIN_TEST(test_crash)
{
	printf("about to crash, this will log an error\n");
	int *crash = 0;
	*crash = 1;
}
END_TEST

BEGIN_GROUP(group_a)
	{ test_pass, HOSTS("localhost") },
	{ test_crash, HOSTS("localhost") }
END_GROUP


BEGIN_SUITE(suite_a)
    group_a
END_SUITE

BEGIN_RUN(run_a, default_format, HOSTS("localhost"))
    suite_a, test_fail
END_RUN


int main(int argc, char **argv)
{
	RUN(run_a, argc, argv);
	return 0;
}
