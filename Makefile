###############################################
# Makefile for the RUDP library and examples. #
# Made by: Roy Simanovich					  #
###############################################

# Debug or release mode (1 for debug, 0 for release).
DEBUG ?= 0

# Constants for the source and binary paths.
SOURCE_PATH = src

# Use the gcc compiler for C files.
CC = gcc

# Use the g++ compiler for C++ files.
CPPC = g++

ifeq ($(DEBUG), 1)
	# Flags for the compiler (C).
	CFLAGS = -Wall -Wextra -Werror -std=gnu11 -g -I$(SOURCE_PATH)

	# Flags for the compiler (C++).
	CPPFLAGS = -Wall -Wextra -Werror -std=gnu++17 -g -I$(SOURCE_PATH)

	BIN_PATH = debug
else
	# Flags for the compiler (C).
	CFLAGS = -Wall -Wextra -Werror -std=gnu11 -s -I$(SOURCE_PATH)

	# Flags for the compiler (C++).
	CPPFLAGS = -Wall -Wextra -Werror -std=gnu++17 -s -I$(SOURCE_PATH)

	BIN_PATH = bin
endif

# Extra flags for the compiler (C++).
CPPFLAGS_EXTRA = -fPIC

# Detect the operating system
ifdef OS
	ifeq ($(OS), Windows_NT)
		PLATFORM = Windows
		SystemRoot = $(shell echo %SystemRoot%)

		# Flags for the linker (C++).
		LDFLAGS = -shared -Wl,--out-implib,$(BIN_PATH)\libRUDP.a

		# Extra flags for the linker (C/C++).
		LDFLAGS_EXTRA = -L$(BIN_PATH) -lRUDP -lws2_32

		# Command to remove files.
		RM = del /q /s

		# Memory checker tool (drmemory).
		MEMCHECK_TOOL = drmemory

		# Flags for the memory checker.
		MEMCHECK_FLAGS = -verbose 0 -ignore_kernel -show_reachable -logdir $(BIN_PATH)

		# Constants for the source, object and include paths.
		BIN_EXAMPLES_PATH = $(BIN_PATH)\examples
		OBJECT_PATH = $(BIN_PATH)\objects
		OBJECT_EXAMPLES_PATH = $(OBJECT_PATH)\examples
		INCLUDE_PATH = $(SOURCE_PATH)\include
		EXAMPLES_PATH = $(SOURCE_PATH)\examples
		EXAMPLES_INCLUDE_PATH = $(EXAMPLES_PATH)\include

		# Variables for the source, object and header files.
		SOURCES = $(wildcard $(SOURCE_PATH)\*.cpp $(SOURCE_PATH)\*.c $(EXAMPLES_PATH)\*.cpp $(EXAMPLES_PATH)\*.c)
		SOURCES_EXAMPLES = $(wildcard $(EXAMPLES_PATH)\*.cpp $(EXAMPLES_PATH)\*.c)
		HEADERS = $(wildcard $(INCLUDE_PATH)\*.hpp $(INCLUDE_PATH)\*.h)
		EXAMPLES_HEADERS = $(wildcard $(EXAMPLES_INCLUDE_PATH)\*.hpp $(EXAMPLES_INCLUDE_PATH)\*.h)

		# CPP library object files and shared library.
		RUDP_LIB_OBJECTS = $(addprefix $(OBJECT_PATH)\, $(RUDP_LIB_OBJS_FILES))
		RUDP_SHARED_LIB = libRUDP.dll
		TARGET = $(BIN_PATH)\$(RUDP_SHARED_LIB)

		# CPP client and server object files and executables.
		CPP_CLIENT_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)\, RUDP_Sender_CPP.o)
		CPP_SERVER_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)\, RUDP_Receiver_CPP.o)
		CPP_CLIENT_TARGET = $(BIN_EXAMPLES_PATH)\RUDP_Sender_CPP.exe
		CPP_SERVER_TARGET = $(BIN_EXAMPLES_PATH)\RUDP_Receiver_CPP.exe

		# C client and server object files and executables.
		C_CLIENT_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)\, RUDP_Sender_C.o)
		C_SERVER_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)\, RUDP_Receiver_C.o)
		C_CLIENT_TARGET = $(BIN_EXAMPLES_PATH)\RUDP_Sender_C.exe
		C_SERVER_TARGET = $(BIN_EXAMPLES_PATH)\RUDP_Receiver_C.exe
	endif
