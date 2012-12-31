#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bitmap.h"

#define unlikely(expr) __builtin_expect(!!(expr), 0 )
#define likely(expr) __builtin_expect(!!(expr), 1)

#define BITMAP_BITS_DEFAULT (1<<12)
#define BITMAP_SIZE_DEFAULT BITS_TO_BYTES(BITMAP_BITS_DEFAULT)

static unsigned char free_offset_map[65536] = {

#define B0(n) 0, n+2
#define B1(n) 0, 1, B0(n)
#define B2(n) B1(n), B1(n+1)
#define B4(n) B2(n), B1(n), B1(n+2)
#define B8(n) B4(n), B2(n), B1(n), B1(n+3)
#define B16(n) B8(n), B4(n), B2(n), B1(n), B1(n+4)
#define B32(n) B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+5)
#define B64(n) B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+6)
#define B128(n) B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+7)
#define B256(n) B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+8)
#define B512(n) B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+9)
#define B1024(n) B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+10)
#define B2048(n) B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+11)
#define B4096(n) B2048(n), B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+12)
#define B8192(n) B4096(n), B2048(n), B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+13)
#define B16384(n) B8192(n), B4096(n), B2048(n), B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+14)

    B16384(0),

#undef B0
#undef B1
#undef B2
#undef B4
#undef B8
#undef B16
#undef B32
#undef B64
#undef B128
#undef B256
#undef B512
#undef B1024
#undef B2048
#undef B4096
#undef B8192
#undef B16384

};

static unsigned char set_offset_map[65536 + 4] = {

#define B0(n) n+2, 0
#define B1(n) B0(n), 1, 0
#define B2(n) B1(n), B1(n+1)
#define B4(n) B2(n), B1(n), B1(n+2)
#define B8(n) B4(n), B2(n), B1(n), B1(n+3)
#define B16(n) B8(n), B4(n), B2(n), B1(n), B1(n+4)
#define B32(n) B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+5)
#define B64(n) B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+6)
#define B128(n) B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+7)
#define B256(n) B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+8)
#define B512(n) B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+9)
#define B1024(n) B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+10)
#define B2048(n) B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+11)
#define B4096(n) B2048(n), B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+12)
#define B8192(n) B4096(n), B2048(n), B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+13)
#define B16384(n) B8192(n), B4096(n), B2048(n), B1024(n), B512(n), B256(n), B128(n), B64(n), B32(n), B16(n), B8(n), B4(n), B2(n), B1(n), B1(n+14)

        0, 0, 1, 0, B16384(0),
};

void bitmap_init(struct bitmap *bitmap, unsigned int size)
{
    if(!size) size = BITMAP_SIZE_DEFAULT;
    else size = (size + 3) & ~3; /*align to 4 byte word */

    if(bitmap->map) free(bitmap->map);
    bitmap->map = calloc(1, size);
    assert(bitmap->map != NULL);
    bitmap->size = bitmap->chunk_size = size;
    bitmap->words = BYTES_TO_WORDS(size);
    bitmap->chunks = 1;
    bitmap->ckpt_chunks = 0;
    bitmap->first_free_bit = ~0U;
    bitmap->first_set_bit = ~0U;
    bitmap->last_free_offset = 0;
    bitmap->last_offset = 0;
    bitmap->last_word = 0;
    bitmap->last_bit_offset = 0;
    bitmap->bits = 0;
}

void bitmap_grow(struct bitmap *bitmap, unsigned int last_size, unsigned int cur_size)
{
    if(!last_size)
        last_size = bitmap->size;

    if(!cur_size)
        cur_size = last_size + bitmap->chunk_size;
    else if(cur_size <= last_size) return;
    
    bitmap->chunks += (cur_size - last_size)/bitmap->chunk_size;
    bitmap->map = realloc(bitmap->map,
                          bitmap->chunks * bitmap->chunk_size);
    assert(bitmap->map != NULL);
    memset(bitmap->map + bitmap->size, 0, cur_size - last_size);
    bitmap->size += (cur_size - last_size);
    bitmap->words = BYTES_TO_WORDS(bitmap->size);
}

