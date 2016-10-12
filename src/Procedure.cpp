

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MpMuchacho.h"

#ifndef MAX_PATH
#	define MAX_PATH	1024
#endif

//~ Need cJSON_Delete 
cJSON* readFile(const char *path) {
	FILE *fin = fopen(path, "r");
	if (fin) {
		fseek(fin, 0, SEEK_END);  //~ So the target file is less than 4GB-bytes-long.
		int lsz = ftell(fin);
		rewind(fin);
		char *buff = (char*)malloc( sizeof(char) * (lsz+1) );
		memset(buff, 0, sizeof(char)*(lsz+1));
		fread(buff, 1, lsz, fin);
		cJSON *root = cJSON_Parse(buff);
		free(buff);
		fclose(fin);
		return root;
	}
	return NULL;
}

// Need free
char* readFileData(const char *path, int *sz){
	FILE *fin = fopen(path, "rb");
	if(!fin){
		printf("cannot open %s\n", path);
		abort();
	}
	fseek(fin, 0, SEEK_END);
	int lsz = ftell(fin);
	rewind(fin);
	char *buff = (char*)malloc(sizeof(char) * lsz);
	memset(buff, 0, sizeof(char) * lsz);
	fread(buff, 1, lsz, fin);
	fclose(fin);
	*sz = lsz;
	return buff;
}


void saveTo(const char *targetPath, const void *data, int sz){
	FILE *fout = fopen(targetPath, "wb");
	if(!fout){
		abort();
	}
	int written = fwrite(data, 1, sz, fout);
	fclose(fout);
}

void saveToJson(const char *targetPath, cJSON *json){
	FILE *fout = fopen(targetPath, "w");
	if (!fout) {
		fprintf(stderr, "Cannot open %s\n", targetPath);
		abort();
	}
	char *verylongstr = cJSON_Print(json);
	fprintf(fout, "%s", verylongstr);
	free(verylongstr);
	fclose(fout);
}

int main(int argc, char **argv) {
	int reverse = 0; // From JSON to MP
	char inPath[2][MAX_PATH+1];
	int off = 0;
	for (int i=1;i<argc;++i) {
		if (!strcmp("-i", argv[i])) {
			reverse = 1;
		} else {
			strcpy(inPath[off++], argv[i]);
			if(2==off)
				break;
		}
	}
	if (off!=2) {
		fprintf(stderr, "Not enough input\n");
		return 1;
	}

	if (!reverse) {
		cJSON *org = readFile(inPath[0]);
		msgpack_object obj = getMsgPackObject(org);
		cJSON_Delete(org);
		msgpack_sbuffer sbuf;
		msgpack_sbuffer_init(&sbuf);
		msgpack_packer pk;
		msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
		msgpack_pack_object(&pk, obj);
		// In case you need to verify.
		//msgpack_zone mempool;
		//msgpack_zone_init(&mempool, 8192 * 4);
		//msgpack_object deserialized;
		//msgpack_unpack(sbuf.data, sbuf.size, NULL, &mempool, &deserialized);
		//printf("Print again:\n");
		//msgpack_object_print(stdout, deserialized);
		saveTo(inPath[1], sbuf.data, sbuf.size);
		destroyMsgPackObject(obj);
		msgpack_sbuffer_destroy(&sbuf);
		printf("done. %s => %s\n", inPath[0], inPath[1]);
	} else {
		int lsz = 0;
		char *buff = readFileData(inPath[0], &lsz);
		msgpack_zone mempool;
		msgpack_zone_init(&mempool, 8192 * 4); //~ Save some trouble enlarging the memory.
		msgpack_object deserialized;
		msgpack_unpack(buff, lsz, NULL, &mempool, &deserialized);
		//~ the `deserialized' refer to `buff':so DO NOT FREE BUFF before discarding deserialized
		cJSON *res = buildFromMsgPack(deserialized);
		free(buff);//
		saveToJson(inPath[1], res);
		cJSON_Delete(res);
		msgpack_zone_destroy(&mempool);
		printf("i-done. %s => %s\n", inPath[0], inPath[1]);
	}
	return 0; // Done
}
