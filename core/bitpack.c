#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "math2d.h"
#include "bitpack.h"

#define CPU_LITTLE_ENDIAN 1
#define CPU_BIG_ENDIAN 2

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define CPU_ENDIAN CPU_LITTLE_ENDIAN
#else
  #define CPU_ENDIAN CPU_BIG_ENDIAN
#endif

static inline uint32_t conv_endian32(uint32_t value)
{
#if CPU_ENDIAN == CPU_BIG_ENDIAN
    return __builtin_bswap32( value );
#else
    return value;
#endif
}

void bitpack_create(BitPack* bp, size_t num_bytes)
{
    // round num_bytes up to multiple of 4
    num_bytes += 4-(num_bytes % 4);
    printf("num_bytes: %d\n",num_bytes);

    bp->data = malloc(num_bytes*sizeof(uint8_t));
    assert(bp->data);
    memset(bp->data,0,num_bytes);

    bp->size_in_bits = 8*num_bytes;
    bp->size_in_words = num_bytes/4;
    bp->bits_written = 0;
    bp->word_index = 0;
    bp->words_written = 0;
    bp->scratch = 0;
    bp->bit_index = 0;
    bp->overflow = false;
}

void bitpack_clear(BitPack* bp)
{
    memset(bp->data,0,4*bp->size_in_words);
    bp->bits_written = 0;
    bp->word_index = 0;
    bp->words_written = 0;
    bp->overflow = false;
}

void bitpack_delete(BitPack* bp)
{
    bp->bits_written = 0;
    bp->word_index = 0;
    bp->words_written = 0;
    bp->overflow = false;
    free(bp->data);
    bp->data = NULL;
}

void bitpack_seek_begin(BitPack* bp)
{
    bp->bit_index = 0;
    bp->word_index = 0;
}

void bitpack_seek_to_written(BitPack* bp)
{
    bp->bit_index = bp->bits_written;
    bp->word_index = bp->words_written;
}

void bitpack_write(BitPack* bp, uint32_t value, int num_bits)
{
    if(!bp->data)
        return;

    if(num_bits < 0 || num_bits > 32)
        return;

    value &= ((uint64_t)1 << num_bits) -1;

    bp->scratch |= ((uint64_t)value) << ( 64 - bp->bit_index - num_bits);
    bp->bit_index += num_bits;

    if(bp->bit_index >= 32)
    {
        bp->data[bp->word_index] = conv_endian32((uint32_t)(bp->scratch >> 32));
        bp->word_index++;
        bp->words_written++;
        bp->scratch <<= 32;
        bp->bit_index -= 32;
    }
    bp->bits_written += num_bits;
}

void bitpack_flush_bits(BitPack* bp)
{
    if(bp->bit_index != 0)
    {
        assert(bp->word_index < bp->size_in_words);
        if (bp->word_index >= bp->size_in_words )
        {
            bp->overflow = true;
            return;
        }
        bp->data[bp->word_index] = conv_endian32((uint32_t)(bp->scratch >> 32));

        bp->word_index++;
        bp->words_written++;

    }
}

uint32_t bitpack_print_data(BitPack* bp)
{
    printf("BitPack %p (%d bits)\n",bp,bp->bits_written);
    for(int i = 0; i < bp->words_written; ++i)
    {
        uint32_t val = bp->data[i];
        for(int j = 31; j >= 0; --j)
            printf("%d",(val >> j) & 0x1);

        printf(" ");
    }
    printf("\n");
}

uint32_t bitpack_read(BitPack* bp, int num_bits)
{
    if(!bp->data)
        return 0;

    if(num_bits < 0 || num_bits > 32)
        return 0;

    // @TODO

    return 0;
}

void bitpack_test()
{
    BitPack bp;

    bitpack_create(&bp,8);

    bitpack_write(&bp,2048,12);
    bitpack_write(&bp,9,4);
    bitpack_write(&bp,5,3);
    bitpack_write(&bp,1020,10);
    bitpack_write(&bp,1,1);
    bitpack_write(&bp,0,1);
    bitpack_write(&bp,1,1);
    bitpack_flush_bits(&bp);
    bitpack_print_data(&bp);

    bitpack_seek_begin(&bp);
    uint32_t v1 = bitpack_read(&bp,12);
    uint32_t v2 = bitpack_read(&bp,4);
    uint32_t v3 = bitpack_read(&bp,3);
    uint32_t v4 = bitpack_read(&bp,10);
    uint32_t v5 = bitpack_read(&bp,1);
    uint32_t v6 = bitpack_read(&bp,1);
    uint32_t v7 = bitpack_read(&bp,1);

    printf("v1: %u\n",v1);
    printf("v2: %u\n",v2);
    printf("v3: %u\n",v3);
    printf("v4: %u\n",v4);
    printf("v5: %u\n",v5);
    printf("v6: %u\n",v6);
    printf("v7: %u\n",v7);

}
