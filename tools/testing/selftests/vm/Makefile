# Makefile for vm selftests

ifndef OUTPUT
  OUTPUT := $(shell pwd)
endif

CFLAGS = -Wall -I ../../../../usr/include $(EXTRA_CFLAGS)
BINARIES = compaction_test
BINARIES += hugepage-mmap
BINARIES += hugepage-shm
BINARIES += map_hugetlb
BINARIES += mlock2-tests
BINARIES += on-fault-limit
BINARIES += thuge-gen
BINARIES += transhuge-stress
BINARIES += userfaultfd
BINARIES += mlock-random-test
BINARIES += virtual_address_range

all: $(BINARIES)
%: %.c
	$(CC) $(CFLAGS) -o $@ $^ -lrt
$(OUTPUT)/userfaultfd: ../../../../usr/include/linux/kernel.h
$(OUTPUT)/userfaultfd: LDLIBS += -lpthread
mlock-random-test: mlock-random-test.c
	$(CC) $(CFLAGS) -o $@ $< -lcap

../../../../usr/include/linux/kernel.h:
	make -C ../../../.. headers_install

TEST_PROGS := run_vmtests
TEST_FILES := $(BINARIES)

include ../lib.mk

clean:
	$(RM) $(BINARIES)
