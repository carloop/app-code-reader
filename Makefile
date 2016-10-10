# Build and run unit tests
# Makefile derived from https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

TARGET_EXEC ?= test

BUILD_DIR ?= build
SRC_DIRS ?= src tests

# All files are not used for the unit tests, so just list the ones needed manually
SRCS := src/dtc.cpp src/OBDMessage.cpp $(shell find tests -name *.cpp)
# SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(SRC_DIRS)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# Use -MMD and -MP to generate dependency files
CXXFLAGS += $(INC_FLAGS) -MMD -MP -DUNIT_TEST -std=c++11

all: $(BUILD_DIR)/$(TARGET_EXEC) run

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean run watch

run:
	./$(BUILD_DIR)/$(TARGET_EXEC)

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

# Use the rerun command to continuously build and run the unit tests
watch:
	rerun -x -p '{*.cpp,*.h,Makefile}' make

# Compile firmware in the cloud
FIRMWARE_BIN ?= $(BUILD_DIR)/fault_code_reader.bin
firmware:
	$(MKDIR_P) $(BUILD_DIR)
	particle compile photon src --saveTo $(FIRMWARE_BIN)

flash:
	particle flash --usb $(FIRMWARE_BIN)


MKDIR_P ?= mkdir -p
