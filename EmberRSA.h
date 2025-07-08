/*
* Copyright (c) 2016 Thomas Pornin
* SPDX-License-Identifier: MIT
*
* This file contains a unified version of BearSSL RSA code
* originally written by Thomas Pornin (https://bearssl.org/).
*
* BearSSL is licensed under the MIT License:
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef EMBER_RSA_H
#define EMBER_RSA_H

#include <BearSSLHelpers.h>

namespace EmberIotRSA
{

#define BR_MAX_RSA_SIZE   4096

/*
 * Maximum size for a RSA factor (in bits). This is for RSA private-key
 * operations. Default is to support factors up to a bit more than half
 * the maximum modulus size.
 *
 * This value MUST be a multiple of 32.
 */
#define BR_MAX_RSA_FACTOR   ((BR_MAX_RSA_SIZE + 64) >> 1)

#define U      (2 + ((BR_MAX_RSA_FACTOR + 30) / 31))
#define TLEN   (8 * U)

/*
 * Negate a boolean.
 */
static inline uint32_t
NOT(uint32_t ctl)
{
	return ctl ^ 1;
}

/*
 * Equality check: returns 1 if x == y, 0 otherwise.
 */
static inline uint32_t
EQ(uint32_t x, uint32_t y)
{
	uint32_t q;

	q = x ^ y;
	return NOT((q | -q) >> 31);
}

/*
 * Multiplexer: returns x if ctl == 1, y if ctl == 0.
 */
static inline uint32_t
MUX(uint32_t ctl, uint32_t x, uint32_t y)
{
	return y ^ (-ctl & (x ^ y));
}

/*
 * Inequality check: returns 1 if x != y, 0 otherwise.
 */
static inline uint32_t
NEQ(uint32_t x, uint32_t y)
{
	uint32_t q;

	q = x ^ y;
	return (q | -q) >> 31;
}

/*
 * Comparison: returns 1 if x > y, 0 otherwise.
 */
static inline uint32_t
GT(uint32_t x, uint32_t y)
{
	/*
	 * If both x < 2^31 and x < 2^31, then y-x will have its high
	 * bit set if x > y, cleared otherwise.
	 *
	 * If either x >= 2^31 or y >= 2^31 (but not both), then the
	 * result is the high bit of x.
	 *
	 * If both x >= 2^31 and y >= 2^31, then we can virtually
	 * subtract 2^31 from both, and we are back to the first case.
	 * Since (y-2^31)-(x-2^31) = y-x, the subtraction is already
	 * fine.
	 */
	uint32_t z;

	z = y - x;
	return (z ^ ((x ^ y) & (x ^ z))) >> 31;
}

/*
 * Compute the bit length of a 32-bit integer. Returned value is between 0
 * and 32 (inclusive).
 */
static inline uint32_t
BIT_LENGTH(uint32_t x)
{
	uint32_t k, c;

	k = NEQ(x, 0);
	c = GT(x, 0xFFFF); x = MUX(c, x >> 16, x); k += c << 4;
	c = GT(x, 0x00FF); x = MUX(c, x >>  8, x); k += c << 3;
	c = GT(x, 0x000F); x = MUX(c, x >>  4, x); k += c << 2;
	c = GT(x, 0x0003); x = MUX(c, x >>  2, x); k += c << 1;
	k += GT(x, 0x0001);
	return k;
}

/* see inner.h */
uint32_t
br_i31_bit_length(uint32_t *x, size_t xlen)
{
	uint32_t tw, twk;

	tw = 0;
	twk = 0;
	while (xlen -- > 0) {
		uint32_t w, c;

		c = EQ(tw, 0);
		w = x[xlen];
		tw = MUX(c, w, tw);
		twk = MUX(c, (uint32_t)xlen, twk);
	}
	return (twk << 5) + BIT_LENGTH(tw);
}

/* see inner.h */
void
br_i31_decode(uint32_t *x, const void *src, size_t len)
{
	const unsigned char *buf;
	size_t u, v;
	uint32_t acc;
	int acc_len;

	buf = (const unsigned char*) src;
	u = len;
	v = 1;
	acc = 0;
	acc_len = 0;
	while (u -- > 0) {
		uint32_t b;

		b = buf[u];
		acc |= (b << acc_len);
		acc_len += 8;
		if (acc_len >= 31) {
			x[v ++] = acc & (uint32_t)0x7FFFFFFF;
			acc_len -= 31;
			acc = b >> (8 - acc_len);
		}
	}
	if (acc_len != 0) {
		x[v ++] = acc;
	}
	x[0] = br_i31_bit_length(x + 1, v - 1);
}

/*
 * Zeroize an integer. The announced bit length is set to the provided
 * value, and the corresponding words are set to 0. The ENCODED bit length
 * is expected here.
 */
