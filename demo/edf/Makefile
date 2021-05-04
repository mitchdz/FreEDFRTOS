CC := gcc
BIN := posix_demo

BUILD_DIR := build

FREERTOS_DIR_REL := ../..
FREERTOS_DIR := $(abspath $(FREERTOS_DIR_REL))

INCLUDE_DIRS := -I.
INCLUDE_DIRS += -I${FREERTOS_DIR}/include
INCLUDE_DIRS += -I${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix
INCLUDE_DIRS += -I${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix/utils
INCLUDE_DIRS += -I${FREERTOS_DIR}/demo/Common/include

SOURCE_FILES := $(wildcard *.c)
SOURCE_FILES += $(wildcard ${FREERTOS_DIR}/src/*.c)
# Memory manager (use malloc() / free() )
SOURCE_FILES += ${FREERTOS_DIR}/portable/MemMang/heap_3.c
# posix port
SOURCE_FILES += ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
SOURCE_FILES += ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix/port.c

# Demo library.
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/AbortDelay.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/BlockQ.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/blocktim.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/countsem.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/death.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/dynamic.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/EventGroupsDemo.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/flop.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/GenQTest.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/integer.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/IntSemTest.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/MessageBufferAMP.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/MessageBufferDemo.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/PollQ.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/QPeek.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/QueueOverwrite.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/QueueSet.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/QueueSetPolling.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/recmutex.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/semtest.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/StaticAllocation.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/StreamBufferDemo.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/StreamBufferInterrupt.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/TaskNotify.c
SOURCE_FILES += ${FREERTOS_DIR}/demo/Common/Minimal/TimerDemo.c

CFLAGS := -DDEBUG -ggdb3 -O0 -DprojCOVERAGE_TEST=0 -D_WINDOWS_
LDFLAGS := -ggdb3 -O0 -pthread -lpcap

OBJ_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

DEP_FILE = $(OBJ_FILES:%.o=%.d)

${BIN} : $(BUILD_DIR)/$(BIN)

${BUILD_DIR}/${BIN} : ${OBJ_FILES}
	-mkdir -p ${@D}
	$(CC) $^ $(CFLAGS) $(INCLUDE_DIRS) ${LDFLAGS} -o $@


-include ${DEP_FILE}

${BUILD_DIR}/%.o : %.c
	-mkdir -p $(@D)
	$(CC) $(CFLAGS) ${INCLUDE_DIRS} -MMD -c $< -o $@

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR)







