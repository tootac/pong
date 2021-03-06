#ifndef __utils_h_

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef s32 bool32;

typedef float real32;
typedef double real64;

#define true 1
#define false 0

#define internal static
#define global_variable static
#define local_persist static

#define ARRAY_COUNT(A) sizeof(A) / sizeof(A[0])

#define THOUSAND 1000.0
#define MILLION  1000000.0
#define BILLION  1000000000.0

#define KILLOBYTE(n) (1024ll * (n))
#define MEGABYTE(n)  (1024ll * KILLOBYTE(n))
#define GIGABYTE(n)  (1024ll * MEGABYTE(n))
#define TERABYTE(n)  (1024ll * GIGABYTE(n))

#define Assert(Expression) if(!(Expression)) {*(volatile int *)0 = 0;}


typedef struct {
    u32 VersionNumber;
    u32 AssetCount;
    u64 Size;
    u8 *Data;
    u8 *AssetSizeTableBase;
    u8 *FileDataBase;
} asset_file;


inline void 
ZeroSize(void *Memory, u64 Size) {
    u8 *Byte = (u8*)Memory; 
    for(u64 Index = 0; Index < Size; Index++) {
        *Byte++ = 0;
    }
}

#define ZERO_STRUCT(A) ZeroSize(&(A), sizeof(A))
#define ZERO_ARRAY(A) ZeroSize(&(A), (sizeof(A)))


#define __utils_h_
#endif

