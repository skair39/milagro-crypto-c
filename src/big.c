/**
 * @file big.c
 * @author Mike Scott
 * @date 19th May 2015
 * @brief AMCL basic functions for BIG type
 *
 * LICENSE
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

/* AMCL basic functions for BIG type */
/* SU=m, SU is Stack Usage */

#include "amcl.h"

/* Calculates x*y+c+*r */

#ifdef dchunk

/* Method required to calculate x*y+c+r, bottom half in r, top half returned */
chunk muladd(chunk x,chunk y,chunk c,chunk *r)
{
    dchunk prod=(dchunk)x*y+c+*r;
    *r=(chunk)prod&BMASK;
    return (chunk)(prod>>BASEBITS);
}

#else

/* No integer type available that can store double the wordlength */
/* accumulate partial products */
/* Method required to calculate x*y+c+r, bottom half in r, top half returned */
chunk muladd(chunk x,chunk y,chunk c,chunk *r)
{
    chunk x0,x1,y0,y1;
    chunk bot,top,mid,carry;
    x0=x&HMASK;
    x1=(x>>HBITS);
    y0=y&HMASK;
    y1=(y>>HBITS);
    bot=x0*y0;
    top=x1*y1;
    mid=x0*y1+x1*y0;
    x0=mid&HMASK1;
    x1=(mid>>HBITS1);
    bot+=x0<<HBITS;
    bot+=*r;
    bot+=c;

#if HDIFF==1
    bot+=(top&HDIFF)<<(BASEBITS-1);
    top>>=HDIFF;
#endif

    top+=x1;
    carry=bot>>BASEBITS;
    bot&=BMASK;
    top+=carry;

    *r=bot;
    return top;
}

#endif

/*

// Alternative non Standard Solution required if no type available that can store double the wordlength
// The use of compiler intrinsics is permitted


#if CHUNK==64
#ifdef _WIN64
#include <intrin.h>

static INLINE chunk muladd(chunk x,chunk y,chunk c,chunk *r)
{
	chunk t,e;
	uchunk b;
	b=_mul128(x,y,&t);
	e=c+*r;
	b+=e;
// make correction for possible carry to top half
	if (e<0)
		t-=(b>e);
	else
		t+=(b<e);

	*r=b&MASK;
	return (chunk)((t<<(CHUNK-BASEBITS)) | (b>>BASEBITS));
}

#endif
#endif

*/

/* Tests for BIG equal to zero */
int BIG_iszilch(BIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        if (a[i]!=0) return 0;
    return 1;
}

/* Tests for DBIG equal to zero */
int BIG_diszilch(DBIG a)
{
    int i;
    for (i=0; i<DNLEN; i++)
        if (a[i]!=0) return 0;
    return 1;
}

/* SU= 56, Outputs a BIG number to the console */
void BIG_output(BIG a)
{
    BIG b;
    int i,len;
    len=BIG_nbits(a);
    if (len%4==0) len/=4;
    else
    {
        len/=4;
        len++;
    }
    if (len<MODBYTES*2) len=MODBYTES*2;

    for (i=len-1; i>=0; i--)
    {
        BIG_copy(b,a);
        BIG_shr(b,i*4);
        printf("%01x",(unsigned int) b[0]&15);
    }
}

/* SU= 16, Outputs a BIG number to the console in raw form (for debugging) */
void BIG_rawoutput(BIG a)
{
    int i;
    printf("(");
    for (i=0; i<NLEN-1; i++)
#if CHUNK==64
        printf("%"PRId64",",(uint64_t) a[i]);
    printf("%"PRId64")",(uint64_t) a[NLEN-1]);
#else
        printf("%x,",(unsigned int) a[i]);
    printf("%x)",(unsigned int) a[NLEN-1]);
#endif
}
/*
void BIG_rawdoutput(DBIG a)
{
	int i;
	printf("(");
	for (i=0;i<DNLEN-1;i++)
#if CHUNK==64
	  printf("%llx,",(long long unsigned int) a[i]);
	printf("%llx)",(long long unsigned int) a[DNLEN-1]);
#else
	  printf("%x,",(unsigned int) a[i]);
	printf("%x)",(unsigned int) a[NLEN-1]);
#endif
}
*/
/* Conditional constant time swap of two BIG numbers */
void BIG_cswap(BIG a,BIG b,int d)
{
    int i;
    chunk t,c=d;
    c=~(c-1);
#ifdef DEBUG_NORM
    for (i=0; i<=NLEN; i++)
#else
    for (i=0; i<NLEN; i++)
#endif
    {
        t=c&(a[i]^b[i]);
        a[i]^=t;
        b[i]^=t;
    }
}

/* Conditional copy of BIG number */
void BIG_cmove(BIG f,BIG g,int d)
{
    int i;
    chunk b=(chunk)-d;
#ifdef DEBUG_NORM
    for (i=0; i<=NLEN; i++)
#else
    for (i=0; i<NLEN; i++)
#endif
    {
        f[i]^=(f[i]^g[i])&b;
    }
}

