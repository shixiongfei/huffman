/*
 *  huffman.h
 *
 *  copyright (c) 2019 Xiongfei Shi
 *
 *  author: Xiongfei Shi <jenson.shixf(a)gmail.com>
 *  license: Apache2.0
 *
 *  https://github.com/shixiongfei/huffman
 */

#ifndef ___HUFFMAN_H__
#define ___HUFFMAN_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUFFMAN_TABLESIZE 256

typedef struct huffman_s huffman_t;

void huffman_setalloc(void * (*alloc)(size_t), void (*release)(void *));
void huffman_table(unsigned short *table, const unsigned char *data, int len);

huffman_t *huffman_create(unsigned short *table);
void huffman_destroy(huffman_t *huffm);

int huffman_rebuild(huffman_t *huffm, unsigned short *table);

int huffman_enclen(huffman_t *huffm, int bytelen);
int huffman_declen(huffman_t *huffm, int bitlen);

int huffman_encode(huffman_t *huffm, void *outbuf, const void *data, int bytelen);
int huffman_decode(huffman_t *huffm, void *outbuf, const void *data, int bitlen);

#ifdef __cplusplus
};
#endif

#endif /* ___HUFFMAN_H__ */
