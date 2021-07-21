#pragma once

#ifdef __cplusplus
#define JITSCAN_API extern "C"
#else
#define JITSCAN_API
#endif

typedef void (*jitscan_foreach_t)(int, void*);
JITSCAN_API void jitscan_foreach(void* _jitscan, const unsigned char* _input, int _size, jitscan_foreach_t _foreach, void* _value);
JITSCAN_API int jitscan_search(void* _jitscan, const unsigned char* _input, int _size);
JITSCAN_API void* jitscan_compile(const char* _input);
JITSCAN_API void jitscan_release(void* _jitscan);