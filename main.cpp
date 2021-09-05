#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vector>

typedef struct _TAILS_MEMORY {
	void* bufferAddr;
	UINT_PTR address;
	ULONGLONG size;
	ULONG pID;
	BOOLEAN read;
	BOOLEAN write;
	BOOLEAN reqBase;
	BOOLEAN draw_box;
	int r, g, b, x, y, w, h, t;
	void* output;
	const char* module_name;
	ULONG64 base_address;

}TAILS_MEMORY;

uintptr_t base_address = 0;
std::uint32_t process_id = 0;
HDC hdc;

template<typename ...Arg>
uint64_t call_hook(const Arg ... args) {
	void* hooked_function = GetProcAddress(LoadLibrary("win32u.dll"), "NtDxgkGetTrackedWorkloadStatistics");
	auto function = static_cast<uint64_t(__stdcall*)(Arg...)>(hooked_function);

	return function(args ...);
}

struct HandleDisposer {
	
	using pointer = HANDLE;
	void operator()(HANDLE handle) const {
		
		if (handle != NULL || handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
		}
	}
};

using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

std::uint32_t get_process_id(std::string_view process_name) {
	PROCESSENTRY32 procEntry;
	const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL));

	if (snapshot_handle.get() == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	procEntry.dwSize = sizeof(MODULEENTRY32);

	while (Process32Next(snapshot_handle.get(), &procEntry) == TRUE) {
		if (process_name.compare(procEntry.szExeFile) == NULL) {
			return procEntry.th32ProcessID;
		}
	}
	return NULL;
}

static ULONG64 get_module_base_address(const char* module_name) {
	TAILS_MEMORY instructions = { 0 };
	instructions.pID = process_id;
	instructions.reqBase = TRUE;
	instructions.read = FALSE;
	instructions.write = FALSE;
	instructions.draw_box = FALSE;
	instructions.module_name = module_name;
	call_hook(&instructions);

	ULONG64 base = NULL;
	base = instructions.base_address;
	return base;
}

template <class T>
T Read(UINT_PTR read_address) {
	T response{};
	TAILS_MEMORY instructions;
	instructions.pID = process_id;
	instructions.size = sizeof(T);
	instructions.address = read_address;
	instructions.read = TRUE;
	instructions.write = FALSE;
	instructions.reqBase = FALSE;
	instructions.draw_box = FALSE;
	instructions.output = &response;
	call_hook(&instructions);

	return response;
}

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size) {
	TAILS_MEMORY instructions;
	instructions.address = write_address;
	instructions.pID = process_id;
	instructions.read = FALSE;
	instructions.write = TRUE;
	instructions.reqBase = FALSE;
	instructions.draw_box = FALSE;
	instructions.bufferAddr = (void*)source_address;
	instructions.size = write_size;

	call_hook(&instructions);

	return true;
}

bool draw_box(int x, int y, int w, int h, int t, int r, int g, int b) {
	TAILS_MEMORY instructions;
	instructions.read = FALSE;
	instructions.write = FALSE;
	instructions.reqBase = FALSE;
	instructions.draw_box = TRUE;

	instructions.x = x;
	instructions.y = y;
	instructions.w = w;
	instructions.h = h;
	instructions.t = t;

	instructions.r = r;
	instructions.g = g;
	instructions.b = b;

	call_hook(&instructions);

	return true;
}

template<typename S>
bool write(UINT_PTR write_address, const S& value) {
	return write_memory(write_address, (UINT_PTR)&value, sizeof(S));
}

int main() {

	while (true) {
		draw_box(50, 50, 50, 50, 2, 0, 255, 0);
	}
}