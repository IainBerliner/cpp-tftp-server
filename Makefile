CC=g++
CPPFLAGS=-g -MP -MD
LDFLAGS=-g -include$(SRC:.cpp=.d)
LDLIBS=
OBJECTS = src/main.o src/argument_handling/handle_arguments.o src/handle_connection/connection_options.o src/handle_connection/handle_connection.o \
	  src/handle_connection/netascii.o src/handle_connection/packet_handler.o src/handle_connection/path_resolution/path_resolution.o          \
	  src/networking/networking.o src/random_functions/random_functions.o src/tftp_server/tftp_server.o 

.PHONY: all, clean, main, testing-build

all: main testing-build

main: $(patsubst src/%, build/%, $(OBJECTS))
	$(CC) $(LD_FLAGS) -o cpp-tftp-server $(patsubst src/%, build/%, $(OBJECTS)) $(LDLIBS)
	
$(patsubst src/%, build/%, $(OBJECTS)): build/%.o: src/%.cpp
	-mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c $< -o $@
	
$(patsubst src/%, build/%, $(OBJECTS:.o=_testing_build.o)): build/%_testing_build.o: src/%.cpp
	-mkdir -p $(dir $@)
	$(CC) -D TESTING_BUILD $(CPPFLAGS) -c $< -o $@
	
clean:
	rm -rf build
	
testing-build: $(patsubst src/%, build/%, $(OBJECTS:.o=_testing_build.o))
	$(CC) $(LD_FLAGS) -o cpp-tftp-server-testing-build $(patsubst src/%, build/%, $(OBJECTS:.o=_testing_build.o)) $(LDLIBS)
