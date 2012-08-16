#include "parbake_defs.h"
#include "emalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>


static void run_forked(parbake_test_func tests[])
{
	parbake_test_func *test;
	size_t n;
	for (test = tests, n = 0; *test; ++test, ++n);
	pid_t pids[n];
	pid_t *ppid;
	FILE *files[n];
	FILE **pfile;

	for (test = tests, ppid = pids, pfile = files; *test; ++test, ++ppid, ++pfile) {
		//Each tests writes its output to a temp file.
		//Use a file instead of a pipe to avoid hitting the pipe limit.
		*pfile = tmpfile();
		*ppid = fork();
		if (!*ppid) {
			if (dup2(fileno(*pfile), STDOUT_FILENO) < 0)
				_exit(errno);
			setvbuf (stdout, 0, _IONBF, 0);
			alarm(PARBAKE_TEST_TIMEOUT);
			//run a single test
			(*test)();
			PARBAKE_TEST_EXIT(0);
		}
	}

	//Test results are printed in a deterministic order.
	for (test = tests, ppid = pids, pfile = files; *test; ++test, ++ppid, ++pfile) {
		int status;
		waitpid(*ppid, &status, 0);
		rewind(*pfile);

		parbake_line_reader reader = parbake_line_reader_create(fileno(*pfile));

		//The first line of the output is the test name.
		char testname[PARBAKE_TEST_NAME_LEN+2] = "";
		fgets(testname, PARBAKE_TEST_NAME_LEN+1, *pfile);
		//fgets can read more than necessary, reset pos
		lseek(fileno(*pfile), strlen(testname), SEEK_SET);
		size_t testname_len = strcspn(testname, "\n");
		testname[testname_len] = '\0';
		//report test name in Subunit format
		printf("test: %s\n", testname);

		while (parbake_line_reader_read(reader) != 0);
		parbake_line_reader_destroy(reader);
		fclose(*pfile);

		//report exit status in Subunit format
		if (WIFEXITED(status)) {
			status = WEXITSTATUS(status);
			if (!status)
				printf("success: %s\n", testname);
			else
				printf("failure: %s [exit %d]\n", testname, status);
		}
		else if (WIFSIGNALED(status)) {
			status = WTERMSIG(status);
			printf("error: %s [signal %d %s]\n", testname, status, strsignal(status));
		}
		else {
			printf("error: %s\n", testname);
		}
	}
}

void parbake_run_suite(const char * const name, parbake_test_func tests[])
{
	//run tests sequentially
	printf("%s\n", name);
	parbake_test_func *test;
	for (test = tests; *test; ++test)
		run_forked((parbake_test_func[]){*test, 0});
}

void parbake_run_group(const char * const name, parbake_test_group group[])
{
	printf("%s\n", name);
	parbake_test_group *p;
	size_t n;
	for (n = 0, p = group; p->test; ++p) {
		parbake_hostname *h;
		for (h = p->hosts; *h; ++h, ++n);
	}

	char *my_name = getenv("PARBAKE_HOST_NAME");
	parbake_test_func my_tests[n];
	size_t count = 0;

	//get a list of tests from the group that match our hostname
	for (p = group; p->test; ++p) {
		parbake_hostname *h;
		for (h = p->hosts; *h; ++h) {
			if (!strncmp(my_name, *h, PARBAKE_HOST_NAME_LEN)) {
				my_tests[count] = p->test;
				++count;
			}
		}
	}
	my_tests[count] = 0;

	//run tests in parallel
	run_forked(my_tests);
}

static void run_launcher(const char * const exec, const char *name, parbake_hostname *hosts)
{
	char * const abspath = realpath(exec, 0);
	if (!abspath) {
		perror(PFX "Could not find absolute path\n");
		exit(1);
	}

	size_t n;
	parbake_hostname *h;
	for (h = hosts, n = 0; *h; ++h, ++n);

	int fds[n];
	size_t i;
	for (i = 0; i < n; ++i) {
		const size_t cmdlen = 32 + PARBAKE_HOST_NAME_LEN + PARBAKE_TEST_NAME_LEN + strlen(abspath);
		char cmd[cmdlen];
		snprintf(cmd, cmdlen, "ssh -n -T -x %s "
		         "'/bin/sh -c \""
		         "PARBAKE_RUN_NAME=\\\"%s\\\" "
		         "PARBAKE_HOST_NAME=\\\"%s\\\" "
		         "%s\"'",
		         hosts[i], name, hosts[i], abspath);
		FILE *f = popen(cmd, "r");
		if (!f) {
			perror(PFX "could not execute ssh\n");
			exit(1);
		}
		fds[i] = fileno(f);
	}
	free(abspath);

	parbake_poller poller = parbake_poller_create(fds, n);
	parbake_poller_poll(poller);
	parbake_poller_destroy(poller);
}


void parbake_launch(int argc, char **argv, const char * const name, parbake_hostname *hosts, parbake_test_func tests[])
{
	const char *testname = getenv("PARBAKE_RUN_NAME");

	if (!testname) {
		// launcher
		run_launcher(argv[0], name, hosts);
	}
	else if (!strcmp(name, testname)) {
		//worker
		fclose(stdin);
		printf("test: ");
		parbake_run_suite(name, tests);
		printf("success: %s\n", name);
		exit(0);
	}
}
