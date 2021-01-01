# Use VBCC toolchain
CC := vc
AS := vasmm68k_mot

VBCC_CONFIG ?= aos68k
CFLAGS += +$(VBCC_CONFIG) -c99
CXXFLAGS += +$(VBCC_CONFIG)
LDFLAGS += +$(VBCC_CONFIG)

# Default to NDK3.9
NDK ?= /usr/share/ndk39
NDK_INC_H ?= $(NDK)/Include/include_h
NDK_INC_I ?= $(NDK)/Include/include_i
NDK_LIB ?= $(NDK)/Include/linker_libs

# Add NDK include paths
CPPFLAGS += -I$(NDK_INC_H)
ASFLAGS += -I$(NDK_INC_I)
LDFLAGS += -lamiga

TARGET_EXEC ?= a.out
BUILD_DIR ?= ./bin
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# link
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