/* SU= 64, Convert from BIG number to byte array */
void BIG_toBytes(char *b,BIG a)
{
    int i;
    BIG c;
    BIG_norm(a);
    BIG_copy(c,a);
    for (i=MODBYTES-1; i>=0; i--)
    {
        b[i]=c[0]&0xff;
        BIG_fshr(c,8);
    }
}

/* SU= 16, Convert to BIG number from byte array */
void BIG_fromBytes(BIG a,char *b)
{
    int i;
    BIG_zero(a);
    for (i=0; i<MODBYTES; i++)
    {
        BIG_fshl(a,8);
        a[0]+=(int)(unsigned char)b[i];
        //BIG_inc(a,(int)(unsigned char)b[i]); BIG_norm(a);
    }
#ifdef DEBUG_NORM
    a[NLEN]=0;
#endif
}

/* Convert to BIG number from byte array of given length */
void BIG_fromBytesLen(BIG a,char *b,int s)
{
    int i,len=s;
    BIG_zero(a);

    if (s>MODBYTES) s=MODBYTES;
    for (i=0; i<len; i++)
    {
        BIG_fshl(a,8);
        a[0]+=(int)(unsigned char)b[i];
    }
#ifdef DEBUG_NORM
    a[NLEN]=0;
#endif
}

/* Convert to DBIG number from byte array of given length */
void BIG_dfromBytesLen(DBIG a,char *b,int s)
{
    int i,len=s;
    BIG_dzero(a);

    for (i=0; i<len; i++)
    {
        BIG_dshl(a,8);
        a[0]+=(int)(unsigned char)b[i];
    }
#ifdef DEBUG_NORM
    a[NLEN]=0;
#endif
}

/* SU= 88, Outputs a DBIG number to the console */
void BIG_doutput(DBIG a)
{
    DBIG b;
    int i,len;
    BIG_dnorm(a);
    len=BIG_dnbits(a);
    if (len%4==0) len/=4;
    else
    {
        len/=4;
        len++;
    }

    for (i=len-1; i>=0; i--)
    {
        BIG_dcopy(b,a);
        BIG_dshr(b,i*4);
        printf("%01x",(unsigned int) b[0]&15);
    }
}

/* Copy b=a, Copy BIG to another BIG */
void BIG_copy(BIG b,BIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        b[i]=a[i];
#ifdef DEBUG_NORM
    b[NLEN]=a[NLEN];
#endif
}

/* Copy from ROM b=a, Copy BIG from Read-Only Memory to a BIG */
void BIG_rcopy(BIG b,const BIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        b[i]=a[i];
#ifdef DEBUG_NORM
    b[NLEN]=0;
#endif
}

/* double length DBIG copy b=a */
void BIG_dcopy(DBIG b,DBIG a)
{
    int i;
    for (i=0; i<DNLEN; i++)
        b[i]=a[i];
#ifdef DEBUG_NORM
    b[DNLEN]=a[DNLEN];
#endif
}

/* Copy BIG to lower half of DBIG */
void BIG_dscopy(DBIG b,BIG a)
{
    int i;
    for (i=0; i<NLEN-1; i++)
        b[i]=a[i];

    b[NLEN-1]=a[NLEN-1]&BMASK; /* top word normalized */
    b[NLEN]=a[NLEN-1]>>BASEBITS;

    for (i=NLEN+1; i<DNLEN; i++) b[i]=0;
#ifdef DEBUG_NORM
    b[DNLEN]=a[NLEN];
#endif
}

/* Copy BIG to upper half of DBIG */
void BIG_dsucopy(DBIG b,BIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        b[i]=0;
    for (i=NLEN; i<DNLEN; i++)
        b[i]=a[i-NLEN];
#ifdef DEBUG_NORM
    b[DNLEN]=a[NLEN];
#endif
}

/* Copy lower half of DBIG to a BIG */
void BIG_sdcopy(BIG b,DBIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        b[i]=a[i];
#ifdef DEBUG_NORM
    b[NLEN]=a[DNLEN];
#endif
}

/* Copy upper half of DBIG to a BIG */
void BIG_sducopy(BIG b,DBIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        b[i]=a[i+NLEN];
#ifdef DEBUG_NORM
    b[NLEN]=a[DNLEN];
#endif
}

/* Set BIG to zero */
void BIG_zero(BIG a)
{
    int i;
    for (i=0; i<NLEN; i++)
        a[i]=0;
#ifdef DEBUG_NORM
    a[NLEN]=0;
#endif
}

/* Set DBIG to zero */
void BIG_dzero(DBIG a)
{
    int i;
    for (i=0; i<DNLEN; i++)
        a[i]=0;
#ifdef DEBUG_NORM
    a[DNLEN]=0;
#endif
}

/* Set BIG to one (unity) */
void BIG_one(BIG a)
{
    int i;
    a[0]=1;
    for (i=1; i<NLEN; i++)
        a[i]=0;
#ifdef DEBUG_NORM
    a[NLEN]=0;
#endif
}

/* SU= 8, Set BIG to sum of two BIGs - output not normalised */
void BIG_add(BIG c,BIG a,BIG b)
{
    int i;
    for (i=0; i<NLEN; i++)
        c[i]=a[i]+b[i];
#ifdef DEBUG_NORM
    c[NLEN]=a[NLEN]+b[NLEN]+1;
    if (c[NLEN]>=NEXCESS) printf("add problem - digit overflow %lu\n",c[NLEN]);
#endif
}

