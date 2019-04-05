/*
 *  huffman.c
 *
 *  copyright (c) 2019 Xiongfei Shi
 *
 *  author: Xiongfei Shi <jenson.shixf(a)gmail.com>
 *  license: Apache2.0
 *
 *  https://github.com/shixiongfei/huffman
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define HUFFMAN_POOLSIZE 512

typedef struct huffnode_s huffnode_t;
typedef struct huffcode_s huffcode_t;
typedef struct huffcontext_s huffcontext_t;

struct huffnode_s {
  int weight;
  unsigned char symbol;
  huffnode_t *left, *right;
};

struct huffcode_s {
  unsigned short bitlen;
  unsigned short code;
};

struct huffcontext_s {
  huffnode_t pool[HUFFMAN_POOLSIZE];
  huffcode_t codes[HUFFMAN_TABLESIZE];
  huffnode_t *queue[HUFFMAN_TABLESIZE];
  huffnode_t **pq; /* priority queue */
  int nodes, qend;
  int maxbits, minbits;
};

struct huffman_s {
  huffcontext_t context;
  unsigned short table[HUFFMAN_TABLESIZE];
};

static struct huffalloc_s {
  void * (*alloc)(size_t);
  void (*release)(void *);
} huffalloc = { malloc, free };

void huffman_setalloc(void * (*alloc)(size_t), void(*release)(void *)) {
  huffalloc.alloc = alloc ? alloc : malloc;
  huffalloc.release = release ? release : free;
}

static huffnode_t *huffnode_new(huffman_t *huffm, int weight,
                                unsigned char symbol, huffnode_t *l,
                                huffnode_t *r) {
  huffnode_t *n = huffm->context.pool + huffm->context.nodes++;

  if (weight) {
    n->symbol = symbol;
    n->weight = weight;

    n->left = NULL;
    n->right = NULL;
  } else {
    n->left = l;
    n->right = r;

    n->weight = l->weight + r->weight;
    n->symbol = 0;
  }
  return n;
}

static void huffnode_push(huffman_t *huffm, huffnode_t *node) {
  int j, i = huffm->context.qend++;

  while ((j = i / 2) && huffm->context.pq[j]->weight > node->weight) {
    huffm->context.pq[i] = huffm->context.pq[j];
    i = j;
  }
  huffm->context.pq[i] = node;
}

static huffnode_t *huffnode_pop(huffman_t *huffm) {
  int i, l;
  huffnode_t *n = huffm->context.pq[i = 1];

  if (huffm->context.qend < 2)
    return NULL;

  huffm->context.qend -= 1;

  while ((l = i * 2) < huffm->context.qend) {
    if ((l + 1) < huffm->context.qend &&
        huffm->context.pq[l + 1]->weight < huffm->context.pq[l]->weight)
      l += 1;

    if (huffm->context.pq[huffm->context.qend]->weight <=
        huffm->context.pq[l]->weight)
      break;

    huffm->context.pq[i] = huffm->context.pq[l];
    i = l;
  }

  huffm->context.pq[i] = huffm->context.pq[huffm->context.qend];
  return n;
}

static void bitstream_writebit(unsigned char **bs, int bit, int *bit_pos) {
  int bit_offset, byte_offset;
  unsigned char *p, stamp;

  bit_offset = *bit_pos & 7;
  byte_offset = (*bit_pos - bit_offset) / 8;

  p = *bs + byte_offset;

  stamp = (0 != bit);
  stamp <<= 8 - 1;
  stamp >>= bit_offset;
  *p &= ~(0x80 >> bit_offset); /* clear bit */
  *p |= stamp;

  *bit_pos += 1;

  if (*bit_pos >= 8) {
    *bit_pos -= 8;
    *bs += 1;
  }
}

static int bitstream_readbit(unsigned char **bs, int *bit_pos) {
  int bit_offset, byte_offset;
  unsigned char *p, val;

  bit_offset = *bit_pos & 7;
  byte_offset = (*bit_pos - bit_offset) / 8;

  p = *bs + byte_offset;

  val = *p;

  val <<= bit_offset;
  val >>= 8 - 1;

  *bit_pos += 1;

  if (*bit_pos >= 8) {
    *bit_pos -= 8;
    *bs += 1;
  }

  return val == 1;
}

#define putbit(c, b, l)                                                        \
  do {                                                                         \
    unsigned char *bs = (unsigned char *)&(c);                                 \
    int bs_off = (l);                                                          \
    bitstream_writebit(&bs, (b), &bs_off);                                     \
  } while (0)

