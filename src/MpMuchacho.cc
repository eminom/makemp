

// Eminem

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "MpMuchacho.h"
#include "msgpack.h"
#include "cJSON.h"

inline void FREE_XPSTR(msgpack_object &t){
	assert( MSGPACK_OBJECT_STR == t.type);
	msgpack_object_str &xstr = t.via.str;
	free((char*)xstr.ptr); // Cast away the const
}

inline void INIT_XPSTR(msgpack_object &t, const char *str) {
	t.type = MSGPACK_OBJECT_STR;
	msgpack_object_str &xstr = t.via.str;
	xstr.size = strlen(str);
	xstr.ptr  = (char*)malloc(sizeof(char)*(xstr.size));
	memcpy((void*)xstr.ptr, str, sizeof(char)*xstr.size);
}

msgpack_object traverseObject(cJSON *root);

msgpack_object traverseArray(cJSON *arr) {
	assert(cJSON_Array == arr->type);
	int l = cJSON_GetArraySize(arr);
	msgpack_object rv;
	rv.type = MSGPACK_OBJECT_ARRAY;
	rv.via.array.size= l;
	rv.via.array.ptr = (msgpack_object*)malloc(sizeof(msgpack_object)*l);
	for(int i=0;i<l;++i){
		msgpack_object &thisOne = *(rv.via.array.ptr + i);
		cJSON *uno = cJSON_GetArrayItem(arr, i);
		switch(uno->type){
		case cJSON_Object:
			thisOne = traverseObject(uno);
			break;
		case cJSON_Array:
			thisOne = traverseArray(uno);
			break;
		case cJSON_NULL:
			thisOne.type = MSGPACK_OBJECT_NIL;
			break;
		case cJSON_False:
		case cJSON_True:
			thisOne.type = MSGPACK_OBJECT_BOOLEAN;
			thisOne.via.boolean = (cJSON_False == uno->type ? false : true);
			break;
		case cJSON_String:
			INIT_XPSTR(thisOne, uno->valuestring);
			break;
		case cJSON_Number:
			thisOne.type = MSGPACK_OBJECT_FLOAT;
			thisOne.via.f64 = uno->valuedouble;
			break;
		default:
			assert(0);
			break;
		}
	}
	return rv;
}

msgpack_object getMsgPackObject(cJSON *root){
	switch(root->type){
	case cJSON_Array:
		return traverseArray(root);
	case cJSON_Object:
		return traverseObject(root);
	default:{
		msgpack_object rv;
		rv.type =  MSGPACK_OBJECT_NIL;
		return rv; //` And the rest need not be initiated.
			}
	}
}

msgpack_object traverseObject(cJSON *root) {
	assert(cJSON_Object == root->type);
	msgpack_object rv;
	rv.type = MSGPACK_OBJECT_MAP;
	msgpack_object_map &map_r = rv.via.map;
	int count = 0;
	for(cJSON *one = root->child;one;one=one->next){
		++count;
	}
	map_r.size = count;
	map_r.ptr = (struct msgpack_object_kv*)malloc(sizeof(msgpack_object_kv) * count);
	int index = 0;
	for (cJSON *one=root->child;one;one=one->next, ++index) {
		msgpack_object_kv& thisOne = *(map_r.ptr+index);
		INIT_XPSTR(thisOne.key, one->string);

		switch(one->type){
		case cJSON_Array:
			thisOne.val = traverseArray(one);
			break;
		case cJSON_Object:
			thisOne.val = traverseObject(one);
			break;
		case cJSON_NULL:
			thisOne.val.type = MSGPACK_OBJECT_NIL;
			break;
		case cJSON_False:
		case cJSON_True:
			thisOne.val.type = MSGPACK_OBJECT_BOOLEAN;
			thisOne.val.via.boolean = (cJSON_False == one->type ? false : true);
			break;
		case cJSON_Number:
			thisOne.val.type = MSGPACK_OBJECT_FLOAT;
			thisOne.val.via.f64 = one->valuedouble;
			break;
		case cJSON_String:
			INIT_XPSTR(thisOne.val, one->valuestring);
			break;
		default:
			assert(0);
			break;
		}
	}
	return rv;
}

