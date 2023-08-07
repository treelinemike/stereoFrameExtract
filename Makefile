INCLUDE_SYS = /usr/local/include
INCLUDE = ~/include ./include /usr/include/opencv4 ../cxxopts/include
LIB = /usr/local/lib

CC = g++
CFLAGS = -no-pie -pthread -Wall
LD_FLAGS_CROP = -lavutil -lavformat -lavcodec
LD_FLAGS_TIFF = -lavutil -lavformat -lavcodec -lswscale -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs

INC_SYS_PARAMS = $(addprefix -isystem,$(INCLUDE_SYS))
INC_PARAMS = $(addprefix -I,$(INCLUDE))
LIB_PARAMS = $(addprefix -L,$(LIB))

default: vcrop vtiff

vcrop: vcrop.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LD_FLAGS_CROP)

vtiff: vtiff.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LD_FLAGS_TIFF)

vcrop.o: ./src/vcrop.cpp
	$(CC) $(CFLAGS) $(INC_SYS_PARAMS) $(INC_PARAMS) -o $@ -c $<

vtiff.o: ./src/vtiff.cpp
	$(CC) $(CFLAGS) $(INC_SYS_PARAMS) $(INC_PARAMS) -o $@ -c $<

clean:
	rm -r *.o
	