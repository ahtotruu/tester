// test program that wastes some CPU time in busy loops in multiple threads

#include <stdlib.h>
#include <windows.h>

DWORD WINAPI f(LPVOID p) {
	int m = *(int *)p;
	int x = 0;
	for (int i = 0; i < m; ++i) {
		for (int j = 0; j < m; ++j) {
			x = x ^ i ^ j;
		}
	}
	return x;
}

int main(int argc, char *argv[]) {
	int n = atoi(argv[1]); // how many threads to create
	int m = atoi(argv[2]); // how much time to waste in each

	HANDLE *t = malloc(n * sizeof(HANDLE));
	for (int i = 0; i < n; ++i) {
		t[i] = CreateThread(0, 0, f, &m, 0, 0);
	}
	for (int i = 0; i < n; ++i) {
		WaitForSingleObject(t[i], INFINITE);
	}

	return 0;
}
