#include "rand.h"
/* 
   rand.cc - Copyright (C) 2003-2004, Reed A. Cartwright
   All right reserved.

   ======================================================================  
   MT19937-2002 Algorithm

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp

*/

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UMASK 0x80000000UL /* most significant w-r bits */
#define LMASK 0x7fffffffUL /* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

#if !defined(USE_MMX)
static unsigned long state[N]; /* the array for the state vector  */
#else
static __m64	state64[N/2];
static unsigned long *state = (unsigned long *)&state64[0];
#endif
static int left = 1;
static unsigned long *next;

void mt_srand(unsigned long s)
{
    int j;
    state[0]= s & 0xffffffffUL;
    for (j=1; j<N; j++) {
        state[j] = (1812433253UL * (state[j-1] ^ (state[j-1] >> 30)) + j); 
        //state[j] &= 0xffffffffUL;  /* for >32 bit machines */
    }
    left = 1;
}

// Default seed procedure
static struct mt_rand_init
{
	mt_rand_init() { mt_srand(5489UL); }
} drinit;

void mt_srand(unsigned long uKeys[], unsigned long uLen)
{
    unsigned int i, j, k;
    mt_srand(19650218UL);
    i=1; j=0;
    k = (N>uLen ? N : uLen);
    for (; k; k--) {
        state[i] = (state[i] ^ ((state[i-1] ^ (state[i-1] >> 30)) * 1664525UL))
          + uKeys[j] + j; /* non linear */
        //state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { state[0] = state[N-1]; i=1; }
        if (j>=uLen) j=0;
    }
    for (k=N-1; k; k--) {
        state[i] = (state[i] ^ ((state[i-1] ^ (state[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        //state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { state[0] = state[N-1]; i=1; }
    }

    state[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
    left = 1;
}

inline void mt_next_state()
{
#if !defined(USE_MMX)
	unsigned long *p=state;
    for (int j=N-M+1; --j; p++) 
        *p = p[M] ^ TWIST(p[0], p[1]);
    for (int j=M; --j; p++) 
        *p = p[M-N] ^ TWIST(p[0], p[1]);
    *p = p[M-N] ^ TWIST(p[0], state[0]);
#else
	static const __m64 Zero = _mm_setzero_si64();
	static const __m64 One = _mm_set1_pi32(1u);
	static const __m64 Magic = _mm_set1_pi32(0x9908b0dfu);
	static const __m64 Low = _mm_set1_pi32(0x7fffffffu);

	__m64 *v1 = &state64[0];
	for(int i=0;i<113;++i)
	{
		*v1++ = _mm_xor_si64(_mm_xor_si64(_mm_srli_pi32(_mm_xor_si64(
			_mm_and_si64(_mm_xor_si64(*v1, *(__m64*)(((unsigned long*)v1)+1)),Low), *v1), 1), 
			_mm_and_si64(_mm_cmpeq_pi32(_mm_and_si64(*(__m64*)(((unsigned long*)v1)+1),
			One), One), Magic)), *(__m64*)(((unsigned long*)v1)+M));
	}
	state[226] = ((((state[226]^state[227])&0x7fffffffu)^state[226])>>1)
		^(0x9908b0dfu & (((state[227]& 1u) == 1u) ? 0xFFFFFFFFu : 0u))^state[623];
	state[227] = ((((state[227]^state[228])&0x7fffffffu)^state[227])>>1)
					^(0x9908b0dfu & (((state[228]& 1u) == 1u) ? 0xFFFFFFFFu : 0u))^state[0];
	*v1++;
	for(int i=114;i<311;++i)
	{
		*v1++ = _mm_xor_si64(_mm_xor_si64(_mm_srli_pi32(_mm_xor_si64(
			_mm_and_si64(_mm_xor_si64(*v1, *(__m64*)(((unsigned long*)v1)+1)),Low), *v1), 1), 
			_mm_and_si64(_mm_cmpeq_pi32(_mm_and_si64(*(__m64*)(((unsigned long*)v1)+1),
			One), One), Magic)), *(__m64*)(((unsigned long*)v1)+M-N));
	}
	state[622] = ((((state[622]^state[623])&0x7fffffffu)^state[622])>>1)
					^(0x9908b0dfu & (((state[623]& 1u) == 1u) ? 0xFFFFFFFFu : 0u))^state[395];
	state[623] = ((((state[623]^state[0])&0x7fffffffu)^state[623])>>1)
					^(0x9908b0dfu & (((state[0]& 1u) == 1u) ? 0xFFFFFFFFu : 0u))^state[396];
	_mm_empty();
#endif
	left = N;
    next = state;
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long mt_rand()
{
    if (--left == 0)
		mt_next_state();

	unsigned long y = *next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}