/* Increment BIG by a small integer - output not normalised */
void BIG_inc(BIG c,int d)
{
    BIG_norm(c);
    c[0]+=(chunk)d;
#ifdef DEBUG_NORM
    c[NLEN]=1;
#endif
}

/* SU= 8, Set BIG to difference of two BIGs */
void BIG_sub(BIG c,BIG a,BIG b)
{
    int i;
    for (i=0; i<NLEN; i++)
        c[i]=a[i]-b[i];
#ifdef DEBUG_NORM
    c[NLEN]=a[NLEN]+b[NLEN]+1;
    if (c[NLEN]>=NEXCESS) printf("sub problem - digit overflow %d\n",c[NLEN]);
#endif
}

/* SU= 8, Set DBIG to difference of two DBIGs */
void BIG_dsub(DBIG c,DBIG a,DBIG b)
{
    int i;
    for (i=0; i<DNLEN; i++)
        c[i]=a[i]-b[i];
#ifdef DEBUG_NORM
    c[DNLEN]=a[DNLEN]+b[DNLEN]+1;
    if (c[DNLEN]>=NEXCESS) printf("sub problem - digit overflow %d\n",c[DNLEN]);
#endif
}


/* Decrement BIG by a small integer - output not normalised */
void BIG_dec(BIG c,int d)
{
    BIG_norm(c);
    c[0]-=(chunk)d;
#ifdef DEBUG_NORM
    c[NLEN]=1;
#endif
}

/* Multiply BIG by a small integer - output not normalised. r=a*c by c<=NEXCESS */
void BIG_imul(BIG r,BIG a,int c)
{
    int i;
    for (i=0; i<NLEN; i++) r[i]=a[i]*c;
#ifdef DEBUG_NORM
    r[NLEN]=(a[NLEN]+1)*c-1;
    if (r[NLEN]>=NEXCESS) printf("int mul problem - digit overflow %d\n",r[NLEN]);
#endif
}

/* SU= 24, Multiply BIG by not-so-small small integer - output normalised. r=a*c by larger integer - c<=FEXCESS */
chunk BIG_pmul(BIG r,BIG a,int c)
{
    int i;
    chunk ak,carry=0;
    BIG_norm(a);
    for (i=0; i<NLEN; i++)
    {
        ak=a[i];
        r[i]=0;
        carry=muladd(ak,(chunk)c,carry,&r[i]);
    }
#ifdef DEBUG_NORM
    r[NLEN]=0;
#endif
    return carry;
}

/* SU= 16, Divide BIG by 3 - output normalised */
int BIG_div3(BIG r)
{
    int i;
    chunk ak,base,carry=0;
    BIG_norm(r);
    base=((chunk)1<<BASEBITS);
    for (i=NLEN-1; i>=0; i--)
    {
        ak=(carry*base+r[i]);
        r[i]=ak/3;
        carry=ak%3;
    }
    return (int)carry;
}

/* SU= 24. Multiply BIG by even bigger small integer resulting in a DBIG - output normalised. c=a*b by even larger integer b>FEXCESS, resulting in DBIG */
void BIG_pxmul(DBIG c,BIG a,int b)
{
    int j;
    chunk carry;
    BIG_dzero(c);
    carry=0;
    for (j=0; j<NLEN; j++)
        carry=muladd(a[j],(chunk)b,carry,&c[j]);
    c[NLEN]=carry;
#ifdef DEBUG_NORM
    c[DNLEN]=0;
#endif
}