else
	PLATFORM = Linux

	# Flags for the linker (C++).
	LDFLAGS = -shared

	# Extra flags for the linker (C/C++).
	LDFLAGS_EXTRA = -L$(BIN_PATH) -lRUDP

	# Command to remove files.
	RM = rm -rf

	# Memory checker tool (valgrind).
	MEMCHECK_TOOL = valgrind

	# Flags for the memory checker.
	MEMCHECK_FLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes

	# Constants for the source, object and include paths.
	BIN_EXAMPLES_PATH = $(BIN_PATH)/examples
	OBJECT_PATH = $(BIN_PATH)/objects
	OBJECT_EXAMPLES_PATH = $(OBJECT_PATH)/examples
	INCLUDE_PATH = $(SOURCE_PATH)/include
	EXAMPLES_PATH = $(SOURCE_PATH)/examples
	EXAMPLES_INCLUDE_PATH = $(EXAMPLES_PATH)/include

	# Variables for the source, object and header files.
	SOURCES = $(wildcard $(SOURCE_PATH)/*.cpp $(SOURCE_PATH)/*.c $(EXAMPLES_PATH)/*.cpp $(EXAMPLES_PATH)/*.c)
	SOURCES_EXAMPLES = $(wildcard $(EXAMPLES_PATH)/*.cpp $(EXAMPLES_PATH)/*.c)
	HEADERS = $(wildcard $(INCLUDE_PATH)/*.hpp $(INCLUDE_PATH)/*.h)
	EXAMPLES_HEADERS = $(wildcard $(EXAMPLES_INCLUDE_PATH)/*.hpp $(EXAMPLES_INCLUDE_PATH)/*.h)

	# CPP library object files and shared library.
	RUDP_LIB_OBJECTS = $(addprefix $(OBJECT_PATH)/, $(RUDP_LIB_OBJS_FILES))
	RUDP_SHARED_LIB = libRUDP.so
	TARGET = $(BIN_PATH)/$(RUDP_SHARED_LIB)

	# CPP client and server object files and executables.
	CPP_CLIENT_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)/, RUDP_Sender_CPP.o)
	CPP_SERVER_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)/, RUDP_Receiver_CPP.o)
	CPP_CLIENT_TARGET = $(BIN_EXAMPLES_PATH)/RUDP_Sender_CPP
	CPP_SERVER_TARGET = $(BIN_EXAMPLES_PATH)/RUDP_Receiver_CPP

	# C client and server object files and executables.
	C_CLIENT_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)/, RUDP_Sender_C.o)
	C_SERVER_OBJECTS = $(addprefix $(OBJECT_EXAMPLES_PATH)/, RUDP_Receiver_C.o)
	C_CLIENT_TARGET = $(BIN_EXAMPLES_PATH)/RUDP_Sender_C
	C_SERVER_TARGET = $(BIN_EXAMPLES_PATH)/RUDP_Receiver_C
	
endif

# Variables for the source, object and header files.
OBJECTS = $(subst $(SOURCE_PATH), $(OBJECT_PATH), $(SOURCES:.cpp=.o) $(SOURCES:.c=.o))
OBJECTS_EXAMPLES = $(subst $(EXAMPLES_PATH), $(OBJECT_EXAMPLES_PATH), $(SOURCES_EXAMPLES:.cpp=.o) $(SOURCES_EXAMPLES:.c=.o))

# CPP library object files.
RUDP_LIB_OBJS_FILES = rudp_lib.o rudp_lib_c_wrap.o rudp_lib_cpp_wrap.o

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean directories lib example example_cpp example_c install uninstall runscpp runccpp runsc runcc memcheckscpp memcheckccpp memchecksc memcheckcc

# Default target - compile everything and create the executables and libraries.
all: directories $(TARGET) example install

# Alias for the default target.
default: all

# Compile the shared library only.
lib: directories $(TARGET)

# Compile the client and server examples (C++).
example_cpp: $(CPP_CLIENT_TARGET) $(CPP_SERVER_TARGET)

# Compile the client and server examples (C).
example_c: $(C_CLIENT_TARGET) $(C_SERVER_TARGET)

# Compile the client and server examples (C and C++).
example: directories example_cpp example_c

# Create the directories for the object files and executables.
directories:
ifeq ($(PLATFORM), Windows)
	if not exist $(BIN_PATH) mkdir $(BIN_PATH)
	if not exist $(OBJECT_PATH) mkdir $(OBJECT_PATH)
	if not exist $(BIN_EXAMPLES_PATH) mkdir $(BIN_EXAMPLES_PATH)
	if not exist $(OBJECT_EXAMPLES_PATH) mkdir $(OBJECT_EXAMPLES_PATH)
else
	mkdir -p $(BIN_PATH) $(OBJECT_PATH) $(BIN_EXAMPLES_PATH) $(OBJECT_EXAMPLES_PATH)
endif

# Install the shared library in the system.
install: directories $(TARGET)
ifeq ($(PLATFORM), Windows)
	@echo Copying $(TARGET) to $(EXAMPLES_PATH)
	copy $(TARGET) $(BIN_EXAMPLES_PATH)

	@echo Use the following command to install the shared library into the system:
	@echo copy $(TARGET) $(SystemRoot)\System32\$(RUDP_SHARED_LIB)
else ifeq ($(PLATFORM), Linux)
	sudo cp $(TARGET) /usr/lib/$(RUDP_SHARED_LIB)
endif

uninstall:
ifeq ($(PLATFORM), Windows)
	@echo Use the following command to uninstall the shared library from the system:
	@echo del /q /s $(SystemRoot)\System32\$(RUDP_SHARED_LIB)
else ifeq ($(PLATFORM), Linux)
	sudo rm -f /usr/lib/$(RUDP_SHARED_LIB)
endif

########################################
# Run the server and client executables.
########################################
runscpp: $(CPP_SERVER_TARGET)
	./$< -p 12345

runccpp: $(CPP_CLIENT_TARGET)
	./$< -ip 127.0.0.1 -p 12345

runsc: $(C_SERVER_TARGET)
	./$< -p 12345

runcc: $(C_CLIENT_TARGET)
	./$< -ip 127.0.0.1 -p 12345


###################################################
# Memory check the server and client executables. #
###################################################
memcheckscpp: $(CPP_SERVER_TARGET)
	$(MEMCHECK_TOOL) $(MEMCHECK_FLAGS) ./$< -p 12345

memcheckccpp: $(CPP_CLIENT_TARGET)
	$(MEMCHECK_TOOL) $(MEMCHECK_FLAGS) ./$< -ip 127.0.0.1 -p 12345

memchecksc: $(C_SERVER_TARGET)
	$(MEMCHECK_TOOL) $(MEMCHECK_FLAGS) ./$< -p 12345

memcheckcc: $(C_CLIENT_TARGET)
	$(MEMCHECK_TOOL) $(MEMCHECK_FLAGS) ./$< -ip 127.0.0.1 -p 12345


############
# Programs #
############

$(TARGET): $(RUDP_LIB_OBJECTS)
ifeq ($(PLATFORM), Windows)
	$(CPPC) $(CPPFLAGS) $(LDFLAGS) $^ -o $@ -lws2_32
else ifeq ($(PLATFORM), Linux)
	$(CPPC) $(CPPFLAGS) $(LDFLAGS) $^ -o $@
endif

$(CPP_CLIENT_TARGET): $(CPP_CLIENT_OBJECTS) $(TARGET)
	$(CPPC) $(CPPFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(CPP_SERVER_TARGET): $(CPP_SERVER_OBJECTS) $(TARGET)
	$(CPPC) $(CPPFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(C_CLIENT_TARGET): $(C_CLIENT_OBJECTS) $(TARGET)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(C_SERVER_TARGET): $(C_SERVER_OBJECTS) $(TARGET)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

################
# Object files #
################

ifeq ($(PLATFORM), Windows)
# Compile all the C++ library files that are in the source directory into object files that are in the object directory.
$(OBJECT_PATH)\rudp_lib.o: $(SOURCE_PATH)\rudp_lib.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(CPPFLAGS_EXTRA) -c $< -o $@

$(OBJECT_PATH)\rudp_lib_c_wrap.o: $(SOURCE_PATH)\rudp_lib_c_wrap.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(CPPFLAGS_EXTRA) -c $< -o $@

$(OBJECT_PATH)\rudp_lib_cpp_wrap.o: $(SOURCE_PATH)\rudp_lib_cpp_wrap.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(CPPFLAGS_EXTRA) -c $< -o $@

# Compile all the C++ example files that are in the examples directory into object files that are in the object directory.
$(OBJECT_EXAMPLES_PATH)\RUDP_Sender_CPP.o: $(EXAMPLES_PATH)\RUDP_Sender_CPP.cpp $(EXAMPLES_HEADERS)
	$(CPPC) $(CPPFLAGS) -c $< -o $@

$(OBJECT_EXAMPLES_PATH)\RUDP_Receiver_CPP.o: $(EXAMPLES_PATH)\RUDP_Receiver_CPP.cpp $(EXAMPLES_HEADERS)
	$(CPPC) $(CPPFLAGS) -c $< -o $@

# Compile all the C example files that are in the examples directory into object files that are in the object directory.
$(OBJECT_EXAMPLES_PATH)\RUDP_Sender_C.o: $(EXAMPLES_PATH)\RUDP_Sender_C.c $(EXAMPLES_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECT_EXAMPLES_PATH)\RUDP_Receiver_C.o: $(EXAMPLES_PATH)\RUDP_Receiver_C.c $(EXAMPLES_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

else ifeq ($(PLATFORM), Linux)
# Compile all the C++ library files that are in the source directory into object files that are in the object directory.
$(OBJECT_PATH)/%.o: $(SOURCE_PATH)/%.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(CPPFLAGS_EXTRA) -c $< -o $@

# Compile all the C++ example files that are in the examples directory into object files that are in the object directory.
$(OBJECT_EXAMPLES_PATH)/%.o: $(EXAMPLES_PATH)/%.cpp $(EXAMPLES_HEADERS)
	$(CPPC) $(CPPFLAGS) -c $< -o $@

# Compile all the C example files that are in the examples directory into object files that are in the object directory.
$(OBJECT_EXAMPLES_PATH)/%.o: $(EXAMPLES_PATH)/%.c $(EXAMPLES_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
endif

#################
# Cleanup files #
#################

# Remove all the object files, shared libraries and executables. Also uninstall the shared library.
clean: uninstall
ifeq ($(PLATFORM), Windows)
	$(RM) $(BIN_PATH)
	if exist $(BIN_PATH) RD /S /Q $(BIN_PATH)
else ifeq ($(PLATFORM), Linux)
	$(RM) $(BIN_PATH)
endif