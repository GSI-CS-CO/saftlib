#ifndef BURSTGEN_SHARED_MMAP_H
#define BURSTGEN_SHARED_MMAP_H
//Location of Buildid and Shared Section in LM32 Memory, to be used by host

#define INT_BASE_ADR		0x10000000
#define RAM_SIZE		131072
#define SHARED_SIZE		16K
#define BUILDID_OFFS		0x100
#define SHARED_OFFS		(0x100 + 0x400)
#endif