/* SU= 72, Multiply BIG by another BIG resulting in DBIG - inputs normalised and output normalised */
void BIG_mul(DBIG c,BIG a,BIG b)
{
    int i;
#ifdef dchunk
    dchunk t,co;
    dchunk s;
    dchunk d[NLEN];
    int k;
#endif

    /* change here - a and b MUST be normed on input */

//	BIG_norm(a);  /* needed here to prevent overflow from addition of partial products */
//	BIG_norm(b);

    /* Faster to Combafy it.. Let the compiler unroll the loops! */

#ifdef COMBA

    /* faster pseudo-Karatsuba method */

    for (i=0; i<NLEN; i++)
        d[i]=(dchunk)a[i]*b[i];

    s=d[0];
    t=s;
    c[0]=(chunk)t&BMASK;
    co=t>>BASEBITS;

    for (k=1; k<NLEN; k++)
    {
        s+=d[k];
        t=co+s;
        for (i=k; i>=1+k/2; i--) t+=(dchunk)(a[i]-a[k-i])*(b[k-i]-b[i]);
        c[k]=(chunk)t&BMASK;
        co=t>>BASEBITS;
    }
    for (k=NLEN; k<2*NLEN-1; k++)
    {
        s-=d[k-NLEN];
        t=co+s;
        for (i=NLEN-1; i>=1+k/2; i--) t+=(dchunk)(a[i]-a[k-i])*(b[k-i]-b[i]);
        c[k]=(chunk)t&BMASK;
        co=t>>BASEBITS;
    }
    c[2*NLEN-1]=(chunk)co;
    /*
    	for (i=0;i<NLEN;i++)
    		d[i]=(dchunk)a[i]*b[i];

    	s[0]=d[0];
    	for (i=1;i<NLEN;i++)
    		s[i]=s[i-1]+d[i];

    	s[2*NLEN-2]=d[NLEN-1];
    	for (i=2*NLEN-3;i>=NLEN;i--)
    		s[i]=s[i+1]+d[i-NLEN+1];

    	c[0]=s[0]&BMASK; co=s[0]>>BASEBITS;

    	for (j=1;j<NLEN;j++)
    	{
    		t=co+s[j];
    		for (k=j,i=0;i<k;i++,k-- )
    			t+=(dchunk)(a[i]-a[k])*(b[k]-b[i]);
    		c[j]=(chunk)t&BMASK; co=t>>BASEBITS;
    	}

    	for (j=NLEN;j<2*NLEN-2;j++)
    	{
    		t=co+s[j];
    		for (k=NLEN-1,i=j-NLEN+1;i<k;i++,k--)
    			t+=(dchunk)(a[i]-a[k])*(b[k]-b[i]);
    		c[j]=(chunk)t&BMASK; co=t>>BASEBITS;
    	}

    	t=(dchunk)s[2*NLEN-2]+co;
    	c[2*NLEN-2]=(chunk)t&BMASK; co=t>>BASEBITS;
    	c[2*NLEN-1]=(chunk)co;
    */
    /*
    	t=(dchunk)a[0]*b[0];
    	c[0]=(chunk)t&BMASK; co=t>>BASEBITS;
    	t=(dchunk)a[1]*b[0]+(dchunk)a[0]*b[1]+co;
    	c[1]=(chunk)t&BMASK; co=t>>BASEBITS;

    	for (j=2;j<NLEN;j++)
    	{
    		t=co; for (i=0;i<=j;i++) t+=(dchunk)a[j-i]*b[i];
    		c[j]=(chunk)t&BMASK; co=t>>BASEBITS;
    	}

    	for (j=NLEN;j<DNLEN-2;j++)
    	{
    		t=co; for (i=j-NLEN+1;i<NLEN;i++) t+=(dchunk)a[j-i]*b[i];
    		c[j]=(chunk)t&BMASK; co=t>>BASEBITS;
    	}

    	t=(dchunk)a[NLEN-1]*b[NLEN-1]+co;
    	c[DNLEN-2]=(chunk)t&BMASK; co=t>>BASEBITS;
    	c[DNLEN-1]=(chunk)co;
    */
#else
    int j;
    chunk carry;
    BIG_dzero(c);
    for (i=0; i<NLEN; i++)
    {
        carry=0;
        for (j=0; j<NLEN; j++)
            carry=muladd(a[i],b[j],carry,&c[i+j]);

        c[NLEN+i]=carry;
    }

#endif

#ifdef DEBUG_NORM
    c[DNLEN]=0;
#endif
}

/* SU= 40, Multiply BIG by another BIG resulting in another BIG - inputs normalised and output normalised. */
void BIG_smul(BIG c,BIG a,BIG b)
{
    int i,j;
    chunk carry;
    BIG_norm(a);
    BIG_norm(b);

    BIG_zero(c);
    for (i=0; i<NLEN; i++)
    {
        carry=0;
        for (j=0; j<NLEN; j++)
        {
            if (i+j<NLEN)
                carry=muladd(a[i],b[j],carry,&c[i+j]);
        }
    }
#ifdef DEBUG_NORM
    c[NLEN]=0;
#endif

}

