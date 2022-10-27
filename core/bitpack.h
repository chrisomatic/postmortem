#pragma once

typedef struct
{
    uint32_t* data;
    uint64_t scratch;
    bool overflow;

    int size_in_bits;
    int bits_written;
    int bit_index;

    int size_in_words;
    int words_written;
    int word_index;
} BitPack;

void bitpack_create(BitPack* bp, size_t num_bytes);
void bitpack_clear(BitPack* bp);
void bitpack_delete(BitPack* bp);
void bitpack_seek_begin(BitPack* bp);
void bitpack_seek_to_written(BitPack* bp);
void bitpack_write(BitPack* bp, uint32_t value, int num_bits);
uint32_t bitpack_read(BitPack* bp, int num_bits);
void bitpack_test();