static inline void
br_i31_zero(uint32_t *x, uint32_t bit_len)
{
	*x ++ = bit_len;
	memset(x, 0, ((bit_len + 31) >> 5) * sizeof *x);
}

/*
 * Multiply two 31-bit integers, with a 62-bit result. This default
 * implementation assumes that the basic multiplication operator
 * yields constant-time code.
 * The MUL31_lo() macro returns only the low 31 bits of the product.
 */
#define MUL31(x, y)     ((uint64_t)(x) * (uint64_t)(y))
#define MUL31_lo(x, y)  (((uint32_t)(x) * (uint32_t)(y)) & (uint32_t)0x7FFFFFFF)

/* see inner.h */
void
br_i31_mulacc(uint32_t *d, const uint32_t *a, const uint32_t *b)
{
	size_t alen, blen, u;
	uint32_t dl, dh;

	alen = (a[0] + 31) >> 5;
	blen = (b[0] + 31) >> 5;

	/*
	 * We want to add the two bit lengths, but these are encoded,
	 * which requires some extra care.
	 */
	dl = (a[0] & 31) + (b[0] & 31);
	dh = (a[0] >> 5) + (b[0] >> 5);
	d[0] = (dh << 5) + dl + (~(uint32_t)(dl - 31) >> 31);

	for (u = 0; u < blen; u ++) {
		uint32_t f;
		size_t v;

		/*
		 * Carry always fits on 31 bits; we want to keep it in a
		 * 32-bit register on 32-bit architectures (on a 64-bit
		 * architecture, cast down from 64 to 32 bits means
		 * clearing the high bits, which is not free; on a 32-bit
		 * architecture, the same operation really means ignoring
		 * the top register, which has negative or zero cost).
		 */
#if BR_64
		uint64_t cc;
#else
		uint32_t cc;
#endif

		f = b[1 + u];
		cc = 0;
		for (v = 0; v < alen; v ++) {
			uint64_t z;

			z = (uint64_t)d[1 + u + v] + MUL31(f, a[1 + v]) + cc;
			cc = z >> 31;
			d[1 + u + v] = (uint32_t)z & 0x7FFFFFFF;
		}
		d[1 + u + alen] = (uint32_t)cc;
	}
}

static inline void
br_enc32be(void *dst, uint32_t x)
{
#if BR_BE_UNALIGNED
	((br_union_u32 *)dst)->u = x;
#else
	unsigned char *buf;

	buf = (unsigned char*) dst;
	buf[0] = (unsigned char)(x >> 24);
	buf[1] = (unsigned char)(x >> 16);
	buf[2] = (unsigned char)(x >> 8);
	buf[3] = (unsigned char)x;
#endif
}

/* see inner.h */
void
br_i31_encode(void *dst, size_t len, const uint32_t *x)
{
	unsigned char *buf;
	size_t k, xlen;
	uint32_t acc;
	int acc_len;

	xlen = (x[0] + 31) >> 5;
	if (xlen == 0) {
		memset(dst, 0, len);
		return;
	}
	buf = (unsigned char *)dst + len;
	k = 1;
	acc = 0;
	acc_len = 0;
	while (len != 0) {
		uint32_t w;

		w = (k <= xlen) ? x[k] : 0;
		k ++;
		if (acc_len == 0) {
			acc = w;
			acc_len = 31;
		} else {
			uint32_t z;

			z = acc | (w << acc_len);
			acc_len --;
			acc = w >> (31 - acc_len);
			if (len >= 4) {
				buf -= 4;
				len -= 4;
				br_enc32be(buf, z);
			} else {
				switch (len) {
				case 3:
					buf[-3] = (unsigned char)(z >> 16);
					/* fall through */
				case 2:
					buf[-2] = (unsigned char)(z >> 8);
					/* fall through */
				case 1:
					buf[-1] = (unsigned char)z;
					break;
				}
				return;
			}
		}
	}
}


/* see inner.h */
uint32_t
br_i31_ninv31(uint32_t x)
{
	uint32_t y;

	y = 2 - x;
	y *= 2 - y * x;
	y *= 2 - y * x;
	y *= 2 - y * x;
	y *= 2 - y * x;
	return MUX(x & 1, -y, 0) & 0x7FFFFFFF;
}

#define GE(x, y)   NOT(GT(y, x))
#define LT(x, y)   GT(y, x)
#define LE(x, y)   NOT(GT(x, y))

/* see inner.h */
uint32_t
br_divrem(uint32_t hi, uint32_t lo, uint32_t d, uint32_t *r)
{
	/* TODO: optimize this */
	uint32_t q;
	uint32_t ch, cf;
	int k;

	q = 0;
	ch = EQ(hi, d);
	hi = MUX(ch, 0, hi);
	for (k = 31; k > 0; k --) {
		int j;
		uint32_t w, ctl, hi2, lo2;

		j = 32 - k;
		w = (hi << j) | (lo >> k);
		ctl = GE(w, d) | (hi >> k);
		hi2 = (w - d) >> j;
		lo2 = lo - (d << k);
		hi = MUX(ctl, hi2, hi);
		lo = MUX(ctl, lo2, lo);
		q |= ctl << k;
	}
	cf = GE(lo, d) | hi;
	q |= cf;
	*r = MUX(cf, lo - d, lo);
	return q;
}