/* SU= 80, Square BIG resulting in a DBIG - input normalised and output normalised */
void BIG_sqr(DBIG c,BIG a)
{
    int i,j,last;
#ifdef dchunk
    dchunk t,co;
#endif

    /* change here - a MUST be normed on input */
//	BIG_norm(a);

    /* Note 2*a[i] in loop below and extra addition */

#ifdef COMBA

    t=(dchunk)a[0]*a[0];
    c[0]=(chunk)t&BMASK;
    co=t>>BASEBITS;
    t=(dchunk)a[1]*a[0];
    t+=t;
    t+=co;
    c[1]=(chunk)t&BMASK;
    co=t>>BASEBITS;

    last=NLEN-NLEN%2;
    for (j=2; j<last; j+=2)
    {
        t=(dchunk)a[j]*a[0];
        for (i=1; i<(j+1)/2; i++) t+=(dchunk)a[j-i]*a[i];
        t+=t;
        t+=co;
        t+=(dchunk)a[j/2]*a[j/2];
        c[j]=(chunk)t&BMASK;
        co=t>>BASEBITS;
        t=(dchunk)a[j+1]*a[0];
        for (i=1; i<(j+2)/2; i++) t+=(dchunk)a[j+1-i]*a[i];
        t+=t;
        t+=co;
        c[j+1]=(chunk)t&BMASK;
        co=t>>BASEBITS;
    }
    j=last;
#if NLEN%2==1
    t=(dchunk)a[j]*a[0];
    for (i=1; i<(j+1)/2; i++) t+=(dchunk)a[j-i]*a[i];
    t+=t;
    t+=co;
    t+=(dchunk)a[j/2]*a[j/2];
    c[j]=(chunk)t&BMASK;
    co=t>>BASEBITS;
    j++;
    t=(dchunk)a[NLEN-1]*a[j-NLEN+1];
    for (i=j-NLEN+2; i<(j+1)/2; i++) t+=(dchunk)a[j-i]*a[i];
    t+=t;
    t+=co;
    c[j]=(chunk)t&BMASK;
    co=t>>BASEBITS;
    j++;
#endif
    for (; j<DNLEN-2; j+=2)
    {
        t=(dchunk)a[NLEN-1]*a[j-NLEN+1];
        for (i=j-NLEN+2; i<(j+1)/2; i++) t+=(dchunk)a[j-i]*a[i];
        t+=t;
        t+=co;
        t+=(dchunk)a[j/2]*a[j/2];
        c[j]=(chunk)t&BMASK;
        co=t>>BASEBITS;
        t=(dchunk)a[NLEN-1]*a[j-NLEN+2];
        for (i=j-NLEN+3; i<(j+2)/2; i++) t+=(dchunk)a[j+1-i]*a[i];
        t+=t;
        t+=co;
        c[j+1]=(chunk)t&BMASK;
        co=t>>BASEBITS;
    }

    t=(dchunk)a[NLEN-1]*a[NLEN-1]+co;
    c[DNLEN-2]=(chunk)t&BMASK;
    co=t>>BASEBITS;
    c[DNLEN-1]=(chunk)co;

#else
    chunk carry;
    BIG_dzero(c);
    for (i=0; i<NLEN; i++)
    {
        carry=0;
        for (j=i+1; j<NLEN; j++)
            carry=muladd(a[i],a[j],carry,&c[i+j]);
        c[NLEN+i]=carry;
    }

    for (i=0; i<DNLEN; i++) c[i]*=2;

    for (i=0; i<NLEN; i++)
        c[2*i+1]+=muladd(a[i],a[i],0,&c[2*i]);

    BIG_dnorm(c);
#endif


#ifdef DEBUG_NORM
    c[DNLEN]=0;
#endif

}

/* SU= 32, Shifts a BIG left by any number of bits - input must be normalised, output normalised */
void BIG_shl(BIG a,int k)
{
    int i;
    int n=k%BASEBITS;
    int m=k/BASEBITS;

//	a[NLEN-1]=((a[NLEN-1-m]<<n))|(a[NLEN-m-2]>>(BASEBITS-n));

    a[NLEN-1]=((a[NLEN-1-m]<<n));
    if (NLEN>=m+2) a[NLEN-1]|=(a[NLEN-m-2]>>(BASEBITS-n));

    for (i=NLEN-2; i>m; i--)
        a[i]=((a[i-m]<<n)&BMASK)|(a[i-m-1]>>(BASEBITS-n));
    a[m]=(a[0]<<n)&BMASK;
    for (i=0; i<m; i++) a[i]=0;

}

/* SU= 16, Fast shifts a BIG left by a small number of bits - input must be normalised, output will be normalised */
chunk BIG_fshl(BIG a,int n)
{
    int i;

    a[NLEN-1]=((a[NLEN-1]<<n))|(a[NLEN-2]>>(BASEBITS-n)); /* top word not masked */
    for (i=NLEN-2; i>0; i--)
        a[i]=((a[i]<<n)&BMASK)|(a[i-1]>>(BASEBITS-n));
    a[0]=(a[0]<<n)&BMASK;

    return (a[NLEN-1]>>((8*MODBYTES)%BASEBITS)); /* return excess - only used in ff.c */
}

/* SU= 32, Shifts a DBIG left by any number of bits - input must be normalised, output normalised */
void BIG_dshl(DBIG a,int k)
{
    int i;
    int n=k%BASEBITS;
    int m=k/BASEBITS;

    a[DNLEN-1]=((a[DNLEN-1-m]<<n))|(a[DNLEN-m-2]>>(BASEBITS-n));

    for (i=DNLEN-2; i>m; i--)
        a[i]=((a[i-m]<<n)&BMASK)|(a[i-m-1]>>(BASEBITS-n));
    a[m]=(a[0]<<n)&BMASK;
    for (i=0; i<m; i++) a[i]=0;

}

/* SU= 32, Shifts a BIG right by any number of bits - input must be normalised, output normalised */
void BIG_shr(BIG a,int k)
{
    int i;
    int n=k%BASEBITS;
    int m=k/BASEBITS;
    for (i=0; i<NLEN-m-1; i++)
        a[i]=(a[m+i]>>n)|((a[m+i+1]<<(BASEBITS-n))&BMASK);
    if (NLEN>m)  a[NLEN-m-1]=a[NLEN-1]>>n;
    for (i=NLEN-m; i<NLEN; i++) a[i]=0;

}

/* SU= 16, Fast shifts a BIG right by a small number of bits - input must be normalised, output will be normalised */
chunk BIG_fshr(BIG a,int k)
{
    int i;
    chunk r=a[0]&(((chunk)1<<k)-1); /* shifted out part */
    for (i=0; i<NLEN-1; i++)
        a[i]=(a[i]>>k)|((a[i+1]<<(BASEBITS-k))&BMASK);
    a[NLEN-1]=a[NLEN-1]>>k;
    return r;
}

