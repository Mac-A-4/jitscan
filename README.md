# jitscan
Array of bytes (AOB) pattern scanner/compiler.

## Improvements

* Compiling an AOB pattern will only make a maximum of 1 heap allocation.
* Uncompressed source is < 8KB.
* JIT compilation is not dependent on external libraries.

## Limitations

* To use only one heap allocation, pattern sizes were limited to <256 bytes.
* Does not yet have 32 bit support, only 64 bit is implemented.
* Only functional on Windows, due to the hard-coded calling convention.
