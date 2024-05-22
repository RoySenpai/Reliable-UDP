# Use the gcc compiler for C files.
CC = gcc

# Use the g++ compiler for C++ files.
CPPC = g++

# Flags for the compiler (C).
CFLAGS = -Wall -Wextra -Werror -std=gnu11 -g -I$(SOURCE_PATH)

# Flags for the compiler (C++).
CPPFLAGS = -Wall -Wextra -Werror -std=gnu++17 -g -I$(SOURCE_PATH)

# Extra flags for the compiler (C++).
CPPFLAGS_EXTRA = -fPIC

# Flags for the linker (C++).
LDFLAGS = -shared

# Extra flags for the linker (C/C++).
LDFLAGS_EXTRA = -L$(BIN_PATH) -lRUDP

# Command to remove files.
RM = rm -rf

# Constants for the source, object and include paths.
SOURCE_PATH = src
BIN_PATH = bin
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
OBJECTS = $(subst $(SOURCE_PATH), $(OBJECT_PATH), $(SOURCES:.cpp=.o), $(SOURCES:.c=.o))
OBJECTS_EXAMPLES = $(subst $(EXAMPLES_PATH), $(OBJECT_EXAMPLES_PATH), $(SOURCES_EXAMPLES:.cpp=.o), $(SOURCES_EXAMPLES:.c=.o))

# CPP library object files and shared library.
RUDP_LIB_OBJS_FILES = rudp_lib.o rudp_lib_c_wrap.o rudp_lib_cpp_wrap.o
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

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean directories lib example example_cpp example_c install uninstall runscpp runccpp runsc runcc

# Default target - compile everything and create the executables and libraries.
all: directories $(TARGET) example install

# Alias for the default target.
default: all

# Compile the shared library only.
lib: $(TARGET)

# Compile the client and server examples (C++).
example_cpp: $(CPP_CLIENT_TARGET) $(CPP_SERVER_TARGET)

# Compile the client and server examples (C).
example_c: $(C_CLIENT_TARGET) $(C_SERVER_TARGET)

# Compile the client and server examples (C and C++).
example: directories example_cpp example_c

# Create the directories for the object files and executables.
directories: $(BIN_PATH) $(OBJECT_PATH) $(BIN_EXAMPLES_PATH) $(OBJECT_EXAMPLES_PATH)

# Install the shared library in the system.
install: directories $(TARGET)
	sudo cp $(TARGET) /usr/lib/$(RUDP_SHARED_LIB)

uninstall:
	sudo rm -f /usr/lib/$(RUDP_SHARED_LIB)


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


############
# Programs #
############

$(TARGET): $(RUDP_LIB_OBJECTS)
	$(CPPC) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(CPP_CLIENT_TARGET): $(CPP_CLIENT_OBJECTS) $(TARGET)
	$(CPPC) $(CPPFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(CPP_SERVER_TARGET): $(CPP_SERVER_OBJECTS) $(TARGET)
	$(CPPC) $(CPPFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(C_CLIENT_TARGET): $(C_CLIENT_OBJECTS) $(TARGET)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(C_SERVER_TARGET): $(C_SERVER_OBJECTS) $(TARGET)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_EXTRA)

$(BIN_PATH) $(OBJECT_PATH) $(BIN_EXAMPLES_PATH) $(OBJECT_EXAMPLES_PATH):
	mkdir -p $@


################
# Object files #
################

# Compile all the C++ library files that are in the source directory into object files that are in the object directory.
$(OBJECT_PATH)/%.o: $(SOURCE_PATH)/%.cpp $(HEADERS)
	$(CPPC) $(CPPFLAGS) $(CPPFLAGS_EXTRA) -c $< -o $@

# Compile all the C++ example files that are in the examples directory into object files that are in the object directory.
$(OBJECT_EXAMPLES_PATH)/%.o: $(EXAMPLES_PATH)/%.cpp $(EXAMPLES_HEADERS)
	$(CPPC) $(CPPFLAGS) -c $< -o $@

# Compile all the C example files that are in the examples directory into object files that are in the object directory.
$(OBJECT_EXAMPLES_PATH)/%.o: $(EXAMPLES_PATH)/%.c $(EXAMPLES_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

#################
# Cleanup files #
#################

# Remove all the object files, shared libraries and executables. Also uninstall the shared library.
clean: uninstall
	$(RM) $(BIN_PATH)