/* SU= 32, Shifts a DBIG right by any number of bits - input must be normalised, output normalised */
void BIG_dshr(DBIG a,int k)
{
    int i;
    int n=k%BASEBITS;
    int m=k/BASEBITS;
    for (i=0; i<DNLEN-m-1; i++)
        a[i]=(a[m+i]>>n)|((a[m+i+1]<<(BASEBITS-n))&BMASK);
    a[DNLEN-m-1]=a[DNLEN-1]>>n;
    for (i=DNLEN-m; i<DNLEN; i++ ) a[i]=0;
}

/* SU= 24, Splits a DBIG into two BIGs - input must be normalised, outputs normalised */
chunk BIG_split(BIG t,BIG b,DBIG d,int n)
{
    int i;
    chunk nw,carry=0;
    int m=n%BASEBITS;
//	BIG_dnorm(d);

    if (m==0)
    {
        for (i=0; i<NLEN; i++) b[i]=d[i];
        if (t!=b)
        {
            for (i=NLEN; i<2*NLEN; i++) t[i-NLEN]=d[i];
            carry=t[NLEN-1]>>BASEBITS;
            t[NLEN-1]=t[NLEN-1]&BMASK; /* top word normalized */
        }
        return carry;
    }

    for (i=0; i<NLEN-1; i++) b[i]=d[i];

    b[NLEN-1]=d[NLEN-1]&(((chunk)1<<m)-1);

    if (t!=b)
    {
        carry=(d[DNLEN-1]<<(BASEBITS-m));
        for (i=DNLEN-2; i>=NLEN-1; i--)
        {
            nw=(d[i]>>m)|carry;
            carry=(d[i]<<(BASEBITS-m))&BMASK;
            t[i-NLEN+1]=nw;
        }
    }
#ifdef DEBUG_NORM
    t[NLEN]=0;
    b[NLEN]=0;
#endif
    return carry;
}

/* you gotta keep the sign of carry! Look - no branching! */
/* Note that sign bit is needed to disambiguate between +ve and -ve values */
/* Normalizes a BIG number - output normalised, force all digits < 2^BASEBITS */
chunk BIG_norm(BIG a)
{
    int i;
    chunk d,carry=0;
    for (i=0; i<NLEN-1; i++)
    {
        d=a[i]+carry;
        a[i]=d&BMASK;
        carry=d>>BASEBITS;
    }
    a[NLEN-1]=(a[NLEN-1]+carry);

#ifdef DEBUG_NORM
    a[NLEN]=0;
#endif
    return (a[NLEN-1]>>((8*MODBYTES)%BASEBITS));  /* only used in ff.c */
}

/* Normalizes a DBIG number - output normalised */
void BIG_dnorm(DBIG a)
{
    int i;
    chunk d,carry=0;
    for (i=0; i<DNLEN-1; i++)
    {
        d=a[i]+carry;
        a[i]=d&BMASK;
        carry=d>>BASEBITS;
    }
    a[DNLEN-1]=(a[DNLEN-1]+carry);
#ifdef DEBUG_NORM
    a[DNLEN]=0;
#endif
}

/* Compare a and b. Return 1 for a>b, -1 for a<b, 0 for a==b */
/* a and b MUST be normalised before call */
int BIG_comp(BIG a,BIG b)
{
    int i;
    for (i=NLEN-1; i>=0; i--)
    {
        if (a[i]==b[i]) continue;
        if (a[i]>b[i]) return 1;
        else  return -1;
    }
    return 0;
}

/* Compares two DBIG numbers. Inputs must be normalised externally */
int BIG_dcomp(DBIG a,DBIG b)
{
    int i;
    for (i=DNLEN-1; i>=0; i--)
    {
        if (a[i]==b[i]) continue;
        if (a[i]>b[i]) return 1;
        else  return -1;
    }
    return 0;
}

/* SU= 8, Calculate number of bits in a BIG - output normalised */
int BIG_nbits(BIG a)
{
    int bts,k=NLEN-1;
    chunk c;
    BIG_norm(a);
    while (k>=0 && a[k]==0) k--;
    if (k<0) return 0;
    bts=BASEBITS*k;
    c=a[k];
    while (c!=0)
    {
        c/=2;
        bts++;
    }
    return bts;
}

/* SU= 8, Calculate number of bits in a DBIG - output normalised */
int BIG_dnbits(DBIG a)
{
    int bts,k=DNLEN-1;
    chunk c;
    BIG_dnorm(a);
    while (k>=0 && a[k]==0) k--;
    if (k<0) return 0;
    bts=BASEBITS*k;
    c=a[k];
    while (c!=0)
    {
        c/=2;
        bts++;
    }
    return bts;
}