/*
 * Wrapper for br_divrem(); the remainder is returned, and the quotient
 * is discarded.
 */
static inline uint32_t
br_rem(uint32_t hi, uint32_t lo, uint32_t d)
{
	uint32_t r;

	br_divrem(hi, lo, d, &r);
	return r;
}

/*
 * Wrapper for br_divrem(); the quotient is returned, and the remainder
 * is discarded.
 */
static inline uint32_t
br_div(uint32_t hi, uint32_t lo, uint32_t d)
{
	uint32_t r;

	return br_divrem(hi, lo, d, &r);
}


/* see inner.h */
uint32_t
br_i31_add(uint32_t *a, const uint32_t *b, uint32_t ctl)
{
	uint32_t cc;
	size_t u, m;

	cc = 0;
	m = (a[0] + 63) >> 5;
	for (u = 1; u < m; u ++) {
		uint32_t aw, bw, naw;

		aw = a[u];
		bw = b[u];
		naw = aw + bw + cc;
		cc = naw >> 31;
		a[u] = MUX(ctl, naw & (uint32_t)0x7FFFFFFF, aw);
	}
	return cc;
}


/* see inner.h */
uint32_t
br_i31_sub(uint32_t *a, const uint32_t *b, uint32_t ctl)
{
	uint32_t cc;
	size_t u, m;

	cc = 0;
	m = (a[0] + 63) >> 5;
	for (u = 1; u < m; u ++) {
		uint32_t aw, bw, naw;

		aw = a[u];
		bw = b[u];
		naw = aw - bw - cc;
		cc = naw >> 31;
		a[u] = MUX(ctl, naw & 0x7FFFFFFF, aw);
	}
	return cc;
}

/* see inner.h */
void
br_i31_muladd_small(uint32_t *x, uint32_t z, const uint32_t *m)
{
	uint32_t m_bitlen;
	unsigned mblr;
	size_t u, mlen;
	uint32_t a0, a1, b0, hi, g, q, tb;
	uint32_t under, over;
	uint32_t cc;

	/*
	 * We can test on the modulus bit length since we accept to
	 * leak that length.
	 */
	m_bitlen = m[0];
	if (m_bitlen == 0) {
		return;
	}
	if (m_bitlen <= 31) {
		uint32_t lo;

		hi = x[1] >> 1;
		lo = (x[1] << 31) | z;
		x[1] = br_rem(hi, lo, m[1]);
		return;
	}
	mlen = (m_bitlen + 31) >> 5;
	mblr = (unsigned)m_bitlen & 31;

	/*
	 * Principle: we estimate the quotient (x*2^31+z)/m by
	 * doing a 64/32 division with the high words.
	 *
	 * Let:
	 *   w = 2^31
	 *   a = (w*a0 + a1) * w^N + a2
	 *   b = b0 * w^N + b2
	 * such that:
	 *   0 <= a0 < w
	 *   0 <= a1 < w
	 *   0 <= a2 < w^N
	 *   w/2 <= b0 < w
	 *   0 <= b2 < w^N
	 *   a < w*b
	 * I.e. the two top words of a are a0:a1, the top word of b is
	 * b0, we ensured that b0 is "full" (high bit set), and a is
	 * such that the quotient q = a/b fits on one word (0 <= q < w).
	 *
	 * If a = b*q + r (with 0 <= r < q), we can estimate q by
	 * doing an Euclidean division on the top words:
	 *   a0*w+a1 = b0*u + v  (with 0 <= v < b0)
	 * Then the following holds:
	 *   0 <= u <= w
	 *   u-2 <= q <= u
	 */
	hi = x[mlen];
	if (mblr == 0) {
		a0 = x[mlen];
		memmove(x + 2, x + 1, (mlen - 1) * sizeof *x);
		x[1] = z;
		a1 = x[mlen];
		b0 = m[mlen];
	} else {
		a0 = ((x[mlen] << (31 - mblr)) | (x[mlen - 1] >> mblr))
			& 0x7FFFFFFF;
		memmove(x + 2, x + 1, (mlen - 1) * sizeof *x);
		x[1] = z;
		a1 = ((x[mlen] << (31 - mblr)) | (x[mlen - 1] >> mblr))
			& 0x7FFFFFFF;
		b0 = ((m[mlen] << (31 - mblr)) | (m[mlen - 1] >> mblr))
			& 0x7FFFFFFF;
	}

	/*
	 * We estimate a divisor q. If the quotient returned by br_div()
	 * is g:
	 * -- If a0 == b0 then g == 0; we want q = 0x7FFFFFFF.
	 * -- Otherwise:
	 *    -- if g == 0 then we set q = 0;
	 *    -- otherwise, we set q = g - 1.
	 * The properties described above then ensure that the true
	 * quotient is q-1, q or q+1.
	 *
	 * Take care that a0, a1 and b0 are 31-bit words, not 32-bit. We
	 * must adjust the parameters to br_div() accordingly.
	 */
	g = br_div(a0 >> 1, a1 | (a0 << 31), b0);
	q = MUX(EQ(a0, b0), 0x7FFFFFFF, MUX(EQ(g, 0), 0, g - 1));

	/*
	 * We subtract q*m from x (with the extra high word of value 'hi').
	 * Since q may be off by 1 (in either direction), we may have to
	 * add or subtract m afterwards.
	 *
	 * The 'tb' flag will be true (1) at the end of the loop if the
	 * result is greater than or equal to the modulus (not counting
	 * 'hi' or the carry).
	 */
	cc = 0;
	tb = 1;
	for (u = 1; u <= mlen; u ++) {
		uint32_t mw, zw, xw, nxw;
		uint64_t zl;

		mw = m[u];
		zl = MUL31(mw, q) + cc;
		cc = (uint32_t)(zl >> 31);
		zw = (uint32_t)zl & (uint32_t)0x7FFFFFFF;
		xw = x[u];
		nxw = xw - zw;
		cc += nxw >> 31;
		nxw &= 0x7FFFFFFF;
		x[u] = nxw;
		tb = MUX(EQ(nxw, mw), tb, GT(nxw, mw));
	}

	/*
	 * If we underestimated q, then either cc < hi (one extra bit
	 * beyond the top array word), or cc == hi and tb is true (no
	 * extra bit, but the result is not lower than the modulus). In
	 * these cases we must subtract m once.
	 *
	 * Otherwise, we may have overestimated, which will show as
	 * cc > hi (thus a negative result). Correction is adding m once.
	 */
	over = GT(cc, hi);
	under = ~over & (tb | LT(cc, hi));
	br_i31_add(x, m, over);
	br_i31_sub(x, m, under);
}


