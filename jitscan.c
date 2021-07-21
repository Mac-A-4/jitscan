#include <Windows.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "jitscan.h"

#define PATTERN_LENGTH_LIMIT 0xFF

#if defined(_WIN64)

unsigned char ASM_1[] = {
	0x48, 0x31, 0xC0, //xor rax, rax
	0x48, 0x83, 0xEA, 0x00, //sub rdx, PATTERN_LENGTH
	0x0F, 0x82, 0x00, 0x00, 0x00, 0x00, //jc END
	0x48, 0x39, 0xD0, //cmp rax, rdx
	0x0F, 0x8F, 0x00, 0x00, 0x00, 0x00 //jg END
};

#define ASM_1_PATTERN_LENGTH_OFFSET 6
#define ASM_1_END_OFFSET_1 9
#define ASM_1_LOOP_LOCATION 13
#define ASM_1_END_OFFSET_2 18

unsigned char ASM_2[] = {
	0x80, 0x7C, 0x01, 0x00, 0x00, //cmp [rcx+rax+PATTERN_OFFSET], PATTERN_VALUE
	0x0F, 0x85, 0x00, 0x00, 0x00, 0x00 //jne NO_MATCH
};

#define ASM_2_PATTERN_OFFSET_OFFSET 3
#define ASM_2_PATTERN_VALUE_OFFSET 4
#define ASM_2_NO_MATCH_OFFSET 7

unsigned char ASM_3[] = {
	0xC3, //ret
	0x48, 0xFF, 0xC0, //inc rax
	0xE9, 0x00, 0x00, 0x00, 0x00, //jmp LOOP
	0x48, 0xC7, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, //mov rax, -1
	0xC3 //ret
};

#define ASM_3_MATCH_LOCATION 0
#define ASM_3_NO_MATCH_LOCATION 1
#define ASM_3_LOOP_OFFSET 5
#define ASM_3_END_LOCATION 9

typedef long long(*function_t)(const unsigned char*, long long);

#elif defined(_WIN32)

typedef long(*function_t)(const unsigned char*, long);

#endif

#define ASM_1_OFFSET 0
#define ASM_2_OFFSET(i) (ASM_1_OFFSET+sizeof(ASM_1)+sizeof(ASM_2)*i)
#define ASM_3_OFFSET(n) (ASM_2_OFFSET(n))

