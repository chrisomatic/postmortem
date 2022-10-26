#pragma once

typedef struct
{
    int size_in_bits;
    int bits_written;
    int curr_bit_index;
    uint8_t* data;
} BitPack;

void bitpack_create(BitPack* bp, size_t num_bits);
void bitpack_clear(BitPack* bp);
void bitpack_delete(BitPack* bp);
void bitpack_seek(BitPack* bp,int bit_index);
void bitpack_write(BitPack* bp, uint32_t value, int num_bits);
uint32_t bitpack_read(BitPack* bp, int num_bits);
void bitpack_test();
