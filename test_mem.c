// test program that allocates some memory in multiple threads

#include <stdlib.h>
#include <windows.h>

DWORD WINAPI f(LPVOID p) {
	int m = *(int *)p;
	char *a = malloc(m);
	int x = 0;
	for (int i = 0; i < m; ++i) {
		a[i] = i & 0xff;
		x = x ^ a[i];
	}
	// do not free!
	return x;
}

int main(int argc, char *argv[]) {
	int n = atoi(argv[1]); // how many threads to create
	int m = atoi(argv[2]); // how much memory to allocate in each

	HANDLE *t = malloc(n * sizeof(HANDLE));
	for (int i = 0; i < n; ++i) {
		t[i] = CreateThread(0, 0, f, &m, 0, 0);
	}
	for (int i = 0; i < n; ++i) {
		WaitForSingleObject(t[i], INFINITE);
	}

	return 0;
}
