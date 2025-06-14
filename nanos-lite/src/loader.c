#include <elf.h>
#include <fs.h>
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
  // 打开ELF可执行文件
  // 注意：缺少了对打开文件失败的检查，应该添加if (fd < 0)判断
  int fd = fs_open(filename, 0, 0);

  // 读取ELF文件头
  // 注意：没有使用assert确认读取的字节数是否正确
  Elf_Ehdr elf_header;
  fs_read(fd, &elf_header, sizeof(Elf_Ehdr));

  // 检查ELF魔数是否正确
  // 功能与第二个程序相同，但使用了if而不是assert
  if (*(uint32_t *)elf_header.e_ident != 0x464c457f) { // 0x7f E L F
    panic("Invalid ELF file");
  }

  // 获取程序头表信息
  uint32_t phoff = elf_header.e_phoff;      // 程序头表偏移
  uint16_t phnum = elf_header.e_phnum;      // 程序头项数量
  uint16_t phsize = elf_header.e_phentsize; // 每个程序头项大小

  // 定位到程序头表起始位置
  fs_lseek(fd, phoff, SEEK_SET);

  // 读取并处理每个程序头
  Elf_Phdr ph;
  for (int i = 0; i < phnum; i++) {
    // 读取程序头
    // 注意：没有使用assert确认读取的字节数是否正确
    fs_read(fd, &ph, phsize);

    // 如果是可加载段（LOAD类型）
    if (ph.p_type == PT_LOAD) {
      // 定位到段内容
      fs_lseek(fd, ph.p_offset, SEEK_SET);

      // 读取段内容到内存
      // 注意：直接读取到目标地址，没有使用临时缓冲区，这是不安全的做法
      // 应该像第二个程序那样先malloc一个临时缓冲区，再用memcpy复制
      fs_read(fd, (void *)ph.p_vaddr, ph.p_filesz);

      // 如果内存大小大于文件大小，需要将多余部分清零（例如.bss段）
      // 这部分与第二个程序实现相同
      if (ph.p_memsz > ph.p_filesz) {
        memset((void *)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
      }
    }

    // 如果不是连续读取程序头，需要重新定位
    // 注意：第二个程序直接在for循环开始时就计算了准确位置，不需要这一步
    if (i < phnum - 1) {
      fs_lseek(fd, phoff + (i + 1) * phsize, SEEK_SET);
    }
  }

  // 关闭文件
  // 注意：没有检查关闭是否成功，应该使用assert(fs_close(fd) == 0)
  fs_close(fd);

  // 返回程序入口点地址
  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
  Log("执行完成");
}