/* see inner.h */
void
br_i31_rshift(uint32_t *x, int count)
{
	size_t u, len;
	uint32_t r;

	len = (x[0] + 31) >> 5;
	if (len == 0) {
		return;
	}
	r = x[1] >> count;
	for (u = 2; u <= len; u ++) {
		uint32_t w;

		w = x[u];
		x[u - 1] = ((w << (31 - count)) | r) & 0x7FFFFFFF;
		r = w >> count;
	}
	x[len] = r;
}

/* see inner.h */
void
br_i31_decode_reduce(uint32_t *x,
	const void *src, size_t len, const uint32_t *m)
{
	uint32_t m_ebitlen, m_rbitlen;
	size_t mblen, k;
	const unsigned char *buf;
	uint32_t acc;
	int acc_len;

	/*
	 * Get the encoded bit length.
	 */
	m_ebitlen = m[0];

	/*
	 * Special case for an invalid (null) modulus.
	 */
	if (m_ebitlen == 0) {
		x[0] = 0;
		return;
	}

	/*
	 * Clear the destination.
	 */
	br_i31_zero(x, m_ebitlen);

	/*
	 * First decode directly as many bytes as possible. This requires
	 * computing the actual bit length.
	 */
	m_rbitlen = m_ebitlen >> 5;
	m_rbitlen = (m_ebitlen & 31) + (m_rbitlen << 5) - m_rbitlen;
	mblen = (m_rbitlen + 7) >> 3;
	k = mblen - 1;
	if (k >= len) {
		br_i31_decode(x, src, len);
		x[0] = m_ebitlen;
		return;
	}
	buf = (const unsigned char*) src;
	br_i31_decode(x, buf, k);
	x[0] = m_ebitlen;

	/*
	 * Input remaining bytes, using 31-bit words.
	 */
	acc = 0;
	acc_len = 0;
	while (k < len) {
		uint32_t v;

		v = buf[k ++];
		if (acc_len >= 23) {
			acc_len -= 23;
			acc <<= (8 - acc_len);
			acc |= v >> acc_len;
			br_i31_muladd_small(x, acc, m);
			acc = v & (0xFF >> (8 - acc_len));
		} else {
			acc = (acc << 8) | v;
			acc_len += 8;
		}
	}

	/*
	 * We may have some bits accumulated. We then perform a shift to
	 * be able to inject these bits as a full 31-bit word.
	 */
	if (acc_len != 0) {
		acc = (acc | (x[1] << acc_len)) & 0x7FFFFFFF;
		br_i31_rshift(x, 31 - acc_len);
		br_i31_muladd_small(x, acc, m);
	}
}