unsigned char* alloc_create() {
	const int REQUIREMENT = (sizeof(ASM_1) + sizeof(ASM_2) * PATTERN_LENGTH_LIMIT + sizeof(ASM_3));
	return (unsigned char*)VirtualAlloc(NULL, REQUIREMENT, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

void alloc_release(unsigned char* _alloc) {
	VirtualFree(_alloc, NULL, MEM_RELEASE);
}

void alloc_insert(unsigned char* _alloc, int _index, int _size, const void* _source) {
	memcpy(_alloc + _index, _source, _size);
}

void alloc_insert_uint8(unsigned char* _alloc, int _index, unsigned char _value) {
	alloc_insert(_alloc, _index, 1, &_value);
}

void alloc_insert_int32(unsigned char* _alloc, int _index, int _value) {
	alloc_insert(_alloc, _index, 4, &_value);
}

void alloc_insert_asm_1(unsigned char* _alloc, int _real_size, int _pattern_size) {
	alloc_insert(_alloc, ASM_1_OFFSET, sizeof(ASM_1), ASM_1);
	int end_loc = ASM_3_OFFSET(_real_size) + ASM_3_END_LOCATION;
	int d1 = end_loc - (ASM_1_OFFSET + ASM_1_END_OFFSET_1 + 4);
	int d2 = end_loc - (ASM_1_OFFSET + ASM_1_END_OFFSET_2 + 4);
	alloc_insert_int32(_alloc, ASM_1_OFFSET + ASM_1_END_OFFSET_1, d1);
	alloc_insert_int32(_alloc, ASM_1_OFFSET + ASM_1_END_OFFSET_2, d2);
	alloc_insert_uint8(_alloc, ASM_1_OFFSET + ASM_1_PATTERN_LENGTH_OFFSET, (unsigned char)_pattern_size);
}

void alloc_insert_asm_2(unsigned char* _alloc, int _real_size, int _real_index, int _pattern_index, unsigned char _pattern_value) {
	int off = ASM_2_OFFSET(_real_index);
	alloc_insert(_alloc, off, sizeof(ASM_2), ASM_2);
	alloc_insert_uint8(_alloc, off + ASM_2_PATTERN_OFFSET_OFFSET, (unsigned char)_pattern_index);
	alloc_insert_uint8(_alloc, off + ASM_2_PATTERN_VALUE_OFFSET, _pattern_value);
	int nm_off = ASM_3_OFFSET(_real_size) + ASM_3_NO_MATCH_LOCATION;
	int d1 = nm_off - (off + ASM_2_NO_MATCH_OFFSET + 4);
	alloc_insert_int32(_alloc, off + ASM_2_NO_MATCH_OFFSET, d1);
}

void alloc_insert_asm_3(unsigned char* _alloc, int _real_size) {
	int off = ASM_3_OFFSET(_real_size);
	alloc_insert(_alloc, off, sizeof(ASM_3), ASM_3);
	const int loop_loc = ASM_1_OFFSET + ASM_1_LOOP_LOCATION;
	int d1 = loop_loc - (off + ASM_3_LOOP_OFFSET + 4);
	alloc_insert_int32(_alloc, off + ASM_3_LOOP_OFFSET, d1);
}

JITSCAN_API int jitscan_search(void* _jitscan, const unsigned char* _input, int _size) {
	function_t function = (function_t)_jitscan;
	return (int)function(_input, _size);
}

void* jitscan_compile_2(const unsigned char* _values, const unsigned char* _masks, int _size) {
	if (_size > 0xFF) {
		return NULL;
	}
	unsigned char* alloc = alloc_create();
	if (!alloc) {
		return NULL;
	}
	int pattern_size = _size;
	int real_size = 0;
	for (int i = 0; i < pattern_size; ++i) {
		if (_masks[i]) {
			real_size++;
		}
	}
	alloc_insert_asm_1(alloc, real_size, pattern_size);
	int real_index = 0;
	for (int pattern_index = 0; pattern_index < pattern_size; ++pattern_index) {
		if (_masks[pattern_index]) {
			alloc_insert_asm_2(alloc, real_size, real_index, pattern_index, _values[pattern_index]);
			real_index++;
		}
	}
	alloc_insert_asm_3(alloc, real_size);
	return alloc;
}

unsigned char convert(char x) {
	if (x >= '0' && x <= '9') {
		return x - '0';
	}
	else if (x >= 'a' && x <= 'f') {
		return x - 'a' + 0xa;
	}
	else if (x >= 'A' && x <= 'F') {
		return x - 'A' + 0xA;
	}
	else {
		return 0;
	}
}

JITSCAN_API void* jitscan_compile(const char* _input) {
	unsigned char values[0xFF];
	unsigned char masks[0xFF];
	int pattern_size = 0;
	for (;;) {
		while (isspace(*_input)) {
			_input++;
		}
		if (*_input == '\0') {
			break;
		}
		else if (*_input == '?') {
			if (_input[1] == '?') {
				values[pattern_size] = 0;
				masks[pattern_size] = 0;
				pattern_size++;
				_input += 2;
			}
			else {
				return NULL;
			}
		}
		else if (isxdigit(*_input)) {
			if (isxdigit(_input[1])) {
				unsigned char val = ((convert(_input[0]) << 4) | convert(_input[1]));
				values[pattern_size] = val;
				masks[pattern_size] = 1;
				pattern_size++;
				_input += 2;
			}
			else {
				return NULL;
			}
		}
		else {
			return NULL;
		}
	}
	return jitscan_compile_2(values, masks, pattern_size);
}

JITSCAN_API void jitscan_foreach(void* _jitscan, const unsigned char* _input, int _size, jitscan_foreach_t _foreach, void* _value) {
	int ptr = 0;
	int off = jitscan_search(_jitscan, _input, _size);
	while (off != -1) {
		ptr += off;
		_foreach(ptr, _value);
		ptr += 1;
		off = jitscan_search(_jitscan, _input + ptr, _size - ptr);
	}
}

JITSCAN_API void jitscan_release(void* _jitscan) {
	alloc_release(_jitscan);
}