/* SU= 16, Reduce x mod n - input and output normalised */
void BIG_mod(BIG b,BIG c)
{
    int k=0;

    BIG_norm(b);
    if (BIG_comp(b,c)<0)
        return;
    do
    {
        BIG_fshl(c,1);
        k++;
    }
    while (BIG_comp(b,c)>=0);

    while (k>0)
    {
        BIG_fshr(c,1);
        if (BIG_comp(b,c)>=0)
        {
            BIG_sub(b,b,c);
            BIG_norm(b);
        }
        k--;
    }
}

/* SU= 96, Set a=b mod c, b is destroyed. Slow but rarely used. */
void BIG_dmod(BIG a,DBIG b,BIG c)
{
    int k=0;
    DBIG m;
    BIG_dnorm(b);
    BIG_dscopy(m,c);

    if (BIG_dcomp(b,m)<0)
    {
        BIG_sdcopy(a,b);
        return;
    }

    do
    {
        BIG_dshl(m,1);
        k++;
    }
    while (BIG_dcomp(b,m)>=0);

    while (k>0)
    {
        BIG_dshr(m,1);
        if (BIG_dcomp(b,m)>=0)
        {
            BIG_dsub(b,b,m);
            BIG_dnorm(b);
        }
        k--;
    }
    BIG_sdcopy(a,b);
}

/* SU= 136, x=y/n - output normalised. Slow but rarely used. */
void BIG_ddiv(BIG a,DBIG b,BIG c)
{
    int k=0;
    DBIG m;
    BIG e;
    BIG_dnorm(b);
    BIG_dscopy(m,c);

    BIG_zero(a);
    BIG_zero(e);
    BIG_inc(e,1);

    while (BIG_dcomp(b,m)>=0)
    {
        BIG_fshl(e,1);
        BIG_dshl(m,1);
        k++;
    }

    while (k>0)
    {
        BIG_dshr(m,1);
        BIG_fshr(e,1);
        if (BIG_dcomp(b,m)>=0)
        {
            BIG_add(a,a,e);
            BIG_norm(a);
            BIG_dsub(b,b,m);
            BIG_dnorm(b);
        }
        k--;
    }
}

/* SU= 136, Divide x by n - output normalised */
void BIG_sdiv(BIG a,BIG c)
{
    int k=0;
    BIG m,e,b;
    BIG_norm(a);
    BIG_copy(b,a);
    BIG_copy(m,c);

    BIG_zero(a);
    BIG_zero(e);
    BIG_inc(e,1);

    while (BIG_comp(b,m)>=0)
    {
        BIG_fshl(e,1);
        BIG_fshl(m,1);
        k++;
    }

    while (k>0)
    {
        BIG_fshr(m,1);
        BIG_fshr(e,1);
        if (BIG_comp(b,m)>=0)
        {
            BIG_add(a,a,e);
            BIG_norm(a);
            BIG_sub(b,b,m);
            BIG_norm(b);
        }
        k--;
    }
}

/* Return parity of BIG, that is the least significant bit */
int BIG_parity(BIG a)
{
    return a[0]%2;
}

/* SU= 16, Return i-th of BIG */
int BIG_bit(BIG a,int n)
{
    if (a[n/BASEBITS]&((chunk)1<<(n%BASEBITS))) return 1;
    else return 0;
}

/* return NAF value as +/- 1, 3 or 5. x and x3 should be normed.
nbs is number of bits processed, and nzs is number of trailing 0s detected */
/* SU= 32 */
/*
int BIG_nafbits(BIG x,BIG x3,int i,int *nbs,int *nzs)
{
	int j,r,nb;

	nb=BIG_bit(x3,i)-BIG_bit(x,i);
	*nbs=1;
	*nzs=0;
	if (nb==0) return 0;
	if (i==0) return nb;

    if (nb>0) r=1;
    else      r=(-1);

    for (j=i-1;j>0;j--)
    {
        (*nbs)++;
        r*=2;
        nb=BIG_bit(x3,j)-BIG_bit(x,j);
        if (nb>0) r+=1;
        if (nb<0) r-=1;
        if (abs(r)>5) break;
    }

	if (r%2!=0 && j!=0)
    { // backtrack
        if (nb>0) r=(r-1)/2;
        if (nb<0) r=(r+1)/2;
        (*nbs)--;
    }

    while (r%2==0)
    { // remove trailing zeros
        r/=2;
        (*nzs)++;
        (*nbs)--;
    }
    return r;
}
*/

/* SU= 16, Return least significant bits of a BIG */
int BIG_lastbits(BIG a,int n)
{
    int msk=(1<<n)-1;
    BIG_norm(a);
    return ((int)a[0])&msk;
}

/* Create a random 8*MODBYTES size  BIG from a random number generator */
void BIG_random(BIG m,csprng *rng)
{
    int i,b,j=0,r=0;
    int len=8*MODBYTES;

    BIG_zero(m);
    /* generate random BIG */
    for (i=0; i<len; i++)
    {
        if (j==0) r=RAND_byte(rng);
        else r>>=1;
        b=r&1;
        BIG_shl(m,1);
        m[0]+=b;
        j++;
        j&=7;
    }

#ifdef DEBUG_NORM
    m[NLEN]=0;
#endif
}

/* Create an unbiased random BIG from a random number generator, reduced with respect to a modulus */