/* see inner.h */
void
br_i31_to_monty(uint32_t *x, const uint32_t *m)
{
	uint32_t k;

	for (k = (m[0] + 31) >> 5; k > 0; k --) {
		br_i31_muladd_small(x, 0, m);
	}
}


/* see inner.h */
void
br_i31_montymul(uint32_t *d, const uint32_t *x, const uint32_t *y,
	const uint32_t *m, uint32_t m0i)
{
	/*
	 * Each outer loop iteration computes:
	 *   d <- (d + xu*y + f*m) / 2^31
	 * We have xu <= 2^31-1 and f <= 2^31-1.
	 * Thus, if d <= 2*m-1 on input, then:
	 *   2*m-1 + 2*(2^31-1)*m <= (2^32)*m-1
	 * and the new d value is less than 2*m.
	 *
	 * We represent d over 31-bit words, with an extra word 'dh'
	 * which can thus be only 0 or 1.
	 */
	size_t len, len4, u, v;
	uint32_t dh;

	len = (m[0] + 31) >> 5;
	len4 = len & ~(size_t)3;
	br_i31_zero(d, m[0]);
	dh = 0;
	for (u = 0; u < len; u ++) {
		/*
		 * The carry for each operation fits on 32 bits:
		 *   d[v+1] <= 2^31-1
		 *   xu*y[v+1] <= (2^31-1)*(2^31-1)
		 *   f*m[v+1] <= (2^31-1)*(2^31-1)
		 *   r <= 2^32-1
		 *   (2^31-1) + 2*(2^31-1)*(2^31-1) + (2^32-1) = 2^63 - 2^31
		 * After division by 2^31, the new r is then at most 2^32-1
		 *
		 * Using a 32-bit carry has performance benefits on 32-bit
		 * systems; however, on 64-bit architectures, we prefer to
		 * keep the carry (r) in a 64-bit register, thus avoiding some
		 * "clear high bits" operations.
		 */
		uint32_t f, xu;
#if BR_64
		uint64_t r;
#else
		uint32_t r;
#endif

		xu = x[u + 1];
		f = MUL31_lo((d[1] + MUL31_lo(x[u + 1], y[1])), m0i);

		r = 0;
		for (v = 0; v < len4; v += 4) {
			uint64_t z;

			z = (uint64_t)d[v + 1] + MUL31(xu, y[v + 1])
				+ MUL31(f, m[v + 1]) + r;
			r = z >> 31;
			d[v + 0] = (uint32_t)z & 0x7FFFFFFF;
			z = (uint64_t)d[v + 2] + MUL31(xu, y[v + 2])
				+ MUL31(f, m[v + 2]) + r;
			r = z >> 31;
			d[v + 1] = (uint32_t)z & 0x7FFFFFFF;
			z = (uint64_t)d[v + 3] + MUL31(xu, y[v + 3])
				+ MUL31(f, m[v + 3]) + r;
			r = z >> 31;
			d[v + 2] = (uint32_t)z & 0x7FFFFFFF;
			z = (uint64_t)d[v + 4] + MUL31(xu, y[v + 4])
				+ MUL31(f, m[v + 4]) + r;
			r = z >> 31;
			d[v + 3] = (uint32_t)z & 0x7FFFFFFF;
		}
		for (; v < len; v ++) {
			uint64_t z;

			z = (uint64_t)d[v + 1] + MUL31(xu, y[v + 1])
				+ MUL31(f, m[v + 1]) + r;
			r = z >> 31;
			d[v] = (uint32_t)z & 0x7FFFFFFF;
		}

		/*
		 * Since the new dh can only be 0 or 1, the addition of
		 * the old dh with the carry MUST fit on 32 bits, and
		 * thus can be done into dh itself.
		 */
		dh += r;
		d[len] = dh & 0x7FFFFFFF;
		dh >>= 31;
	}

	/*
	 * We must write back the bit length because it was overwritten in
	 * the loop (not overwriting it would require a test in the loop,
	 * which would yield bigger and slower code).
	 */
	d[0] = m[0];

	/*
	 * d[] may still be greater than m[] at that point; notably, the
	 * 'dh' word may be non-zero.
	 */
	br_i31_sub(d, m, NEQ(dh, 0) | NOT(br_i31_sub(d, m, 0)));
}

/* see inner.h */
void
br_ccopy(uint32_t ctl, void *dst, const void *src, size_t len)
{
	unsigned char *d;
	const unsigned char *s;

	d = (unsigned char*) dst;
	s = (unsigned char*) src;
	while (len -- > 0) {
		uint32_t x, y;

		x = *s ++;
		y = *d;
		*d = MUX(ctl, x, y);
		d ++;
	}
}

#define CCOPY   br_ccopy


