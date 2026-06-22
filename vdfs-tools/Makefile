#
# Makefile for the linux vdfs4-filesystem tools routines.
#
##########################################################################
# VDFS Tools Version Definition(MAJ.MIN-DATE)
##########################################################################
#-------------------------------------------------------------------------
MAJ_VER=1
MIN_VER=64
DATE=241105 #YYMMDD
#-------------------------------------------------------------------------
TOOLS_VERSION="$(strip $(MAJ_VER)).$(strip $(MIN_VER))-$(strip $(DATE))"
##########################################################################

MKFS = mkfs.vdfs
UNPACK = unpack.vdfs
FSCK = fsck.vdfs
TUNE = tune.vdfs
INFO = info.vdfs
TEST_BINS = test.*.vdfs

CC = $(CROSS_COMPILE)gcc
HOST=`$(CROSS_COMPILE)gcc -dumpmachine`
LIBS = -lrt -lpthread
LIBZ = -lz

OPENSSL_BASE=./openssl
OPENSSL_DIR=$(OPENSSL_BASE)/openssl-1.0.1g
OPENSSL_LIB=-L$(OPENSSL_DIR) -lcrypto -ldl
OPENSSL_FILE=$(OPENSSL_DIR)/libcrypto.a
OPENSSL_PACK=$(OPENSSL_BASE)/openssl-1.0.1g.tar.gz

TEST_BASE=./test
TEST_BUILD_DIR=$(TEST_BASE)/build

CFLAGS += -Wall -Wextra -I./include
CFLAGS += -DTOOLS_VERSION=\"$(TOOLS_VERSION)\"
CFLAGS += -O1 -ggdb
CFLAGS += -DUSER_SPACE
CFLAGS += -DCONFIG_VDFS4_DEBUG
CFLAGS += -Iinclude
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -fcommon

ifdef VDFS4_NO_WARN
	CFLAGS += -Werror
endif

ifeq ($(host),x86_32)
	CROSS_COMPILE=
	HOST=i686-linux-gnu
	CFLAGS += -m32
	SECURE_CFLAGS += -m32
endif
ifeq ($(host),x86_64)
	CROSS_COMPILE=
	HOST=x86_64-linux-gnu
endif

CFLAGS += -I$(OPENSSL_DIR)/include

#Secure flags
ifeq ($(ENABLE_SECURE_FLAGS),1)
 SECURE_CFLAGS += -fstack-protector-strong
 SECURE_CFLAGS += -Wl,-z,relro
 SECURE_CFLAGS += -D_FORTIFY_SOURCE=2 -O1
 SECURE_CFLAGS += -fPIE -pie
 CFLAGS += $(SECURE_CFLAGS)
endif