void BIG_randomnum(BIG m,BIG q,csprng *rng)
{
    int i,b,j=0,r=0;
    DBIG d;
    BIG_dzero(d);
    /* generate random DBIG */
    for (i=0; i<2*MODBITS; i++)
    {
        if (j==0) r=RAND_byte(rng);
        else r>>=1;
        b=r&1;
        BIG_dshl(d,1);
        d[0]+=b;
        j++;
        j&=7;
    }
    /* reduce modulo a BIG. Removes bias */
    BIG_dmod(m,d,q);
#ifdef DEBUG_NORM
    m[NLEN]=0;
#endif
}

/* SU= 96, Calculate x=y*z mod n */
void BIG_modmul(BIG r,BIG a,BIG b,BIG m)
{
    DBIG d;
    BIG_mod(a,m);
    BIG_mod(b,m);
//BIG_norm(a); BIG_norm(b);
    BIG_mul(d,a,b);
    BIG_dmod(r,d,m);
}

/* SU= 88, Calculate x=y^2 mod n */
void BIG_modsqr(BIG r,BIG a,BIG m)
{
    DBIG d;
    BIG_mod(a,m);
//BIG_norm(a);
    BIG_sqr(d,a);
    BIG_dmod(r,d,m);
}

/* SU= 16, Calculate x=-y mod n */
void BIG_modneg(BIG r,BIG a,BIG m)
{
    BIG_mod(a,m);
    BIG_sub(r,m,a);
    BIG_mod(r,m);
}

/* SU= 136, Calculate x=y/z mod n */
void BIG_moddiv(BIG r,BIG a,BIG b,BIG m)
{
    DBIG d;
    BIG z;
    BIG_mod(a,m);
    BIG_invmodp(z,b,m);
//BIG_norm(a); BIG_norm(z);
    BIG_mul(d,a,z);
    BIG_dmod(r,d,m);
}

/* SU= 216, Calculate jacobi Symbol (x/y) */
int BIG_jacobi(BIG a,BIG p)
{
    int n8,k,m=0;

    BIG t,x,n,zilch,one;
    BIG_one(one);
    BIG_zero(zilch);
    if (BIG_parity(p)==0 || BIG_comp(a,zilch)==0 || BIG_comp(p,one)<=0) return 0;
    BIG_norm(a);
    BIG_copy(x,a);
    BIG_copy(n,p);
    BIG_mod(x,p);

    while (BIG_comp(n,one)>0)
    {
        if (BIG_comp(x,zilch)==0) return 0;
        n8=BIG_lastbits(n,3);
        k=0;
        while (BIG_parity(x)==0)
        {
            k++;
            BIG_shr(x,1);
        }
        if (k%2==1) m+=(n8*n8-1)/8;
        m+=(n8-1)*(BIG_lastbits(x,2)-1)/4;
        BIG_copy(t,n);

        BIG_mod(t,x);
        BIG_copy(n,x);
        BIG_copy(x,t);
        m%=2;

    }
    if (m==0) return 1;
    else return -1;
}

/* SU= 240, Calculate x=1/y mod n */
void BIG_invmodp(BIG r,BIG a,BIG p)
{
    BIG u,v,x1,x2,t,one;
    BIG_mod(a,p);
    BIG_copy(u,a);
    BIG_copy(v,p);
    BIG_one(one);
    BIG_copy(x1,one);
    BIG_zero(x2);

    while (BIG_comp(u,one)!=0 && BIG_comp(v,one)!=0)
    {
        while (BIG_parity(u)==0)
        {
            BIG_shr(u,1);
            if (BIG_parity(x1)!=0)
            {
                BIG_add(x1,p,x1);
                BIG_norm(x1);
            }
            BIG_shr(x1,1);
        }
        while (BIG_parity(v)==0)
        {
            BIG_shr(v,1);
            if (BIG_parity(x2)!=0)
            {
                BIG_add(x2,p,x2);
                BIG_norm(x2);
            }
            BIG_shr(x2,1);
        }
        if (BIG_comp(u,v)>=0)
        {
            BIG_sub(u,u,v);
            BIG_norm(u);
            if (BIG_comp(x1,x2)>=0) BIG_sub(x1,x1,x2);
            else
            {
                BIG_sub(t,p,x2);
                BIG_add(x1,x1,t);
            }
            BIG_norm(x1);
        }
        else
        {
            BIG_sub(v,v,u);
            BIG_norm(v);
            if (BIG_comp(x2,x1)>=0) BIG_sub(x2,x2,x1);
            else
            {
                BIG_sub(t,p,x1);
                BIG_add(x2,x2,t);
            }
            BIG_norm(x2);
        }
    }
    if (BIG_comp(u,one)==0)
        BIG_copy(r,x1);
    else
        BIG_copy(r,x2);
}

/* Calculate x=x mod 2^m  */
void BIG_mod2m(BIG x,int m)
{
    int i,wd,bt;
    chunk msk;
//	if (m>=MODBITS) return;
    wd=m/BASEBITS;
    bt=m%BASEBITS;
    msk=((chunk)1<<bt)-1;
    x[wd]&=msk;
    for (i=wd+1; i<NLEN; i++) x[i]=0;
}

