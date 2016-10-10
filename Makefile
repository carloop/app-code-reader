################################################################################
# Compile firmware in the cloud and flash to a device
# 
# make firmware
#   Compile in the cloud
# make flash
#   Flash to a device in DFU mode (flashing yellow)
#
################################################################################
BUILD_DIR ?= build
PLATFORM ?= photon
FIRMWARE_BIN ?= $(BUILD_DIR)/fault_code_reader.bin
firmware:
	$(MKDIR_P) $(BUILD_DIR)
	particle compile $(PLATFORM) src --saveTo $(FIRMWARE_BIN)

flash:
	particle flash --usb $(FIRMWARE_BIN)

################################################################################
# Build and run unit tests
#
# make tests
#   Compile and run the tests
# make watch
#   Continuously compile and run the tests
#
# Makefile derived from https://spin.atomicobject.com/2016/08/26/makefile-c-projects/
################################################################################

TARGET_EXEC ?= test

SRC_DIRS ?= src tests

# Not all files are used for the unit tests, so just list the ones needed manually
SRCS := src/dtc.cpp src/OBDMessage.cpp $(shell find tests -name *.cpp)
# SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(SRC_DIRS)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# Use -MMD and -MP to generate .d dependency files
CXXFLAGS += $(INC_FLAGS) -MMD -MP -DUNIT_TEST -std=c++11

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CFLAGS) -c $< -o $@

tests: $(BUILD_DIR)/$(TARGET_EXEC)
	./$(BUILD_DIR)/$(TARGET_EXEC)

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

# Use the rerun command to continuously build and run the unit tests
watch:
	rerun -x -p '{*.cpp,*.h,Makefile}' make tests


MKDIR_P ?= mkdir -p

.PHONY: clean run watch firmware flash

