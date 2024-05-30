MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF_INCLUDE:=$(SDDF)/include/sddf
PIPE:=${SDDF}/examples/pipe
UTIL:=$(SDDF)/util

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_FILE := ${PIPE}/board/$(MICROKIT_BOARD)/pipe.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

vpath %.c ${SDDF} ${PIPE}

IMAGES := send.elf recv.elf

CFLAGS := -mcpu=$(CPU) \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
	  -MD \
	  -MP

LDFLAGS := -L$(BOARD_DIR)/lib -L${LIBC}
LIBS := --start-group -lmicrokit -Tmicrokit.ld -lc libsddf_util_debug.a --end-group

%.elf: %.o
	$(LD) $(LDFLAGS) $< $(LIBS) -o $@

SEND_SRCS = send.c
SEND_OBJS := $(SEND_SRCS:.c=.o)
RECV_SRCS = recv.c
RECV_OBJS := $(RECV_SRCS:.c=.o)
OBJS := $(SEND_OBJS) $(RECV_OBJS)
DEPS := $(filter %.d,$(OBJS:.o=.d))

all: loader.img

send.o: send.c
recv.o: recv.c

send.elf: $(SEND_OBJS) libsddf_util.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

recv.elf: $(RECV_OBJS) libsddf_util.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Need to build libsddf_util_debug.a because it's included in LIBS
# for the unimplemented libc dependencies
${IMAGES}: libsddf_util_debug.a

${IMAGE_FILE} $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

include ${SDDF}/util/util.mk

clean::
	${RM} -f *.elf .depend* $
	find . -name \*.[do] | xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f ${IMAGE_FILE} ${REPORT_FILE}

-include $(DEPS)
