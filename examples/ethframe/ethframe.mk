#
# Copyright 2022, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF_INCLUDE:=$(SDDF)/include/sddf
ETHFRAME:=${SDDF}/examples/ethframe
UTIL:=$(SDDF)/util
ETHERNET_DRIVER:=$(SDDF)/drivers/network/$(DRIV_DIR)
ETHERNET_CONFIG_INCLUDE:=${ETHFRAME}/include/ethernet_config

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_FILE := ${ETHFRAME}/board/$(MICROKIT_BOARD)/ethframe.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

vpath %.c ${SDDF} ${ETHFRAME}

IMAGES := eth_driver.elf recv.elf

CFLAGS := -mcpu=$(CPU) \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
	  -I${ETHERNET_CONFIG_INCLUDE} \
	  -MD \
	  -MP

LDFLAGS := -L$(BOARD_DIR)/lib -L${LIBC}
LIBS := --start-group -lmicrokit -Tmicrokit.ld -lc libsddf_util_debug.a --end-group

%.elf: %.o
	$(LD) $(LDFLAGS) $< $(LIBS) -o $@

RECV_SRCS = recv.c
RECV_OBJS := $(RECV_SRCS:.c=.o)
OBJS := $(RECV_OBJS)
DEPS := $(filter %.d,$(OBJS:.o=.d))

all: loader.img

recv.o: recv.c

recv.elf: $(RECV_OBJS) libsddf_util.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Need to build libsddf_util_debug.a because it's included in LIBS
# for the unimplemented libc dependencies
${IMAGES}: libsddf_util_debug.a

${IMAGE_FILE} $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

include ${SDDF}/util/util.mk
include ${ETHERNET_DRIVER}/eth_driver.mk

clean::
	${RM} -f *.elf .depend* $
	find . -name \*.[do] | xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f ${IMAGE_FILE} ${REPORT_FILE}

-include $(DEPS)