/* see inner.h */
void
br_i31_from_monty(uint32_t *x, const uint32_t *m, uint32_t m0i)
{
	size_t len, u, v;

	len = (m[0] + 31) >> 5;
	for (u = 0; u < len; u ++) {
		uint32_t f;
		uint64_t cc;

		f = MUL31_lo(x[1], m0i);
		cc = 0;
		for (v = 0; v < len; v ++) {
			uint64_t z;

			z = (uint64_t)x[v + 1] + MUL31(f, m[v + 1]) + cc;
			cc = z >> 31;
			if (v != 0) {
				x[v] = (uint32_t)z & 0x7FFFFFFF;
			}
		}
		x[len] = (uint32_t)cc;
	}

	/*
	 * We may have to do an extra subtraction, but only if the
	 * value in x[] is indeed greater than or equal to that of m[],
	 * which is why we must do two calls (first call computes the
	 * carry, second call performs the subtraction only if the carry
	 * is 0).
	 */
	br_i31_sub(x, m, NOT(br_i31_sub(x, m, 0)));
}

/* see inner.h */
uint32_t
br_i31_modpow_opt(uint32_t *x,
	const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *tmp, size_t twlen)
{
	size_t mlen, mwlen;
	uint32_t *t1, *t2, *base;
	size_t u, v;
	uint32_t acc;
	int acc_len, win_len;

	/*
	 * Get modulus size.
	 */
	mwlen = (m[0] + 63) >> 5;
	mlen = mwlen * sizeof m[0];
	mwlen += (mwlen & 1);
	t1 = tmp;
	t2 = tmp + mwlen;

	/*
	 * Compute possible window size, with a maximum of 5 bits.
	 * When the window has size 1 bit, we use a specific code
	 * that requires only two temporaries. Otherwise, for a
	 * window of k bits, we need 2^k+1 temporaries.
	 */
	if (twlen < (mwlen << 1)) {
		return 0;
	}
	for (win_len = 5; win_len > 1; win_len --) {
		if ((((uint32_t)1 << win_len) + 1) * mwlen <= twlen) {
			break;
		}
	}

	/*
	 * Everything is done in Montgomery representation.
	 */
	br_i31_to_monty(x, m);

	/*
	 * Compute window contents. If the window has size one bit only,
	 * then t2 is set to x; otherwise, t2[0] is left untouched, and
	 * t2[k] is set to x^k (for k >= 1).
	 */
	if (win_len == 1) {
		memcpy(t2, x, mlen);
	} else {
		memcpy(t2 + mwlen, x, mlen);
		base = t2 + mwlen;
		for (u = 2; u < ((unsigned)1 << win_len); u ++) {
			br_i31_montymul(base + mwlen, base, x, m, m0i);
			base += mwlen;
		}
	}

	/*
	 * We need to set x to 1, in Montgomery representation. This can
	 * be done efficiently by setting the high word to 1, then doing
	 * one word-sized shift.
	 */
	br_i31_zero(x, m[0]);
	x[(m[0] + 31) >> 5] = 1;
	br_i31_muladd_small(x, 0, m);

	/*
	 * We process bits from most to least significant. At each
	 * loop iteration, we have acc_len bits in acc.
	 */
	acc = 0;
	acc_len = 0;
	while (acc_len > 0 || elen > 0) {
		int i, k;
		uint32_t bits;

		/*
		 * Get the next bits.
		 */
		k = win_len;
		if (acc_len < win_len) {
			if (elen > 0) {
				acc = (acc << 8) | *e ++;
				elen --;
				acc_len += 8;
			} else {
				k = acc_len;
			}
		}
		bits = (acc >> (acc_len - k)) & (((uint32_t)1 << k) - 1);
		acc_len -= k;

		/*
		 * We could get exactly k bits. Compute k squarings.
		 */
		for (i = 0; i < k; i ++) {
			br_i31_montymul(t1, x, x, m, m0i);
			memcpy(x, t1, mlen);
		}

		/*
		 * Window lookup: we want to set t2 to the window
		 * lookup value, assuming the bits are non-zero. If
		 * the window length is 1 bit only, then t2 is
		 * already set; otherwise, we do a constant-time lookup.
		 */
		if (win_len > 1) {
			br_i31_zero(t2, m[0]);
			base = t2 + mwlen;
			for (u = 1; u < ((uint32_t)1 << k); u ++) {
				uint32_t mask;

				mask = -EQ(u, bits);
				for (v = 1; v < mwlen; v ++) {
					t2[v] |= mask & base[v];
				}
				base += mwlen;
			}
		}

		/*
		 * Multiply with the looked-up value. We keep the
		 * product only if the exponent bits are not all-zero.
		 */
		br_i31_montymul(t1, x, t2, m, m0i);
		CCOPY(NEQ(bits, 0), x, t1, mlen);
	}

	/*
	 * Convert back from Montgomery representation, and exit.
	 */
	br_i31_from_monty(x, m, m0i);
	return 1;
}


