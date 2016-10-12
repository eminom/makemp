
#ifndef _MP_MUCHACHO__DEF__
#define _MP_MUCHACHO__DEF__

#include "msgpack.h"
#include "deps/cJSON/cJSON.h"

msgpack_object getMsgPackObject(cJSON *root);
void destroyMsgPackObject(msgpack_object target);

cJSON* buildFromMsgPack(msgpack_object org);

#endif