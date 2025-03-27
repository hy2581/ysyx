# 用于AbstractMachine内核和库的Makefile

### *通过`make html`获取此Makefile的更可读版本* (需要python-markdown)
html:
	cat Makefile | sed 's/^\([^#]\)/    \1/g' | markdown_py > Makefile.html
.PHONY: html

## 1. 基本设置和检查

### 默认创建裸机内核镜像
# ifeq ($(MAKECMDGOALS),) 检查用户是否没有指定目标
# 即判断是否只输入了make命令而没有额外参数 
# 确保当用户只输入make时，会自动执行"image"目标
ifeq ($(MAKECMDGOALS),)
  MAKECMDGOALS  = image
  .DEFAULT_GOAL = image
endif

### 当运行`make clean/clean-all/html`时跳过检查
# 检查第一个参数是否出现在第二个参数中
ifeq ($(findstring $(MAKECMDGOALS),clean|clean-all|html),)

### 打印构建信息消息
# 输出如：# Building am-kernels-image [x86_64-qemu]
$(info # Building $(NAME)-$(MAKECMDGOALS) [$(ARCH)])

### 检查：环境变量`$AM_HOME`看起来合理
# $(wildcard 模式) - 这个Make函数用于检查文件是否存在
ifeq ($(wildcard $(AM_HOME)/am/include/am.h),)
  $(error $$AM_HOME must be an AbstractMachine repo)
endif

### 检查：环境变量`$ARCH`必须在支持的列表中
ARCHS = $(basename $(notdir $(shell ls $(AM_HOME)/scripts/*.mk)))
ifeq ($(filter $(ARCHS), $(ARCH)), )
  $(error Expected $$ARCH in {$(ARCHS)}, Got "$(ARCH)")
endif

### 从`$ARCH`中提取指令集架构(`ISA`)和平台。例如：`ARCH=x86_64-qemu -> ISA=x86_64; PLATFORM=qemu`
ARCH_SPLIT = $(subst -, ,$(ARCH))
ISA        = $(word 1,$(ARCH_SPLIT))
PLATFORM   = $(word 2,$(ARCH_SPLIT))

### 检查是否有内容需要构建
ifeq ($(flavor SRCS), undefined)
  $(error Nothing to build)
endif

### 检查到此结束
endif

## 2. 通用编译目标

### 创建目标目录 (`build/$ARCH`)
WORK_DIR  = $(shell pwd)
DST_DIR   = $(WORK_DIR)/build/$(ARCH)
$(shell mkdir -p $(DST_DIR))

### 编译目标（二进制镜像或归档文件）
IMAGE_REL = build/$(NAME)-$(ARCH)
IMAGE     = $(abspath $(IMAGE_REL))
ARCHIVE   = $(WORK_DIR)/build/$(NAME)-$(ARCH).a

### 收集要链接的文件：目标文件（`.o`）和库文件（`.a`）
OBJS      = $(addprefix $(DST_DIR)/, $(addsuffix .o, $(basename $(SRCS))))
LIBS     := $(sort $(LIBS) am klib) # lazy evaluation ("=") causes infinite recursions
LINKAGE   = $(OBJS) \
  $(addsuffix -$(ARCH).a, $(join \
    $(addsuffix /build/, $(addprefix $(AM_HOME)/, $(LIBS))), \
    $(LIBS) ))

## 3. 通用编译标志

### （交叉）编译器，例如，mips-linux-gnu-g++
AS        = $(CROSS_COMPILE)gcc
CC        = $(CROSS_COMPILE)gcc
CXX       = $(CROSS_COMPILE)g++
LD        = $(CROSS_COMPILE)ld
AR        = $(CROSS_COMPILE)ar
OBJDUMP   = $(CROSS_COMPILE)objdump
OBJCOPY   = $(CROSS_COMPILE)objcopy
READELF   = $(CROSS_COMPILE)readelf

### 编译标志
INC_PATH += $(WORK_DIR)/include $(addsuffix /include/, $(addprefix $(AM_HOME)/, $(LIBS)))
INCFLAGS += $(addprefix -I, $(INC_PATH))

ARCH_H := arch/$(ARCH).h
CFLAGS   += -O2 -MMD -Wall -Werror $(INCFLAGS) \
            -D__ISA__=\"$(ISA)\" -D__ISA_$(shell echo $(ISA) | tr a-z A-Z)__ \
            -D__ARCH__=$(ARCH) -D__ARCH_$(shell echo $(ARCH) | tr a-z A-Z | tr - _) \
            -D__PLATFORM__=$(PLATFORM) -D__PLATFORM_$(shell echo $(PLATFORM) | tr a-z A-Z | tr - _) \
            -DARCH_H=\"$(ARCH_H)\" \
            -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector \
            -Wno-main -U_FORTIFY_SOURCE -fvisibility=hidden
CXXFLAGS +=  $(CFLAGS) -ffreestanding -fno-rtti -fno-exceptions
ASFLAGS  += -MMD $(INCFLAGS)
LDFLAGS  += -z noexecstack $(addprefix -T, $(LDSCRIPTS))

## 4. 架构特定配置

### 粘贴架构特定配置（例如，来自`scripts/x86_64-qemu.mk`）
-include $(AM_HOME)/scripts/$(ARCH).mk

## 5. 编译规则

### 规则（编译）：单个`.c` -> `.o`（gcc）
$(DST_DIR)/%.o: %.c
	@mkdir -p $(dir $@) && echo + CC $<
	@$(CC) -std=gnu11 $(CFLAGS) -c -o $@ $(realpath $<)

### 规则（编译）：单个`.cc` -> `.o`（g++）
$(DST_DIR)/%.o: %.cc
	@mkdir -p $(dir $@) && echo + CXX $<
	@$(CXX) -std=c++17 $(CXXFLAGS) -c -o $@ $(realpath $<)

### 规则（编译）：单个`.cpp` -> `.o`（g++）
$(DST_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@) && echo + CXX $<
	@$(CXX) -std=c++17 $(CXXFLAGS) -c -o $@ $(realpath $<)

### 规则（编译）：单个`.S` -> `.o`（gcc，预处理并调用汇编器）
$(DST_DIR)/%.o: %.S
	@mkdir -p $(dir $@) && echo + AS $<
	@$(AS) $(ASFLAGS) -c -o $@ $(realpath $<)

### 规则（递归make）：构建依赖库（am, klib, ...）
$(LIBS): %:
	@$(MAKE) -s -C $(AM_HOME)/$* archive

### 规则（链接）：目标文件（`*.o`）和库（`*.a`）-> `IMAGE.elf`，最终打包成镜像的ELF二进制文件（ld）
$(IMAGE).elf: $(LINKAGE) $(LDSCRIPTS)
	@echo \# Creating image [$(ARCH)]
	@echo + LD "->" $(IMAGE_REL).elf
ifneq ($(filter $(ARCH),native),)
	@$(CXX) -o $@ -Wl,--whole-archive $(LINKAGE) -Wl,-no-whole-archive $(LDFLAGS_CXX)
else
	@$(LD) $(LDFLAGS) -o $@ --start-group $(LINKAGE) --end-group
endif

### 规则（归档）：目标文件（`*.o`）-> `ARCHIVE.a`（ar）
$(ARCHIVE): $(OBJS)
	@echo + AR "->" $(shell realpath $@ --relative-to .)
	@$(AR) rcs $@ $^

### 规则（`#include`依赖关系）：粘贴gcc使用`-MMD`生成的`.d`文件
-include $(addprefix $(DST_DIR)/, $(addsuffix .d, $(basename $(SRCS))))

## 6. 其他杂项

### 构建顺序控制
image: image-dep
archive: $(ARCHIVE)
image-dep: $(LIBS) $(IMAGE).elf
.NOTPARALLEL: image-dep
.PHONY: image image-dep archive run $(LIBS)

### 清理单个项目（删除`build/`）
clean:
	rm -rf Makefile.html $(WORK_DIR)/build/
.PHONY: clean

### 清理深度为2以内的所有子项目（并忽略错误）
CLEAN_ALL = $(dir $(shell find . -mindepth 2 -name Makefile))
clean-all: $(CLEAN_ALL) clean
$(CLEAN_ALL):
	-@$(MAKE) -s -C $@ clean
.PHONY: clean-all $(CLEAN_ALL)