/* see inner.h */
void
br_i31_reduce(uint32_t *x, const uint32_t *a, const uint32_t *m)
{
	uint32_t m_bitlen, a_bitlen;
	size_t mlen, alen, u;

	m_bitlen = m[0];
	mlen = (m_bitlen + 31) >> 5;

	x[0] = m_bitlen;
	if (m_bitlen == 0) {
		return;
	}

	/*
	 * If the source is shorter, then simply copy all words from a[]
	 * and zero out the upper words.
	 */
	a_bitlen = a[0];
	alen = (a_bitlen + 31) >> 5;
	if (a_bitlen < m_bitlen) {
		memcpy(x + 1, a + 1, alen * sizeof *a);
		for (u = alen; u < mlen; u ++) {
			x[u + 1] = 0;
		}
		return;
	}

	/*
	 * The source length is at least equal to that of the modulus.
	 * We must thus copy N-1 words, and input the remaining words
	 * one by one.
	 */
	memcpy(x + 1, a + 2 + (alen - mlen), (mlen - 1) * sizeof *a);
	x[mlen] = 0;
	for (u = 1 + alen - mlen; u > 0; u --) {
		br_i31_muladd_small(x, a[u], m);
	}
}

/* see bearssl_rsa.h */
uint32_t
br_rsa_i31_private(unsigned char *x, const br_rsa_private_key *sk)
{
	const unsigned char *p, *q;
	size_t plen, qlen;
	size_t fwlen;
	uint32_t p0i, q0i;
	size_t xlen, u;
	uint32_t tmp[1 + TLEN];
	long z;
	uint32_t *mp, *mq, *s1, *s2, *t1, *t2, *t3;
	uint32_t r;

	/*
	 * Compute the actual lengths of p and q, in bytes.
	 * These lengths are not considered secret (we cannot really hide
	 * them anyway in constant-time code).
	 */
	p = sk->p;
	plen = sk->plen;
	while (plen > 0 && *p == 0) {
		p ++;
		plen --;
	}
	q = sk->q;
	qlen = sk->qlen;
	while (qlen > 0 && *q == 0) {
		q ++;
		qlen --;
	}

	/*
	 * Compute the maximum factor length, in words.
	 */
	z = (long)(plen > qlen ? plen : qlen) << 3;
	fwlen = 1;
	while (z > 0) {
		z -= 31;
		fwlen ++;
	}

	/*
	 * Round up the word length to an even number.
	 */
	fwlen += (fwlen & 1);

	/*
	 * We need to fit at least 6 values in the stack buffer.
	 */
	if (6 * fwlen > TLEN) {
		return 0;
	}

	/*
	 * Compute modulus length (in bytes).
	 */
	xlen = (sk->n_bitlen + 7) >> 3;

	/*
	 * Decode q.
	 */
	mq = tmp;
	br_i31_decode(mq, q, qlen);

	/*
	 * Decode p.
	 */
	t1 = mq + fwlen;
	br_i31_decode(t1, p, plen);

	/*
	 * Compute the modulus (product of the two factors), to compare
	 * it with the source value. We use br_i31_mulacc(), since it's
	 * already used later on.
	 */
	t2 = mq + 2 * fwlen;
	br_i31_zero(t2, mq[0]);
	br_i31_mulacc(t2, mq, t1);

	/*
	 * We encode the modulus into bytes, to perform the comparison
	 * with bytes. We know that the product length, in bytes, is
	 * exactly xlen.
	 * The comparison actually computes the carry when subtracting
	 * the modulus from the source value; that carry must be 1 for
	 * a value in the correct range. We keep it in r, which is our
	 * accumulator for the error code.
	 */
	t3 = mq + 4 * fwlen;
	br_i31_encode(t3, xlen, t2);
	u = xlen;
	r = 0;
	while (u > 0) {
		uint32_t wn, wx;

		u --;
		wn = ((unsigned char *)t3)[u];
		wx = x[u];
		r = ((wx - (wn + r)) >> 8) & 1;
	}

	/*
	 * Move the decoded p to another temporary buffer.
	 */
	mp = mq + 2 * fwlen;
	memmove(mp, t1, fwlen * sizeof *t1);

	/*
	 * Compute s2 = x^dq mod q.
	 */
	q0i = br_i31_ninv31(mq[1]);
	s2 = mq + fwlen;
	br_i31_decode_reduce(s2, x, xlen, mq);
	r &= br_i31_modpow_opt(s2, sk->dq, sk->dqlen, mq, q0i,
		mq + 3 * fwlen, TLEN - 3 * fwlen);

	/*
	 * Compute s1 = x^dp mod p.
	 */
	p0i = br_i31_ninv31(mp[1]);
	s1 = mq + 3 * fwlen;
	br_i31_decode_reduce(s1, x, xlen, mp);
	r &= br_i31_modpow_opt(s1, sk->dp, sk->dplen, mp, p0i,
		mq + 4 * fwlen, TLEN - 4 * fwlen);

	/*
	 * Compute:
	 *   h = (s1 - s2)*(1/q) mod p
	 * s1 is an integer modulo p, but s2 is modulo q. PKCS#1 is
	 * unclear about whether p may be lower than q (some existing,
	 * widely deployed implementations of RSA don't tolerate p < q),
	 * but we want to support that occurrence, so we need to use the
	 * reduction function.
	 *
	 * Since we use br_i31_decode_reduce() for iq (purportedly, the
	 * inverse of q modulo p), we also tolerate improperly large
	 * values for this parameter.
	 */
	t1 = mq + 4 * fwlen;
	t2 = mq + 5 * fwlen;
	br_i31_reduce(t2, s2, mp);
	br_i31_add(s1, mp, br_i31_sub(s1, t2, 1));
	br_i31_to_monty(s1, mp);
	br_i31_decode_reduce(t1, sk->iq, sk->iqlen, mp);
	br_i31_montymul(t2, s1, t1, mp, p0i);

	/*
	 * h is now in t2. We compute the final result:
	 *   s = s2 + q*h
	 * All these operations are non-modular.
	 *
	 * We need mq, s2 and t2. We use the t3 buffer as destination.
	 * The buffers mp, s1 and t1 are no longer needed, so we can
	 * reuse them for t3. Moreover, the first step of the computation
	 * is to copy s2 into t3, after which s2 is not needed. Right
	 * now, mq is in slot 0, s2 is in slot 1, and t2 is in slot 5.
	 * Therefore, we have ample room for t3 by simply using s2.
	 */
	t3 = s2;
	br_i31_mulacc(t3, mq, t2);

	/*
	 * Encode the result. Since we already checked the value of xlen,
	 * we can just use it right away.
	 */
	br_i31_encode(x, xlen, t3);

	/*
	 * The only error conditions remaining at that point are invalid
	 * values for p and q (even integers).
	 */
	return p0i & q0i & r;
}

