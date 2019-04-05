/*
 *  test.c
 *
 *  copyright (c) 2019 Xiongfei Shi
 *
 *  author: Xiongfei Shi <jenson.shixf(a)gmail.com>
 *  license: Apache2.0
 *
 *  https://github.com/shixiongfei/huffman
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.h"

#define BITS2BYTES(b) ((b >> 3) + !!(b & 7))

int main(int argc, char *argv[]) {
  huffman_t * huffman;
  unsigned short hufftable[HUFFMAN_TABLESIZE] = { 0 };
  const char test_string[] = { "This is a test string!!!" };
  unsigned char *encbuf, *decbuf;
  int enclen, declen;
  int encreal, decreal;
  int i;

  printf("test data length: %i\n", (int)strlen(test_string));
  printf("test data: ");
  for (i = 0; i < (int)strlen(test_string); ++i)
    printf("%02x ", (unsigned char)test_string[i]);
  printf("\n");

  huffman_table(hufftable, test_string, (int)strlen(test_string));

  huffman = huffman_create(hufftable);

  enclen = huffman_enclen(huffman, (int)strlen(test_string));
  encbuf = (unsigned char *)malloc(enclen);
  encreal = huffman_encode(huffman, encbuf, test_string, (int)strlen(test_string));

  printf("huffman encode length: %i\n", BITS2BYTES(encreal));

  printf("huffman encode data: ");
  for (i = 0; i < BITS2BYTES(encreal); ++i)
    printf("%02x ", encbuf[i]);
  printf("\n");

  declen = huffman_declen(huffman, encreal);
  decbuf = (unsigned char *)malloc(declen);
  decreal = huffman_decode(huffman, decbuf, encbuf, encreal);

  printf("huffman decode length: %i\n", decreal);

  printf("huffman decode data: ");
  for (i = 0; i < decreal; ++i)
    printf("%02x ", decbuf[i]);
  printf("\n");

  free(encbuf);
  free(decbuf);

  huffman_destroy(huffman);

  return 0;
}
