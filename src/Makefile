# Compiler options
CC=clang++
CFLAGS=-g -O0 -fPIC -D_XOPEN_SOURCE=700 -std=c++14 -Wmain -W -Wall -Wthread-safety -pedantic -pedantic-errors
CCS=-fsanitize=address,undefined,leak
LDFLAGS=-lrt -lpthread

# Sources
SOURCES_SERVER=printer.cpp
SOURCES_CLIENT=client.cpp

# Executable names
EXECUTABLE_SERVER=printer
EXECUTABLE_SERVER_VG=printer_vg
EXECUTABLE_CLIENT=client
EXECUTABLE_CLIENT_VG=client_vg
EXECUTABLELIST= $(EXECUTABLE_SERVER) $(EXECUTABLE_SERVER_VG) $(EXECUTABLE_CLIENT) $(EXECUTABLE_CLIENT_VG) 

# Targets
all: printer client

printer: $(SOURCES_SERVER)
	$(CC) $(CFLAGS) $(CCS) $< -o $(EXECUTABLE_SERVER) $(LDFLAGS)
	$(CC) $(CFLAGS) $< -o $(EXECUTABLE_SERVER_VG) $(LDFLAGS)

client: $(SOURCES_CLIENT)
	$(CC) $(CFLAGS) $(CCS) $< -o $(EXECUTABLE_CLIENT) $(LDFLAGS)
	$(CC) $(CFLAGS) $< -o $(EXECUTABLE_CLIENT_VG) $(LDFLAGS)

clean:
	rm $(EXECUTABLELIST)