/* see inner.h */
uint32_t
br_rsa_pkcs1_sig_pad(const unsigned char *hash_oid,
    const unsigned char *hash, size_t hash_len,
    uint32_t n_bitlen, unsigned char *x)
{
    size_t u, x3, xlen;

    /*
     * Padded hash value has format:
     *  00 01 FF .. FF 00 30 x1 30 x2 06 x3 OID 05 00 04 x4 HASH
     *
     * with the following rules:
     *
     *  -- Total length is equal to the modulus length (unsigned
     *     encoding).
     *
     *  -- There must be at least eight bytes of value 0xFF.
     *
     *  -- x4 is equal to the hash length (hash_len).
     *
     *  -- x3 is equal to the encoded OID value length (hash_oid[0]).
     *
     *  -- x2 = x3 + 4.
     *
     *  -- x1 = x2 + x4 + 4 = x3 + x4 + 8.
     *
     * Note: the "05 00" is optional (signatures with and without
     * that sequence exist in practice), but notes in PKCS#1 seem to
     * indicate that the presence of that sequence (specifically,
     * an ASN.1 NULL value for the hash parameters) may be slightly
     * more "standard" than the opposite.
     */
    xlen = (n_bitlen + 7) >> 3;

    if (hash_oid == NULL) {
        if (xlen < hash_len + 11) {
            return 0;
        }
        x[0] = 0x00;
        x[1] = 0x01;
        u = xlen - hash_len;
        memset(x + 2, 0xFF, u - 3);
        x[u - 1] = 0x00;
    } else {
        x3 = hash_oid[0];

        /*
         * Check that there is enough room for all the elements,
         * including at least eight bytes of value 0xFF.
         */
        if (xlen < (x3 + hash_len + 21)) {
            return 0;
        }
        x[0] = 0x00;
        x[1] = 0x01;
        u = xlen - x3 - hash_len - 11;
        memset(x + 2, 0xFF, u - 2);
        x[u] = 0x00;
        x[u + 1] = 0x30;
        x[u + 2] = x3 + hash_len + 8;
        x[u + 3] = 0x30;
        x[u + 4] = x3 + 4;
        x[u + 5] = 0x06;
        memcpy(x + u + 6, hash_oid, x3 + 1);
        u += x3 + 7;
        x[u ++] = 0x05;
        x[u ++] = 0x00;
        x[u ++] = 0x04;
        x[u ++] = hash_len;
    }
    memcpy(x + u, hash, hash_len);
    return 1;
}

/* see bearssl_rsa.h */
uint32_t
br_rsa_i31_pkcs1_sign(const unsigned char *hash_oid,
	const unsigned char *hash, size_t hash_len,
	const br_rsa_private_key *sk, unsigned char *x)
{
	if (!br_rsa_pkcs1_sig_pad(hash_oid, hash, hash_len, sk->n_bitlen, x)) {
		return 0;
	}
	return EmberIotRSA::br_rsa_i31_private(x, sk);
}
}

#endif