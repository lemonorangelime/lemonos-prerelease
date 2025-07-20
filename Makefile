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
QEMUFDD := -drive format=raw,media=disk,if=floppy,cache=writeback,file=disks/fdd/test.img -drive format=raw,media=disk,if=floppy,cache=writeback,file=disks/fdd/test2.img
QEMUHDD := -drive format=raw,media=disk,if=ide,cache=writeback,file=disks/hdd/test.img
QEMUAHCI := -device ahci,id=ahci -drive format=raw,id=satahd,media=disk,if=none,cache=writeback,file=disks/sata/test.img -device ide-hd,drive=satahd
QEMUSCSI := -device am53c974,id=scsi -drive format=raw,id=scsihd,media=disk,if=none,cache=writeback,file=disks/scsi/test.img -device scsi-hd,drive=scsihd
QEMUSD := -device sdhci-pci,id=sdhci -drive format=raw,id=sdhd,media=disk,if=none,cache=writeback,file=disks/sd/test.img -device sd-card,drive=sdhd
QEMUNVME := -device nvme,serial=deadbeef,drive=nvmehd -drive format=raw,id=nvmehd,media=disk,if=none,cache=writeback,file=disks/nvme/test.img
QEMUSAS := -device mptsas1068 -drive format=raw,id=sashd,media=disk,if=none,cache=writeback,file=disks/sas/test.img
QEMUPFLASH := 
# -drive format=raw,media=disk,if=pflash,cache=writeback,file=disks/pflash/test.img
QEMUVIRTIO := -drive format=raw,media=disk,if=virtio,cache=writeback,file=disks/virtio/test.img
QEMUIPMI := -device ipmi-bmc-sim,id=bmc -device pci-ipmi-kcs,bmc=bmc

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
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD) -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_adlib:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device adlib,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_gus:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device gus,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_audiopci:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device es1370,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_crystal:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device cs4231a,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_sb16:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device sb16,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_hda:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device hda,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_ac97:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD)  -audiodev pa,id=audio0 -device ac97,audiodev=audio0  -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_ipmi:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD) $(QEMUIPMI) -cdrom $(GRUBDST) -m $(QEMUMEMORY)

qemu_shit_tonne_of_disks:
	$(QEMU) $(QEMUFLAGS) -net nic,model=rtl8139,netdev=rtl8139 -netdev user,id=rtl8139 $(QEMUFDD) $(QEMUHDD) $(QEMUPFLASH) $(QEMUSCSI) $(QEMUSD) $(QEMUNVME) $(QEMUSAS) $(QEMUAHCI) -cdrom $(GRUBDST) -m $(QEMUMEMORY)
