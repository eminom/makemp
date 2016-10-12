Main_Src = \
	src/Procedure.cpp

Cpp_Src = \
	src/MpMuchacho.cc

C_Src = \
	libs/cJSON/cJSON.c \
	libs/msgpack/src/objectc.c \
	libs/msgpack/src/unpack.c \
	libs/msgpack/src/version.c \
	libs/msgpack/src/vrefbuffer.c \
	libs/msgpack/src/zone.c

Incl = -Ilibs/src \
	-Ilibs/cJSON \
	-Ilibs/msgpack/include

all:
	gcc -g -c ${C_Src} ${Incl}
	g++ -g -c ${Cpp_Src} ${Incl}
	g++ -g ${Incl} ${Main_Src} *.o -o bin/makemp

clean:
	rm -f *.o

