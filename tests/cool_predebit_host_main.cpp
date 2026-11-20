// SPDX-License-Identifier: MIT
#include <cstddef>

extern "C" __declspec(dllimport) int __cdecl run_predebit_host(
    int argc, char** argv, unsigned char* storage, std::size_t size);

int main(int argc, char** argv) {
  // Our own BSS region, initially writable. The loader reserves it ahead of
  // the heap and DLLs for the verified call pages at 0x660000 through 0x79ffff.
  alignas(4096) static unsigned char storage[0x300000];
  return run_predebit_host(argc, argv, storage, sizeof(storage));
}