static void huffnode_build_codes(huffman_t *huffm, huffnode_t *node,
                                 unsigned short code, unsigned short bitlen) {
  if (node->symbol || (!node->left && !node->right)) {
    huffm->context.codes[node->symbol].bitlen = bitlen;
    huffm->context.codes[node->symbol].code = code;

    huffm->context.maxbits = max(huffm->context.maxbits, bitlen);
    huffm->context.minbits = min(huffm->context.minbits, bitlen);
    return;
  }

  putbit(code, 0, bitlen);
  huffnode_build_codes(huffm, node->left, code, bitlen + 1);

  putbit(code, 1, bitlen);
  huffnode_build_codes(huffm, node->right, code, bitlen + 1);
}

static int huffman_rebuild_all(huffman_t *huffm) {
  huffnode_t *n;
  int i;

  memset(&huffm->context, 0, sizeof(huffcontext_t));

  huffm->context.pq = huffm->context.queue - 1;
  huffm->context.qend = 1;
  huffm->context.maxbits = 0;
  huffm->context.minbits = 16;

  for (i = 0; i < HUFFMAN_TABLESIZE; ++i) {
    if (huffm->table[i] < 1)
      huffm->table[i] = 1;

    n = huffnode_new(huffm, huffm->table[i], i, NULL, NULL);
    huffnode_push(huffm, n);
  }

  while (huffm->context.qend > 2) {
    n = huffnode_new(huffm, 0, 0, huffnode_pop(huffm), huffnode_pop(huffm));
    huffnode_push(huffm, n);
  }

  huffnode_build_codes(huffm, huffm->context.pq[1], 0, 0);

  return 0;
}

void huffman_table(unsigned short *table, const unsigned char *data, int len) {
  int i, j, overflow = 0;

  for (i = 0; i < len; ++i) {
    if (USHRT_MAX == table[data[i]]) {
      overflow = 1;
      break;
    } else
      table[data[i]] += 1;
  }

  if (overflow)
    for (j = 0; j < i; ++j)
      table[data[j]] -= 1; /* rollback */
}

huffman_t *huffman_create(unsigned short *table) {
  huffman_t *huffm = (huffman_t *)huffalloc.alloc(sizeof(huffman_t));

  if (!huffm)
    return NULL;

  if (0 != huffman_rebuild(huffm, table)) {
    huffalloc.release(huffm);
    return NULL;
  }
  return huffm;
}

void huffman_destroy(huffman_t *huffm) {
  if (!huffm)
    return;
  huffalloc.release(huffm);
}

int huffman_rebuild(huffman_t *huffm, unsigned short *table) {
  if (table)
    memcpy(huffm->table, table, sizeof(huffm->table));
  else
    memset(huffm->table, 0, sizeof(huffm->table));

  return huffman_rebuild_all(huffm);
}

int huffman_enclen(huffman_t *huffm, int bytelen) {
  return bytelen * huffm->context.maxbits / 8 + 1;
}

int huffman_declen(huffman_t *huffm, int bitlen) {
  return bitlen / huffm->context.minbits;
}

int huffman_encode(huffman_t *huffm, void *outbuf, const void *data, int bytelen) {
  const unsigned char *p = (const unsigned char *)data;
  unsigned char *rbs, *wbs = (unsigned char *)outbuf;
  int i, j, b, roff, woff = 0;
  int bitlen = 0;

  for (i = 0; i < bytelen; ++i) {
    if (huffm->context.codes[p[i]].bitlen <= 0) {
      return -1;
    }

    rbs = (unsigned char *)&huffm->context.codes[p[i]].code;
    roff = 0;

    for (j = 0; j < huffm->context.codes[p[i]].bitlen; ++j) {
      b = bitstream_readbit(&rbs, &roff);
      bitstream_writebit(&wbs, b, &woff);

      bitlen += 1;
    }
  }

  return bitlen;
}

int huffman_decode(huffman_t *huffm, void *outbuf, const void *data, int bitlen) {
  huffnode_t *node = huffm->context.pq[1];
  unsigned char *bs = (unsigned char *)data;
  unsigned char *decode = (unsigned char *)outbuf;
  int i, b, bs_off = 0;
  int bytelen = 0;

  for (i = 0; i < bitlen; ++i) {
    b = bitstream_readbit(&bs, &bs_off);

    if (0 == b)
      node = node->left;
    else
      node = node->right;

    if (!node)
      return -1;

    if (node->symbol || (!node->left && !node->right)) {
      decode[bytelen++] = node->symbol;
      node = huffm->context.pq[1];
    }
  }

  if (node != huffm->context.pq[1])
    return -1;

  return bytelen;
}