void destroyMsgPackObject(msgpack_object target) {
	switch(target.type){
	case MSGPACK_OBJECT_BOOLEAN://Both false & true
	case MSGPACK_OBJECT_FLOAT:
	case MSGPACK_OBJECT_NIL:
		// They do not hold any memory themselves.
		break;
	case MSGPACK_OBJECT_MAP:{
		msgpack_object_map &mp = target.via.map;
		int l = mp.size;
		for(int i=0;i<l;++i){
			msgpack_object_kv &thisOne = *(mp.ptr+i);
			destroyMsgPackObject(thisOne.key);
			destroyMsgPackObject(thisOne.val);
		}
		free(mp.ptr);
		}break;
	case MSGPACK_OBJECT_ARRAY:{
		msgpack_object_array &arr = target.via.array;
		int l = arr.size;
		for(int i=0;i<l;++i){
			msgpack_object &thisOne = *(arr.ptr + i);
			destroyMsgPackObject(thisOne);
		}
		free(arr.ptr);
		}break;
	case MSGPACK_OBJECT_STR:
		free((void*)target.via.str.ptr);
		break;
	}
}



char* _NewStr(msgpack_object_str str){
	int als = sizeof(char)*(str.size+1);
	int cps = sizeof(char)*(str.size);
	char *rv = (char*)malloc(als);
	memset(rv, 0, als);
	memcpy(rv, str.ptr, cps);
	return rv;
}
void _FreeStr(void *ptr){
	free(ptr);
}

//Forward declaration.
cJSON *buildFromMpArr(msgpack_object_array arr);

cJSON *buildFromMpObj(msgpack_object_map obj) {
	cJSON *rv = cJSON_CreateObject();
	int l = obj.size;
	for(int i=0;i<l;++i){
		msgpack_object_kv &thisKV = *(obj.ptr+i);
		msgpack_object &thisOne = thisKV.val;
		cJSON *newEle = NULL;
		switch (thisOne.type) {
		case MSGPACK_OBJECT_NIL:
			newEle = cJSON_CreateNull();
			break;
		case MSGPACK_OBJECT_FLOAT:
			newEle = cJSON_CreateNumber(thisOne.via.f64);
			break;
		case MSGPACK_OBJECT_BOOLEAN:
			newEle = cJSON_CreateBool(thisOne.via.boolean ? 1:0);
			break;
		case MSGPACK_OBJECT_STR:{
			char *ns = _NewStr(thisOne.via.str);
			newEle = cJSON_CreateString(ns);
			_FreeStr(ns);
			}break;
		case MSGPACK_OBJECT_ARRAY:
			newEle = buildFromMpArr(thisOne.via.array);
			break;
		case MSGPACK_OBJECT_MAP:
			newEle = buildFromMpObj(thisOne.via.map);
			break;
		default:
			fprintf(stderr, "Not expected msg-object type:%d\n", thisOne.type);
			abort();
			break;
		}
		char *ns = _NewStr(thisKV.key.via.str);
		cJSON_AddItemToObject(rv, ns, newEle);
		_FreeStr(ns);
	}
	return rv;
}

cJSON *buildFromMpArr(msgpack_object_array arr){
	cJSON *rv = cJSON_CreateArray();
	int l = arr.size;
	for(int i=0;i<l;++i){
		msgpack_object &thisOne = *(arr.ptr+i);
		cJSON *newEle = NULL;
		switch (thisOne.type) {
		case MSGPACK_OBJECT_NIL:
			newEle = cJSON_CreateNull();
			break;
		case MSGPACK_OBJECT_FLOAT:
			newEle = cJSON_CreateNumber(thisOne.via.f64);
			break;
		case MSGPACK_OBJECT_BOOLEAN:
			newEle = cJSON_CreateBool(thisOne.via.boolean ? 1:0);
			break;
		case MSGPACK_OBJECT_STR:{
			char *ns = _NewStr(thisOne.via.str);
			newEle = cJSON_CreateString(ns);
			_FreeStr(ns);
			}break;
		case MSGPACK_OBJECT_ARRAY:
			newEle = buildFromMpArr(thisOne.via.array);
			break;
		case MSGPACK_OBJECT_MAP:
			newEle = buildFromMpObj(thisOne.via.map);
			break;
		default:
			fprintf(stderr, "Not expected msg-object type:%d\n", thisOne.type);
			abort();
			break;
		}
		cJSON_AddItemToArray(rv, newEle);
	}
	int _l = cJSON_GetArraySize(rv);
	return rv;
}

cJSON* buildFromMsgPack(msgpack_object org) {
	switch(org.type){
	case MSGPACK_OBJECT_ARRAY:
		return buildFromMpArr(org.via.array);
		break;
	case MSGPACK_OBJECT_MAP:
		return buildFromMpObj(org.via.map);
	default:
		fprintf(stderr, "Not expected type of msg-pack object:%d\n", org.type);
		abort();
		break;
	}
	abort();
}