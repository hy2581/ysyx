#include <common.h>
#include <elf.h>

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))

// 添加 Elf_Ehdr 和 Elf_Phdr 的定义
#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#endif

/* The kernel is monolithic, therefore we do not need to
 * translate the address `buf' from the user process to
 * a physical one, which is necessary for a microkernel.
 */

/* read `len' bytes starting from `offset' of ramdisk into `buf' */
size_t ramdisk_read(void *buf, size_t offset, size_t len) {
  Log("ramdisk_read: RAMDISK_SIZE = %d bytes", RAMDISK_SIZE);
  Log("ramdisk_read: offset = %d, len = %d", offset, len);
  assert(offset + len <= RAMDISK_SIZE);
  memcpy(buf, &ramdisk_start + offset, len);
  return len;
}

/* write `len' bytes starting from `buf' into the `offset' of ramdisk */
size_t ramdisk_write(const void *buf, size_t offset, size_t len) {
  assert(offset + len <= RAMDISK_SIZE);
  Log("ramdisk_write: offset = %zu, len = %zu", offset, len);
  memcpy(&ramdisk_start + offset, buf, len);
  return len;
}

void init_ramdisk() {
  Log("ramdisk info: start = %p, end = %p, size = %d bytes",
      &ramdisk_start, &ramdisk_end, RAMDISK_SIZE);
}

size_t get_ramdisk_size() {
  return RAMDISK_SIZE;
}
