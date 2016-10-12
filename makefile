Main_Src = \
	src/Procedure.cpp

Cpp_Src = \
	src/MpMuchacho.cc

Libs = deps

C_Src = \
	${Libs}/cJSON/cJSON.c \
	${Libs}/msgpack/src/objectc.c \
	${Libs}/msgpack/src/unpack.c \
	${Libs}/msgpack/src/version.c \
	${Libs}/msgpack/src/vrefbuffer.c \
	${Libs}/msgpack/src/zone.c

Incl = -I.\
	-I${Libs}/cJSON \
	-I${Libs}/msgpack/include

all:
	gcc -g -c ${C_Src} ${Incl}
	g++ -g -c ${Cpp_Src} ${Incl}
	g++ -g ${Incl} ${Main_Src} *.o -o bin/makemp

clean:
	rm -f *.o
	rm -rf makemp.dSYM

