#ifndef _BITMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BITS_TO_BYTES(n)  ( ( ( (n) + 7) & ~7 ) >> 3)
#define BYTES_TO_WORDS(b) ( ( ( (b) + 3 ) & ~3 ) >> 2 )
#define BITS_TO_WORDS(b) BYTES_TO_WORDS( BITS_TO_BYTES(b) )
#define BYTE_TO_WORD(b)  ( ( (b) & ~3) >> 2 )
#define BIT_TO_BYTE(b) ( ( (b) & ~7) >> 3 )
#define BIT_TO_WORD(b) BYTE_TO_WORD(BIT_TO_BYTE(b))
#define __BIT_SET(map, b) ( (map)[ (b) >> 3 ] |= ( 1 << ( (b) & 7 ) ) )
#define __BIT_CLEAR(map, b) ( (map)[ (b) >> 3 ] &= ~( 1 <<  ( (b) & 7 ) ) )
#define __BIT_TEST(map, b) (!! ( (map)[ (b) >> 3 ] & ( 1 << ( (b) & 7 ) ) ) )

typedef struct bitmap
{
    unsigned char *map;
    unsigned int words;
    unsigned int chunk_size;
    unsigned int size; 
    unsigned int chunks;
    unsigned int ckpt_chunks;
    unsigned int last_offset;
    unsigned int last_free_offset;
    unsigned int last_bit_offset;
    unsigned int last_word;
    unsigned int first_free_bit;
    unsigned int first_set_bit;
    unsigned int buffer;
    unsigned int bits; /* total bits operated upon by the bitmap ops*/
} BitmapT;

void bitmap_init(struct bitmap *bitmap, unsigned int size);

void bitmap_grow(struct bitmap *bitmap, unsigned int last_size, unsigned int cur_size);

unsigned int bitmap_free_bit(struct bitmap *bitmap);

void bitmap_buffer_enable(struct bitmap *bitmap);

void bitmap_buffer_disable(struct bitmap *bitmap);

unsigned int next_free_bit(struct bitmap *bitmap);

/*
 * Returns 0 if bit wasn't set, 1 otherwise
 */
int clear_bit(unsigned int bit, struct bitmap *bitmap);

/*
 * Returns 1 if bit was already set, 0 otherwise
 */
int set_bit(unsigned int bit, struct bitmap *bitmap);

unsigned int find_next_bit_slow(struct bitmap *bitmap);

unsigned int __find_next_bit(struct bitmap *bitmap);

unsigned int find_next_bit(struct bitmap *bitmap);

int find_next_bit_walk(struct bitmap *bitmap, void *arg,
                       void (*cb) (struct bitmap *bitmap, unsigned int bit, void *arg));

int __find_next_bit_walk(struct bitmap *bitmap, void *arg,
                         void (*cb) (struct bitmap *bitmap, unsigned int bit, void *arg));

#ifdef __cplusplus
}
#endif

#endif
