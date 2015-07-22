CXX := g++
CXXFLAGS := -DLINUX  -Wall -std=c++0x -Wl,--no-undefined -O2
OBJECTS := main.o amx.o function.o 

TARGET_FILE = i99_Function.so

# Link command:
i99_Function.so : $(OBJECTS)
	$(CXX) -shared $(OBJECTS) -o $(TARGET_FILE)

# Compilation commands:
function.o : function.cpp function.h
	$(CXX) $(CXXFLAGS) -c function.cpp -o function.o

amx.o : sdk/amxplugin.cpp sdk/plugincommon.h sdk/amx/amx.h
	$(CXX) $(CXXFLAGS) -c sdk/amxplugin.cpp -o amx.o

main.o : main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

clean:
	rm *.o $(TARGET_FILE)
	
