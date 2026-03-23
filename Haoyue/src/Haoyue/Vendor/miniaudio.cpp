#include "pch.h"

/*
由于 miniaudio.h 包含了大量代码，包含 MINIAUDIO_IMPLEMENTATION 的那个编译单元编译速度会较慢。
建议：单独创建一个 audio_impl.c 文件专门放宏定义和 include，不要在频繁修改的业务代码文件中定义它。
*/

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "miniaudio_engine.h"