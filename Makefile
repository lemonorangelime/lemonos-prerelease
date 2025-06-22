BUILD_DIR := build

CC := gcc
S := gcc
ASM := nasm
ASMFLAGS := -f elf32
CCFLAGS := -O3 -mgeneral-regs-only -mhard-float -static -m32 -fno-builtin -fno-builtin-function -fno-stack-protector -fomit-frame-pointer -falign-functions=16 -nostdlib -nostdinc -funsigned-char -Iinclude -Ilinked-lists/include/ -g -Wno-address-of-packed-member
LD := ld
LDFLAGS := -m elf_i386 --strip-all --discard-all --discard-locals --strip-debug
MAKE := make
AR := ar
GRUB := grub-mkrescue
GRUBFLAGS := --compress=xz
# does this cause side effects?
# --install-modules="normal part_gpt all_video multiboot"
GRUBVOLID := "LEMON_OS"
GRUBDST := lemonos.iso
GRUBSRC := src/grub/*
QEMU := qemu-system-i386
QEMUFLAGS := -cpu host --enable-kvm -vga std -machine acpi=on -boot dca
QEMUMEMORY := 16m

DISK=1

STRUCTURE := $(shell find src/ -type d)
FILES := $(addsuffix /*,$(STRUCTURE))
FILES := $(wildcard $(FILES))

SOURCES := $(filter %.c,$(FILES))
ASM_SOURCES := $(filter %.asm,$(FILES))

OBJS := $(subst src/,build/,$(SOURCES:%.c=%.c.o))
ASM_OBJS := $(subst src/,build/,$(ASM_SOURCES:%.asm=%.asm.o))

BUILD_STRUCTURE := $(subst src/,build/,$(STRUCTURE))

OUTPUT := kernel.bin
LINKED := linked-lists/linked.o

default: mkdir depends build grub qemu

depends:
	if [ ! -d "./linked-lists" ]; then \
		git clone https://github.com/kitty14956590/linked-lists; \
	else \
		cd linked-lists; \
		git pull; \
		cd ..; \
	fi
	if [ ! -f "./linked-lists/linked.o" ]; then \
		cd linked-lists; \
		make; \
		cd ..; \
	fi

mkdir:
	mkdir -p ${BUILD_DIR} ${BUILD_STRUCTURE}

clean:
	rm -rf ${BUILD_DIR} ${OUTPUT} ${GRUBDST}

grub:
	mkdir -p ${BUILD_DIR}/grub/
	mkdir -p ${BUILD_DIR}/grub/boot/
	mkdir -p ${BUILD_DIR}/grub/boot/grub/
	cp ${OUTPUT} ${BUILD_DIR}/grub/boot/kernel.bin
	cp disks/${DISK}.tar ${BUILD_DIR}/grub/boot/disk.tar
	cp -r ${GRUBSRC} ${BUILD_DIR}/grub/boot/grub/
	${GRUB} ${GRUBFLAGS} -o ${GRUBDST} ${BUILD_DIR}/grub/ -- -volid $(GRUBVOLID)

$(BUILD_DIR)/%.c.o: src/%.c
	$(CC) $(CCFLAGS) $^ -c -o $@

$(BUILD_DIR)/%.asm.o: src/%.asm
	$(ASM) $(ASMFLAGS) $^ -o $@

build: $(OBJS) $(ASM_OBJS)
	$(LD) $(LDFLAGS) -T src/link.ld -o $(OUTPUT) $(LINKED) $^
	strip $(OUTPUT)

qemu:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 -cdrom $(GRUBDST) -m $(QEMUMEMORY)
