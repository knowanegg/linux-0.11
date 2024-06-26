# ----------------------------------------------------------------------
# Qemu option
#
# Copyright (C) 2017 Buddy <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

# -----------------------------------
# Qemu flags
# -----------------------------------
QEMU_FLAGS :=

# -----------------------------------
# IMAGE Path
# -----------------------------------
IMAGE_NAME := $(srctree)/arch/$(SRCARCH)/kernel/BiscuitOS

# -----------------------------------
# Platform
# -----------------------------------
ifdef CONFIG_X86
  QEMU := qemu-system-i386
else
  QEMU := qemu
endif

# -----------------------------------
# Memory size
# -----------------------------------
ifneq ($(CONFIG_QEMU_MEMORY_SIZE),)
  MEMORY_SIZE := $(CONFIG_QEMU_MEMORY_SIZE)M
else
  MEMORY_SIZE := 16M
endif
QEMU_FLAGS  += -m $(MEMORY_SIZE)

# -----------------------------------
# Boot order
# On X86 machine
# a: floppy 1
# b: floppy 2
# c: first hard disk
# d: first CD-ROM
# n: Etherboot from network adapter 1
# o: Etherboot from network adapter 2
# p: Etherboot from network adapter 3
# q: Etherboot from network adapter 4
# Hard disk boot is the default
# -----------------------------------
ifdef CONFIG_QEMU_BOOT_FLOPPY
  BOOT_DEVICE := a
else
  BOOT_DEVICE := c
endif
QEMU_FLAGS += -boot order=$(BOOT_DEVICE)

# -----------------------------------
# Block device options
# Utilize file as block
# For floppy:
# -fda : floppy disk 0 image
# -fdb : floppy disk 1 image
# For Disk:
# -hda : hard disk 0 image
# -hdb : hard disk 1 image
# -hdc : hard disk 2 image
# -hdd : hard disk 3 image
# -----------------------------------
ifdef CONFIG_QEMU_BOOT_FLOPPY
  BLOCK_DEVICE := -fda $(IMAGE_NAME)
else
  BLOCK_DEVICE := -fha $(IMAGE_NAME)
endif
QEMU_FLAGS += $(BLOCK_DEVICE)

# -----------------------------------
# Rootfs
# -----------------------------------
ifdef CONFIG_QEMU_ROOTFS_NAME
ROOTFS := -hda $(CONFIG_QEMU_ROOTFS_NAME)
else
ROOTFS := -hda BiscuitOS.img
endif

QEMU_FLAGS += $(ROOTFS)

# -----------------------------------
# Display options
# -----------------------------------
CONS :=
QEMU_FLAGS += $(CONS)

# -----------------------------------
# Real Time Chip(RTC)
# -----------------------------------
# Specify time
# RTC := -rtc base=2017-10-16T16:01:21
# Local time
ifdef CONFIG_QEMU_RTC
  RTC := -rtc base=localtime
else
# UTL time (default)
  RTC := -rtc base=utc
endif
#QEMU_FLAGS += $(RTC)


# ----------------------------------
# Serial Settings
# ----------------------------------
ifdef CONFIG_QEMU_SERIAL_PTY
  SERIAL := -serial pty
endif

#QEMU_FLAGS += $(SERIAL)
# Export Argument
export QEMU_FLAGS QEMU