SOURCE_MKFS = $(wildcard ./mkfs/*.c)
SOURCE_UNPACK = $(wildcard ./unpack/*.c)
SOURCE_FSCK = $(wildcard ./fsck/*.c)
SOURCE_TUNE = $(wildcard ./tune/*.c)
SOURCE_LIB = $(wildcard ./lib/*.c)
SOURCE_INFO = $(wildcard ./info/*.c)


DEPS = $(wildcard ./mkfs/*.d)
DEPS := $(DEPS) $(wildcard ./unpack/*.d)
DEPS := $(DEPS) $(wildcard ./fsck/*.d)
DEPS := $(DEPS) $(wildcard ./tune/*.d)
DEPS := $(DEPS) $(wildcard ./lib/*.d)
DEPS := $(DEPS) $(wildcard ./info/*.d)

DEPS := $(DEPS) $(wildcard ./tune/*.d)
DEPS := $(DEPS) $(wildcard ./lib/*.d)

ZSTD_BASE = ./zstd
ZSTD_ARCH = $(wildcard $(ZSTD_BASE)/zstd*.tar.gz)
ZSTD_DIR = $(ZSTD_ARCH:.tar.gz=)
ZSTD_FILE = $(ZSTD_DIR)/lib/libzstd.a
CFLAGS += -I$(ZSTD_DIR)/lib

ZLIB_BASE = ./zlib
ZLIB_ARCH = $(wildcard $(ZLIB_BASE)/zlib*.tar.gz)
ZLIB_DIR = $(ZLIB_ARCH:.tar.gz=)
ZLIB_FILE = $(ZLIB_DIR)/libz.a
CFLAGS += -I$(ZLIB_DIR)

LZOLIB_BASE = ./lzolib
LZOLIB_ARCH = $(wildcard $(LZOLIB_BASE)/lzo*.tar.gz)
LZOLIB_DIR = $(LZOLIB_ARCH:.tar.gz=)
LZOLIB_FILE = $(LZOLIB_DIR)/src/.libs/liblzo2.a
CFLAGS += -I$(LZOLIB_DIR)/include
CFLAGS += -I$(HOME)/local/include

OBJ_MKFS = $(SOURCE_MKFS:.c=.o)
OBJ_TUNE = $(SOURCE_TUNE:.c=.o)
OBJ_LIB = $(SOURCE_LIB:.c=.o)
OBJ_FSCK = $(SOURCE_FSCK:.c=.o)
OBJ_UNPACK = $(SOURCE_UNPACK:.c=.o)
OBJ_INFO = $(SOURCE_INFO:.c=.o)

unpack: CFLAGS += -D__RD_FROM_VOL__
fsck: CFLAGS += -D__RD_FROM_VOL__
btrtst: CFLAGS += -DCONFIG_VDFS4_DEBUG_TOOLS_GET_BNODE

all: mkfs

openssl: $(OPENSSL_PACK)
	@if [ ! -d $(OPENSSL_DIR) ]; then tar -xf $(OPENSSL_PACK) -C $(OPENSSL_BASE); cd $(OPENSSL_DIR); ./Configure no-shared no-asm linux-elf --cross-compile-prefix=$(CROSS_COMPILE) $(SECURE_CFLAGS); make build_crypto; fi

zlib: $(ZLIB_ARCH)
	@if [ ! -d $(ZLIB_DIR) ]; then tar -xf $(ZLIB_ARCH) -C $(ZLIB_BASE); cd $(ZLIB_DIR); env CC=$(CROSS_COMPILE)gcc CFLAGS="$(SECURE_CFLAGS)" ./configure; make; fi

zstd: $(ZSTD_ARCH)
	@if [ ! -d $(ZSTD_DIR) ]; then tar -xf $(ZSTD_ARCH) -C $(ZSTD_BASE); cd $(ZSTD_DIR); env CC=$(CROSS_COMPILE)gcc CFLAGS="$(SECURE_CFLAGS)" make lib-mt; fi

lzo: $(LZO_ARCH)
	@if [ ! -d $(LZOLIB_DIR) ]; then tar -xf $(LZOLIB_ARCH) -C $(LZOLIB_BASE); cd $(LZOLIB_DIR); env CFLAGS="$(SECURE_CFLAGS)" ./configure --host=$(HOST) --target=$(HOST) -q; make src/liblzo2.la; fi

mkfs: lzo zlib zstd openssl $(OBJ_LIB) $(OBJ_MKFS)
	@$(CC) -g -rdynamic -std=gnu99 -o $(MKFS) $(OBJ_LIB) $(OBJ_MKFS) $(LIBS) $(ZLIB_FILE) $(ZSTD_FILE) $(LZOLIB_FILE) $(OPENSSL_LIB) $(SECURE_CFLAGS) $(CFLAGS)
	@echo "  CCLD      " $@;

unpack: zlib lzo zstd openssl $(OBJ_LIB) $(OBJ_UNPACK)
	@$(CC) -o $(UNPACK) $(OBJ_LIB) $(OBJ_UNPACK) $(LIBS) $(ZLIB_FILE) $(ZSTD_FILE) $(LZOLIB_FILE) $(OPENSSL_LIB) $(CFLAGS)
	@echo "  CCLD      " $@;

tune: lzo zlib zstd openssl $(OBJ_LIB) $(OBJ_TUNE)
	@$(CC) -std=gnu99 -o $(TUNE) $(OBJ_LIB) $(OBJ_TUNE) $(LIBS) $(ZLIB_FILE) $(ZSTD_FILE) $(LZOLIB_FILE) $(OPENSSL_LIB) $(CFLAGS)
	@echo "  CCLD      " $@;

fsck: zlib lzo zstd openssl $(OBJ_LIB) $(OBJ_FSCK)
	@$(CC) -std=gnu99 -o $(FSCK) $(OBJ_LIB) $(OBJ_FSCK) $(LIBS) $(ZLIB_FILE) $(ZSTD_FILE) $(LZOLIB_FILE) -lm $(OPENSSL_LIB) $(CFLAGS)
	@echo "  CCLD      " $@;

info: openssl $(OBJ_INFO)
	@$(CC) -std=gnu99 -o $(INFO) $(OBJ_INFO) $(CFLAGS)
	@echo "  CCLD      " $@;

%.o : %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC        " $@;
-include $(DEPS)

#List up libraries for test build
LIB_FILES = $(shell readlink -m $(OPENSSL_FILE))
LIB_FILES += $(shell readlink -m $(ZLIB_FILE))
LIB_FILES += $(shell readlink -m $(ZSTD_FILE))
LIB_FILES += $(shell readlink -m $(LZOLIB_FILE))
LIB_LIST = dl m rt pthread

test: zlib lzo zstd openssl $(OBJ_INFO) $(OBJ_FSCK) $(OBJ_LIB)
	@mkdir -p $(TEST_BUILD_DIR)
	@cmake $(TEST_BASE) -B$(TEST_BUILD_DIR) -DLIB_FILES="$(LIB_FILES)" -DLIB_LIST="$(LIB_LIST)"
	-make -C $(TEST_BUILD_DIR) --no-print-directory
	@cp -f $(TEST_BUILD_DIR)/*/$(TEST_BINS) ./

testclean:
	@make -C $(TEST_BUILD_DIR) --no-print-directory clean
	@rm -f $(TEST_BINS)
	@rm -rf $(TEST_BUILD_DIR)

clean:
	-rm -f $(OBJ_LIB)
	-rm -f $(MKFS) $(OBJ_MKFS)
	-rm -f $(UNPACK) $(OBJ_UNPACK)
	-rm -f $(FSCK) $(OBJ_FSCK)
	-rm -f $(TUNE) $(OBJ_TUNE)
	-rm -f $(INFO) $(OBJ_INFO)
	-rm -rf *.o
	-rm -rf ./src/*.o
	-rm -f $(DEPS)
	$(shell rm -f `find -name \*.o` > /dev/null 2> /dev/null)
	$(shell rm -f `find -name \*.d` > /dev/null 2> /dev/null)
	-rm -f $(TEST_BINS)
	$(shell test -d $(TEST_BUILD_DIR) && make -C $(TEST_BUILD_DIR) --no-print-directory clean)

distclean: clean
	-rm -rf $(ZLIB_DIR)
	-rm -rf $(ZSTD_DIR)
	-rm -rf $(LZOLIB_DIR)
	-rm -rf $(OPENSSL_DIR)
	-rm -rf $(TEST_BUILD_DIR)

opensource: clean
	-rm -rf ./fsck ./info ./vdcrc ./tune ./unpack ./test

.PHONY: all zlib zstd clean mkfs unpack tune fsck openssl lzo info test
