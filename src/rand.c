/**
 * @file rand.c
 * @author Mike Scott
 * @author Kealan McCusker
 * @date 19th May 2015
 * @brief AMCL Implementation of a cryptographic strong random number generator
 *
 * @section LICENSE
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/* AMCL Implementation of a cryptographic strong random number generator */

/*
 *   Cryptographic strong random number generator
 *
 *   Unguessable seed -> SHA -> PRNG internal state -> SHA -> random numbers
 *   Slow - but secure
 *
 *   See ftp://ftp.rsasecurity.com/pub/pdfs/bull-1.pdf for a justification
 */

/* SU=m, m is Stack Usage */

#include "amcl.h"

/* SU= 20, Marsaglia & Zaman random number generator */
static unsign32 sbrand(csprng *rng)
{
    int i,k;
    unsign32 pdiff,t;
    rng->rndptr++;
    if (rng->rndptr<NK) return rng->ira[rng->rndptr];
    rng->rndptr=0;
    for (i=0,k=NK-NJ; i<NK; i++,k++)
    {
        /* calculate next NK values */
        if (k==NK) k=0;
        t=rng->ira[k];
        pdiff=t - rng->ira[i] - rng->borrow;

        if (pdiff<t) rng->borrow=0;
        if (pdiff>t) rng->borrow=1;
        rng->ira[i]=pdiff;
    }
    return rng->ira[0];
}

/* SU= 20, initialise random number system */
static void sirand(csprng* rng,unsign32 seed)
{
    /* modified so that a subsequent call "stirs" in another seed
       value in this way as many seed bits as desired may be used */
    int i,in;
    unsign32 t,m=1;
    rng->borrow=0L;
    rng->rndptr=0;
    rng->ira[0]^=seed;
    for (i=1; i<NK; i++)
    {
        // fill initialisation vector
        in=(NV*i)%NK;
        rng->ira[in]^=m; //note XOR
        t=m;
        m=seed-m;
        seed=t;
    }
    for (i=0; i<10000; i++) sbrand(rng ); // "warm-up" & stir the generator
}

/* SU= 312, hash down output of RNG to re-fill the pool */
static void fill_pool(csprng *rng)
{
    int i;
    hash256 sh;
    HASH256_init(&sh);
    for (i=0; i<128; i++) HASH256_process(&sh,sbrand(rng));
    HASH256_hash(&sh,rng->pool);
    rng->pool_ptr=0;
}

/* Pack bytes into a 32-bit word */
static unsign32 pack(const uchar *b)
{
    return ((unsign32)b[3]<<24)|((unsign32)b[2]<<16)|((unsign32)b[1]<<8)|(unsign32)b[0];
}

/* SU= 360, initialize RNG with some real entropy from some external source */
void RAND_seed(csprng *rng,int rawlen,char *raw)
{
    /* initialise from at least 128 byte string
       of raw random input, and 32-bit time-of-day */
    int i;
    char digest[32];
    uchar b[4];
    hash256 sh;
    rng->pool_ptr=0;
    for (i=0; i<NK; i++) rng->ira[i]=0;
    if (rawlen>0)
    {
        HASH256_init(&sh);
        for (i=0; i<rawlen; i++)
            HASH256_process(&sh,raw[i]);
        HASH256_hash(&sh,digest);

        /* initialise PRNG from distilled randomness */

        for (i=0; i<8; i++)
        {
            b[0]=digest[4*i];
            b[1]=digest[4*i+1];
            b[2]=digest[4*i+2];
            b[3]=digest[4*i+3];
            //	printf("%08x\n",pack(b));
            sirand(rng,pack(b));
        }
    }
    fill_pool(rng);
}

/* Terminate and clean up */
void RAND_clean(csprng *rng)
{
    /* kill internal state */
    int i;
    rng->pool_ptr=rng->rndptr=0;
    for (i=0; i<32; i++) rng->pool[i]=0;
    for (i=0; i<NK; i++) rng->ira[i]=0;
    rng->borrow=0;
}

/* SU= 8, get random byte */
int RAND_byte(csprng *rng)
{
    int r;
    r=rng->pool[rng->pool_ptr++];
    if (rng->pool_ptr>=32) fill_pool(rng);
    return (r&0xff);
}

/* test main program */
/*
#include <stdio.h>
#include <string.h>

void main()
{
    int i;
    char raw[256];
    csprng rng;

	RAND_clean(&rng);


	for (i=0;i<256;i++) raw[i]=(char)i;
    RAND_seed(&rng,256,raw);

	for (i=0;i<1000;i++)
		printf("%02x ",(unsigned char)RAND_byte(&rng));
}

*/
