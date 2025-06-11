#include <elf.h>
#include <proc.h>

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  // 忽略pcb和filename参数（在实现文件系统前）

  // 读取ELF文件头
  Elf_Ehdr elf_header;
  ramdisk_read(&elf_header, 0, sizeof(Elf_Ehdr));

  // 检查ELF魔数是否正确
  if (*(uint32_t *)elf_header.e_ident != 0x464c457f) { // 0x7f E L F
    panic("Invalid ELF file");
  }

  // 获取程序头表的偏移和数量
  uint32_t phoff = elf_header.e_phoff;
  uint16_t phnum = elf_header.e_phnum;
  uint16_t phsize = elf_header.e_phentsize;

  // 读取并处理每个程序头
  Elf_Phdr ph;
  for (int i = 0; i < phnum; i++) {
    // 读取程序头
    ramdisk_read(&ph, phoff + i * phsize, phsize);

    // 如果是可加载段（LOAD类型）
    if (ph.p_type == PT_LOAD) {
      // 读取段内容到内存
      ramdisk_read((void *)ph.p_vaddr, ph.p_offset, ph.p_filesz);

      // 如果内存大小大于文件大小，需要将多余部分清零（例如.bss段）
      if (ph.p_memsz > ph.p_filesz) {
        memset((void *)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
      }
    }
  }

  // 返回程序入口点地址
  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
  Log("执行完成");
}

