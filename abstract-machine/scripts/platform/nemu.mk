AM_SRCS := platform/nemu/trm.c \
           platform/nemu/ioe/ioe.c \
           platform/nemu/ioe/timer.c \
           platform/nemu/ioe/input.c \
           platform/nemu/ioe/gpu.c \
           platform/nemu/ioe/audio.c \
           platform/nemu/ioe/disk.c \
           platform/nemu/mpe.c

CFLAGS    += -fdata-sections -ffunction-sections
CFLAGS    += -I$(AM_HOME)/am/src/platform/nemu/include
LDSCRIPTS += $(AM_HOME)/scripts/linker.ld
LDFLAGS   += --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0
LDFLAGS   += --gc-sections -e _start
NEMUFLAGS += -l $(shell dirname $(IMAGE).elf)/nemu-log.txt

MAINARGS_MAX_LEN = 64
MAINARGS_PLACEHOLDER = The insert-arg rule in Makefile will insert mainargs here.
CFLAGS += -DMAINARGS_MAX_LEN=$(MAINARGS_MAX_LEN) -DMAINARGS_PLACEHOLDER=\""$(MAINARGS_PLACEHOLDER)"\"

insert-arg: image
	@python $(AM_HOME)/tools/insert-arg.py $(IMAGE).bin $(MAINARGS_MAX_LEN) "$(MAINARGS_PLACEHOLDER)" "$(mainargs)"
# 执行 Python 脚本，将命令行参数插入到二进制镜像中：
# $(IMAGE).bin - 生成的二进制镜像文件路径
# $(MAINARGS_MAX_LEN) - 命令行参数的最大长度（64字节）
# "$(MAINARGS_PLACEHOLDER)" - 要查找的占位符文本
# "$(mainargs)" - 用户通过 make 命令提供的实际命令行参数

image: image-dep
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin
    
# 使用 objdump 工具反汇编 ELF 文件，生成可读的汇编代码文本文件。
# 输出提示信息，表明正在创建二进制镜像文件。
# 使用 objcopy 工具将 ELF 文件转换为纯二进制格式：
# -S - 删除所有符号和重定位信息
# --set-section-flags .bss=alloc,contents - 设置 BSS 段属性
# -O binary - 指定输出格式为纯二进制
# $(IMAGE).elf - 输入 ELF 文件
# $(IMAGE).bin - 输出二进制文件

run: insert-arg
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin

# 在 NEMU 目录中执行 make 命令，以运行模拟器：
# -C $(NEMU_HOME) - 切换到 NEMU 目录
# ISA=$(ISA) - 传递指令集架构（如 riscv32）
# run - 指定 NEMU 目标为运行模式
# ARGS="$(NEMUFLAGS)" - 传递额外参数，包括日志文件路径
# IMG=$(IMAGE).bin - 指定要加载的程序镜像

gdb: insert-arg
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) gdb ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin

.PHONY: insert-arg
