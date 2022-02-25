#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <windows.h>

// Helper to print system error messages
void err(const char *msg, DWORD res) {
	static char buf[8192];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, res, 0, buf, sizeof(buf), 0);
	fprintf(stderr, "%s failed: %lu\n%s\n", msg, res, buf);
}

// Exit codes
const int EXIT_OK = 0;
const int EXIT_FAIL = 1; // system call failed
const int EXIT_FORK = 2; // child process forked
const int EXIT_TIME = 3; // time limit exceeded
const int EXIT_MEMORY = 4; // memory limit exceeded

int main(int argc, char *argv[]) {
	int tlim = 0; // CPU time limit
	int mlim = 0; // memory limit

	int i = 1, n;
	while (i < argc) {
		if (strcmp(argv[i], "-?") == 0) {
			i = argc;
		} else if (strcmp(argv[i], "-t") == 0) {
			if (i + 1 >= argc)
				i = argc;
			else if (sscanf(argv[i + 1], "%d%n", &tlim, &n) != 1)
				i = argc;
			else if (strlen(argv[i + 1]) != (unsigned) n)
				i = argc;
			else
				i += 2;
		} else if (strcmp(argv[i], "-m") == 0) {
			if (i + 1 >= argc)
				i = argc;
			else if (sscanf(argv[i + 1], "%d%n", &mlim, &n) != 1)
				i = argc;
			else if (strlen(argv[i + 1]) != (unsigned) n)
				i = argc;
			else
				i += 2;
		} else
			break;
	}
	if (i + 1 != argc) {
		fprintf(stderr, "Usage: %s [-t tlim] [-m mlim] command\n", argv[0]);
		fprintf(stderr, "\ttlim - CPU time limit in ms, default is no limit\n");
		fprintf(stderr, "\tmlim - memory limit in MB, default is no limit\n");
		fprintf(stderr, "\tmulti-word command must be enclosed in double quotes\n");
		fprintf(stderr, "Example: %s -t 1000 -m 3 \"java -cp . Hello\"\n", argv[0]);
		return EXIT_FAIL;
	}

	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi;
	DWORD res = CreateProcess(0, argv[i], 0, 0, FALSE, CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM, 0, 0, &si, &pi);
	if (!res) {
		err("CreateProcess", GetLastError());
		return EXIT_FAIL;
	}

	HANDLE job = CreateJobObject(0, 0);
	if (!job) {
		err("CreateJobObject", GetLastError());
		return EXIT_FAIL;
	}

	res = AssignProcessToJobObject(job, pi.hProcess);
	if (!res) {
		err("AssignProcessToJobObject", GetLastError());
		return EXIT_FAIL;
	}

	res = ResumeThread(pi.hThread);
	if (res == (DWORD) -1) {
		err("ResumeThread", GetLastError());
		return EXIT_FAIL;
	}

	if (tlim > 0)
		res = WaitForSingleObject(pi.hProcess, 2 * tlim);
	else
		res = WaitForSingleObject(pi.hProcess, INFINITE);
	if (res == WAIT_FAILED) {
		err("WaitForSingleObject", GetLastError());
		return EXIT_FAIL;
	}
	if (res == WAIT_TIMEOUT) {
		fprintf(stderr, "Wall time limit exceeded!\n");
		fprintf(stderr, "Attempting to kill... %s\n", (TerminateProcess(pi.hProcess, 0) ? "OK" : "FAILED"));
		return EXIT_TIME;
	}

	DWORD code;
	res = GetExitCodeProcess(pi.hProcess, &code);
	if (!res) {
		err("GetExitCodeProcess", GetLastError());
		return EXIT_FAIL;
	}
	if (code != 0) {
		fprintf(stderr, "Process exit code: %lu\n", code);
		return EXIT_FAIL;
	}

	JOBOBJECT_BASIC_ACCOUNTING_INFORMATION bai;
	res = QueryInformationJobObject(job, JobObjectBasicAccountingInformation, &bai, sizeof(bai), 0);
	if (!res) {
		err("QueryInformationJobObject(basic)", GetLastError());
		return EXIT_FAIL;
	}
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION eli;
	res = QueryInformationJobObject(job, JobObjectExtendedLimitInformation, &eli, sizeof(eli), 0);
	if (!res) {
		err("QueryInformationJobObject(extended)", GetLastError());
		return EXIT_FAIL;
	}

	if (bai.TotalProcesses > 1) {
		fprintf(stderr, "Process forked!\n");
		return EXIT_FORK;
	}

	int t = (bai.TotalUserTime.QuadPart + bai.TotalKernelTime.QuadPart) / 10000; // to milliseconds
	if (tlim == 0)
		fprintf(stderr, "CPU time used: %d ms\n", t);
	if (tlim > 0)
		fprintf(stderr, "CPU time used: %d / %d ms\n", t, tlim);
	int m = eli.PeakJobMemoryUsed / 1024 / 1024; // to megabytes
	if (mlim == 0)
		fprintf(stderr, "Memory used: %d MB\n", m);
	if (mlim > 0)
		fprintf(stderr, "Memory used: %d / %d MB\n", m, mlim);

	if (tlim > 0 && t > tlim) {
		fprintf(stderr, "CPU time limit exceeded!\n");
		return EXIT_TIME;
	}
	if (mlim > 0 && m > mlim) {
		fprintf(stderr, "Memory limit exceeded!\n");
		return EXIT_MEMORY;
	}

	return EXIT_OK;
}
