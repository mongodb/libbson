# Endianness Concerns

The BSON specification dictates that the encoding format is in little-endian.
Many implementations imply ignore endianess altogether and expect that they are to be run on little-endian.
Libbson, tries to handle the system endianness as well as be careful with alignment of values to avoid those pesky `Bus Error`'s.
This means we use memcpy() where appropriate instead of dereferencing and expect the compiler intrinstics to optimize it to a dereference when possible.

However, a few issues remain.
No checks are in place for double values.
Therefore, they are always encoded in the system format.
This may be an issue, but warrants further investigation.
