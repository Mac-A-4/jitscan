#pragma once
#include "jitscan.h"
#include <functional>
#include <vector>

class jitscan {
	
	private:

		void* m_jitscan;

		static void static_foreach(int _off, void* _val) {
			std::function<void(int)>* val = (std::function<void(int)>*)_val;
			(*val)(_off);
		}

	public:

		jitscan(const char* _input) {
			m_jitscan = jitscan_compile(_input);
		}

		~jitscan() {
			jitscan_release(m_jitscan);
		}

		jitscan(const jitscan&) = delete;
		jitscan& operator=(const jitscan&) = delete;

		int search(const unsigned char* _input, int _size) const {
			return jitscan_search(m_jitscan, _input, _size);
		}

		void foreach(const unsigned char* _input, int _size, std::function<void(int)> _foreach) {
			jitscan_foreach(m_jitscan, _input, _size, static_foreach, &_foreach);
		}

};