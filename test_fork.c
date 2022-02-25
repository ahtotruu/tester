// test program that tries to fork

#include <stdlib.h>
#include <windows.h>

int main(int argc, char *argv[]) {
	if (argc > 1) {
		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi;
		CreateProcess(0, argv[0], 0, 0, FALSE, 0, 0, 0, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}
	return 0;
}