unsigned int bitmap_free_bit(struct bitmap *bitmap)
{
    int bit;
    bit = next_free_bit(bitmap);
    if((int)bit < 0)
    {
        unsigned int last_bitmap_size = bitmap->size;
        bitmap_grow(bitmap, last_bitmap_size, 0);
        bit = last_bitmap_size * 8;
        bitmap->map[last_bitmap_size] |= 1;
    }
    else
    {
        set_bit(bit, bitmap);
    }
    ++bitmap->bits;
    return bit;
}

void bitmap_buffer_enable(struct bitmap *bitmap)
{
    unsigned int cur_size = bitmap->size;
    unsigned int extend_size = cur_size + bitmap->chunk_size*2;
    bitmap_grow(bitmap, cur_size, extend_size);
    bitmap->last_free_offset = BYTES_TO_WORDS(cur_size + bitmap->chunk_size);
    bitmap->buffer = 1;
}

void bitmap_buffer_disable(struct bitmap *bitmap)
{
    bitmap->buffer = 0;
    bitmap->last_free_offset = 0;
    bitmap->first_free_bit = ~0U;
}

unsigned int next_free_bit(struct bitmap *bitmap)
{
    unsigned int i;
    unsigned int start_offset = bitmap->last_free_offset;
    unsigned int *map = (unsigned int*)bitmap->map;

    for(i = start_offset; i < bitmap->words; ++i)
    {
        unsigned int offset = 0;
        unsigned int word = map[i];
        if(word == 0xffffffffU) continue;
        if( (word & 0xffffU) == 0xffffU )
        {
            offset = 16;
            word >>= 16;
        }
        bitmap->last_free_offset = i;
        offset += free_offset_map[word & 0xffff];
        return (i << 5) + offset;
    }
    return (unsigned int)-1;
}

/*
 * Returns 0 if bit wasn't set, 1 otherwise
 */
int clear_bit(unsigned int bit, struct bitmap *bitmap)
{
    if(bit >= (bitmap->words << 5))
        return 0;
    if(!__BIT_TEST(bitmap->map, bit))
        return 0;
    __BIT_CLEAR(bitmap->map, bit);
    if(bit < bitmap->first_free_bit)
    {
        bitmap->first_free_bit = bit;
        if(likely(bitmap->buffer == 0))
            bitmap->last_free_offset = BIT_TO_WORD(bit);
    }
    return 1;
}

/*
 * Returns 1 if bit was already set, 0 otherwise
 */
int set_bit(unsigned int bit, struct bitmap *bitmap)
{
    if(bit >= (bitmap->words << 5))
        return 0;
    if(__BIT_TEST(bitmap->map, bit))
        return 1;
    __BIT_SET(bitmap->map, bit);
    if(bit < bitmap->first_set_bit)
    {
        bitmap->first_set_bit = bit;
        bitmap->last_offset = BIT_TO_WORD(bit);
        bitmap->last_bit_offset = bit & 31;
    }
    return 0;
}

unsigned int find_next_bit_slow(struct bitmap *bitmap)
{
    unsigned int *map = (unsigned int*)bitmap->map;
    unsigned int start_offset = bitmap->last_offset;
    unsigned int last_bit_offset = bitmap->last_bit_offset;
    unsigned int offset = 0;
    unsigned int i;
    for(i = start_offset, ++last_bit_offset; i < bitmap->words; ++i, last_bit_offset = 0)
    {
        unsigned int w = map[i];
        unsigned int bit;
        if(last_bit_offset >= 32) continue;
        if(!w) continue;
        if( !( w & 0xffff ) )
        {
            offset = 16;
            w >>= 16;
        }
        for(bit = last_bit_offset; bit < 32; ++bit)
        {
            if(w & (1 << bit)) 
            {
                offset += bit;
                break;
            }
        }
        bitmap->last_offset = i;
        bitmap->last_bit_offset = offset;
        return (i << 5) + offset;
    }
    return (unsigned int)-1;
}

/*
 * This fast version of the next bit walk needs to be used with a clear of the last bit
 * In short -- it affects the input bitmap as caller needs to clear the last set bit before 
 * the call to __next_bit
 */
