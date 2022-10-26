#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "math2d.h"
#include "bitpack.h"

void bitpack_create(BitPack* bp, size_t num_bits)
{
    size_t num_bytes = (size_t)ceil(num_bits/8);
    bp->data = malloc(num_bytes*sizeof(uint8_t));
    assert(bp->data);

    memset(bp->data,0,num_bytes);

    bp->curr_bit_index = 0;
    bp->bits_written = 0;
    bp->size_in_bits = num_bits;
}

void bitpack_clear(BitPack* bp)
{
    int bytes = bp->size_in_bits/8;
    memset(bp->data,0,bytes);
    bp->bits_written = 0;
}

void bitpack_delete(BitPack* bp)
{
    bp->bits_written = 0;
    free(bp->data);
    bp->data = NULL;
}

void bitpack_write(BitPack* bp, uint32_t value, int num_bits)
{
    if(num_bits > 32)
        return;

    uint8_t curr_byte = bp->curr_bit_index / 8;
    uint8_t bit_index = bp->curr_bit_index % 8;

    uint8_t* p = (bp->data+curr_byte);

    int bits_written = 0;

    if(bit_index > 0)
    {
        // write in first byte
        int bits_left_in_byte = bit_index + MIN(num_bits, 8-bit_index);
        for(int i = bit_index; i < bits_left_in_byte; ++i)
        {
            uint8_t bit_val = (uint8_t)((value >> (i-bit_index)) & 0x1);
            *p |= (bit_val << i);
        }
        p++; // advance to next byte
        bits_written += bits_left_in_byte;
    }

    uint32_t* k = (uint32_t*)p;

    // write remaining bits
    for(int i = bits_written; i < num_bits; ++i)
    {
        uint32_t bit_val = ((value >> i) & 0x1);
        *k |= (bit_val << (i-bits_written));
    }

    bp->bits_written += num_bits;
    bp->curr_bit_index += num_bits;
}

void bitpack_seek(BitPack* bp,int bit_index)
{
    bp->curr_bit_index = bit_index;
}

uint32_t bitpack_print_data(BitPack* bp)
{
    printf("BitPack %p (%d bits)\n",bp,bp->bits_written);
    uint32_t val = ((uint32_t*)(bp->data))[0];
    for(int i = bp->bits_written; i >= 0; --i)
        printf("%d",(val >> i) & 0x1);
    printf("\n");
}

uint32_t bitpack_read(BitPack* bp, int num_bits)
{
    if(num_bits > 32)
        return 0;

    uint8_t curr_byte = bp->curr_bit_index / 8;
    uint8_t bit_remainder = bp->curr_bit_index % 8;

    uint32_t* p = (uint32_t*)(bp->data+curr_byte);

    uint32_t ret = 0;

    for(int i = 0; i < num_bits; ++i)
    {
        ret |= ((*p >> (i+bit_remainder)) & 1) << i;
    }
    bp->curr_bit_index += num_bits;

    return ret;
}

void bitpack_test()
{
    BitPack bp;

    bitpack_create(&bp,100);

    bitpack_write(&bp,2048,12);
    bitpack_write(&bp,9,4);
    bitpack_write(&bp,5,3);
    bitpack_write(&bp,1020,10);
    bitpack_write(&bp,1,1);
    bitpack_write(&bp,0,1);
    bitpack_write(&bp,1,1);
    bitpack_print_data(&bp);

    bitpack_seek(&bp,0);
    uint32_t v1 = bitpack_read(&bp,10);
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
