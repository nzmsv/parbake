#ifndef _PARBAKE_H_
#define _PARBAKE_H_

#define PARBAKE_TEST_TIMEOUT 5

#define PARBAKE_TEST_NAME_LEN 64
#define PARBAKE_HOST_NAME_LEN 256
#define PARBAKE_PREFIX_LEN 1024


#include <stdio.h>
#include <unistd.h>

typedef char *parbake_hostname;
typedef void (*parbake_test_func)();
typedef struct parbake_test_group {
	parbake_test_func test;
	parbake_hostname *hosts;
} parbake_test_group;

#define HOSTS(...) ((parbake_hostname[]){ __VA_ARGS__ , 0})

#define PARBAKE_TEST_ANNOUNCE(_name) do { printf(_name "\n"); fflush(stdout); } while (0)
#define PARBAKE_TEST_EXIT(_code) do { fclose(stdout); fclose(stderr); _exit(_code); } while (0)

#define ASSERT(_x) \
	do { \
		if (!(_x)) { \
			printf("Assertion failed: " #_x "\n"); \
			PARBAKE_TEST_EXIT(1); \
		} \
	} while (0)

#define BEGIN_TEST(_name) \
	void _name () { \
		PARBAKE_TEST_ANNOUNCE(#_name);

#define END_TEST }


void parbake_run_group(const char * const name, parbake_test_group group[]);
#define BEGIN_GROUP(_name) \
	void _name () { \
		parbake_run_group(#_name, (parbake_test_group[]){

#define END_GROUP \
		, {0, 0} }); \
	}

void parbake_run_suite(const char * const name, parbake_test_func tests[]);
#define BEGIN_SUITE(_name) \
	void _name () { \
		parbake_run_suite(#_name, (parbake_test_func[]){

#define END_SUITE \
		, 0 }); \
	}

void parbake_launch(int argc, char **argv, const char * const name, parbake_hostname *hosts, parbake_test_func tests[]);
#define BEGIN_RUN(_name, _format, _hosts) \
	void _name (int argc, char **argv) { \
		parbake_launch(argc, argv, #_name, _hosts, (parbake_test_func[]){ \

#define END_RUN \
		, 0 }); \
	}

#define RUN(_name, _argc, _argv) \
	do { _name(_argc, _argv); } while (0)

#endif
