TARGET     = vk-music-sync
CC         = g++
CFLAGS     = -Wall -std=c++11
LIBS       = -lcurl -ljansson -ltag
SOURCES    = main.cpp vk.cpp depends/iniconfig.cpp depends/variant.cpp
OBJECTS    = $(SOURCES:.cpp=.o)

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(TARGET) $(OBJECTS)