unsigned int __find_next_bit(struct bitmap *bitmap)
{
    unsigned int *map = (unsigned int*)bitmap->map;
    unsigned int start_offset = bitmap->last_offset;
    unsigned int offset = 0;
    unsigned int i;
    for(i = start_offset; i < bitmap->words; ++i)
    {
        unsigned int w = map[i];
        if(!w) continue;
        if( !( w & 0xffff ) )
        {
            offset += 16;
            w >>= 16;
        }
        offset += set_offset_map[ w & 0xffff ];
        bitmap->last_offset = i;
        return (i << 5) + offset;
    }
    return (unsigned int)-1;
}

unsigned int find_next_bit(struct bitmap *bitmap)
{
    unsigned int *map = (unsigned int*)bitmap->map;
    unsigned int start_offset = bitmap->last_offset;
    unsigned int last_bit = ((int)bitmap->last_bit_offset >= 0) ? 
        (start_offset << 5) + bitmap->last_bit_offset : -1;
    unsigned int offset = 0, bit = 0;
    unsigned int i;
    for(i = start_offset; i < bitmap->words; ++i)
    {
        unsigned int w = map[i];
        if(!w) continue;
        if( !( w & 0xffff ) )
        {
            offset = 16;
            w >>= 16;
        }
        offset += set_offset_map[ w & 0xffff ];
        bit = (i << 5) + offset;
        if(likely(bit == last_bit) )
        {
            /*
             * Clear the last bit, re-scan and restore
             */
            if(!bitmap->last_word)
                bitmap->last_word = w;
            map[i] &= ~( 1 << (offset & 31) );
            w = map[i];
            offset = 0;
            if(!w) continue;
            if(!(w & 0xffff)) 
            {
                offset = 16;
                w >>= 16;
            }
            offset += set_offset_map[w & 0xffff];
            bit = (i << 5) + offset;
        }
        /*
         * restore last word
         */
        if( (start_offset != i) && bitmap->last_word)
        {
            map[start_offset] = bitmap->last_word;
            bitmap->last_word = 0;
        }
        bitmap->last_offset = i;
        bitmap->last_bit_offset = offset;
        return bit;
    }
    if(bitmap->last_word)
    {
        map[start_offset] = bitmap->last_word;
        bitmap->last_word = 0;
    }
    return (unsigned int)-1;
}

int find_next_bit_walk(struct bitmap *bitmap, void *arg,
                       void (*cb) (struct bitmap *bitmap, unsigned int bit, void *arg))
{
    unsigned int bit = 0;
    if(!bitmap) return -1;
    if(unlikely(!bitmap->map)) return -1;
    bitmap->last_offset = 0;
    bitmap->last_bit_offset = -1;
    bitmap->last_word = 0;
    while ( (bit = find_next_bit(bitmap) ) != -1 )
    {
        if(cb) cb(bitmap, bit, arg);
        else
            printf("Bit set at offset [%u]\n", bit);
        ++bitmap->bits;
    }
    //    printf("Found a total of [%u] set bits\n", bitmap->bits);
    bitmap->last_offset = 0;
    bitmap->last_bit_offset = -1;
    bitmap->last_word = 0;
    return 0;
}

int __find_next_bit_walk(struct bitmap *bitmap, void *arg,
                         void (*cb) (struct bitmap *bitmap, unsigned int bit, void *arg))
{
    unsigned int bit = 0;
    if(!bitmap) return -1;
    if(unlikely(!bitmap->map)) return -1;
    bitmap->last_offset = 0;
    while ( (bit = __find_next_bit(bitmap) ) != -1 )
    {
        if(cb) cb(bitmap, bit, arg);
        else
            printf("Bit set at offset [%u]\n", bit);
        ((unsigned int *)bitmap->map )[ bit >> 5 ] &= ~(1 << ( bit & 31) );
        ++bitmap->bits;
    }
    //    printf("Found a total of [%u] set bits\n", bitmap->bits);
    bitmap->last_offset = 0;
    return 0;
}
