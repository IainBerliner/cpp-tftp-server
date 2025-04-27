CXX      = g++
CXXFLAGS = -MMD -MD -MP -std=c++20
LDFLAGS  = -include$(SRC:.cpp=.d)
LDLIBS   =
OBJECTS  = src/main.o src/argument_handler/argument_handler.o src/connection_handler/connection_options.o src/connection_handler/connection_handler.o \
	         src/connection_handler/file_wrapper/file_wrapper.o src/connection_handler/file_wrapper/netascii_formatter.o src/networking/networking.o    \
	         src/networking/connection.o src/random/random.o src/tftp_server/tftp_server.o
WARNING  = -Wall -Wextra

.PHONY: all, release, debug-build, clean

all: release debug-build

release: $(patsubst src/%, build/%, $(OBJECTS))
	$(CXX) $(LD_FLAGS) -o cpp-tftp-server $(patsubst src/%, build/%, $(OBJECTS)) $(LDLIBS)

debug-build: $(patsubst src/%, build/%, $(OBJECTS:.o=_debug_build.o))
	$(CXX) $(LD_FLAGS) -o cpp-tftp-server-debug-build $(patsubst src/%, build/%, $(OBJECTS:.o=_debug_build.o)) $(LDLIBS)
	
$(patsubst src/%, build/%, $(OBJECTS)): build/%.o: src/%.cpp Makefile
	-mkdir -p $(dir $@)
	$(CXX) $(WARNING) $(CXXFLAGS) -MMD -MP -c $< -o $@
-include $(patsubst src/%.o, build/%.d, $(OBJECTS))
	
$(patsubst src/%, build/%, $(OBJECTS:.o=_debug_build.o)): build/%_debug_build.o: src/%.cpp Makefile
	-mkdir -p $(dir $@)
	$(CXX) $(WARNING) -D DEBUG_BUILD $(CXXFLAGS) -g -c $< -o $@
-include $(patsubst src/%.o, build/%_debug_build.d, $(OBJECTS))
	
clean:
	rm -rf build
