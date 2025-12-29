#include <cstdint>
#include <memory.h>


//-----------------------------------------------------------------------------
// MurmurHash1Aligned, by Austin Appleby
// Same algorithm as MurmurHash1, but only does aligned reads - should be safer
// on certain platforms. 
// Performance should be equal to or better than the simple version.
// MurmurHash was written by Austin Appleby, and is placed in the public domain
// The author hereby disclaims copyright to this source code.
//-----------------------------------------------------------------------------
unsigned int MurmurHash1Aligned(const void* key, int len, unsigned int seed)
{
  const unsigned int m = 0xc6a4a793;
  const int r = 16;

  const unsigned char * data = (const unsigned char *)key;

  unsigned int h = seed ^ (len * m);

  int align = (uint64_t)data & 3;

  if(align && (len >= 4))
  {
    unsigned int t = 0, d = 0;

    switch(align)
    {
      case 1: 
        t |= data[2] << 16;
        [[fallthrough]];
      case 2:
        t |= data[1] << 8;
        [[fallthrough]];
      case 3:
        t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    int sl = 8 * (4-align);
    int sr = 8 * align;

    while(len >= 4)
    {
      d = *(unsigned int *)data;
      t = (t >> sr) | (d << sl);
      h += t;
      h *= m;
      h ^= h >> r;
      t = d;

      data += 4;
      len -= 4;
    }

    int pack = len < align ? len : align;

    d = 0;

    switch(pack)
    {
      case 3:
        d |= data[2] << 16;
        [[fallthrough]];
      case 2:
        d |= data[1] << 8;
        [[fallthrough]];
      case 1: 
        d |= data[0];
        [[fallthrough]];
      case 0:
        h += (t >> sr) | (d << sl);
        h *= m;
        h ^= h >> r;
    }

    data += pack;
    len -= pack;
  }
  else
  {
    while(len >= 4)
    {
      h += *(unsigned int *)data;
      h *= m;
      h ^= h >> r;

      data += 4;
      len -= 4;
    }
  }

  switch(len)
  {
    case 3: 
      h += data[2] << 16;
      [[fallthrough]];
    case 2: 
      h += data[1] << 8;
      [[fallthrough]];
    case 1:
      h += data[0];
      h *= m;
      h ^= h >> r;
  };

  h *= m;
  h ^= h >> 10;
  h *= m;
  h ^= h >> 17;

  return h;
}


#if defined(_MSC_VER)

#define BIG_CONSTANT(x) (x)

// Other compilers

#else	// defined(_MSC_VER)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

uint32_t MurmurHash2 ( const void * key, int len, uint32_t seed )
{
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.

  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  // Initialize the hash to a 'random' value

  uint32_t h = seed ^ len;

  // Mix 4 bytes at a time into the hash

  const unsigned char * data = (const unsigned char *)key;

  while(len >= 4)
  {
    uint32_t k = *(uint32_t*)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array

  switch(len)
  {
    case 3:
      h ^= data[2] << 16;
      [[fallthrough]];
    case 2:
      h ^= data[1] << 8;
      [[fallthrough]];
    case 1:
      h ^= data[0];
      h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
} 

//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms

uint64_t MurmurHash64A ( const void * key, int len, uint64_t seed )
{
  const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data++;

    k *= m; 
    k ^= k >> r; 
    k *= m; 
    
    h ^= k;
    h *= m; 
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
    case 7:
      h ^= uint64_t(data2[6]) << 48;
      [[fallthrough]];
    case 6:
      h ^= uint64_t(data2[5]) << 40;
      [[fallthrough]];
    case 5:
      h ^= uint64_t(data2[4]) << 32;
      [[fallthrough]];
    case 4:
      h ^= uint64_t(data2[3]) << 24;
      [[fallthrough]];
    case 3:
      h ^= uint64_t(data2[2]) << 16;
      [[fallthrough]];
    case 2:
      h ^= uint64_t(data2[1]) << 8;
      [[fallthrough]];
    case 1:
      h ^= uint64_t(data2[0]);
      h *= m;
  };
 
  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
} 


// 64-bit hash for 32-bit platforms

uint64_t MurmurHash64B ( const void * key, int len, uint64_t seed )
{
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  uint32_t h1 = uint32_t(seed) ^ len;
  uint32_t h2 = uint32_t(seed >> 32);

  const uint32_t * data = (const uint32_t *)key;

  while(len >= 8)
  {
    uint32_t k1 = *data++;
    k1 *= m; k1 ^= k1 >> r; k1 *= m;
    h1 *= m; h1 ^= k1;
    len -= 4;

    uint32_t k2 = *data++;
    k2 *= m; k2 ^= k2 >> r; k2 *= m;
    h2 *= m; h2 ^= k2;
    len -= 4;
  }

  if(len >= 4)
  {
    uint32_t k1 = *data++;
    k1 *= m; k1 ^= k1 >> r; k1 *= m;
    h1 *= m; h1 ^= k1;
    len -= 4;
  }

  switch(len)
  {
    case 3:
      h2 ^= ((unsigned char*)data)[2] << 16;
      [[fallthrough]];
    case 2:
      h2 ^= ((unsigned char*)data)[1] << 8;
      [[fallthrough]];
    case 1:
      h2 ^= ((unsigned char*)data)[0];
      h2 *= m;
  };

  h1 ^= h2 >> 18; h1 *= m;
  h2 ^= h1 >> 22; h2 *= m;
  h1 ^= h2 >> 17; h1 *= m;
  h2 ^= h1 >> 19; h2 *= m;

  uint64_t h = h1;

  h = (h << 32) | h2;

  return h;
} 

//-----------------------------------------------------------------------------
// MurmurHash2A, by Austin Appleby

// This is a variant of MurmurHash2 modified to use the Merkle-Damgard 
// construction. Bulk speed should be identical to Murmur2, small-key speed 
// will be 10%-20% slower due to the added overhead at the end of the hash.

// This variant fixes a minor issue where null keys were more likely to
// collide with each other than expected, and also makes the function
// more amenable to incremental implementations.

#define mmix(h,k) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

uint32_t MurmurHash2A ( const void * key, int len, uint32_t seed )
{
  const uint32_t m = 0x5bd1e995;
  const int r = 24;
  uint32_t l = len;

  const unsigned char * data = (const unsigned char *)key;

  uint32_t h = seed;

  while(len >= 4)
  {
    uint32_t k = *(uint32_t*)data;

    mmix(h,k);

    data += 4;
    len -= 4;
  }

  uint32_t t = 0;

  switch(len)
  {
    case 3:
      t ^= data[2] << 16;
      [[fallthrough]];
    case 2:
      t ^= data[1] << 8;
      [[fallthrough]];
    case 1: t ^= data[0];
  };

  mmix(h,t);
  mmix(h,l);

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}


//-----------------------------------------------------------------------------
// MurmurHashNeutral2, by Austin Appleby

// Same as MurmurHash2, but endian- and alignment-neutral.
// Half the speed though, alas.

uint32_t MurmurHashNeutral2 ( const void * key, int len, uint32_t seed )
{
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  uint32_t h = seed ^ len;

  const unsigned char * data = (const unsigned char *)key;

  while(len >= 4)
  {
    uint32_t k;

    k  = data[0];
    k |= data[1] << 8;
    k |= data[2] << 16;
    k |= data[3] << 24;

    k *= m; 
    k ^= k >> r; 
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }
  
  switch(len)
  {
    case 3:
      h ^= data[2] << 16;
      [[fallthrough]];
    case 2:
      h ^= data[1] << 8;
      [[fallthrough]];
    case 1:
      h ^= data[0];
      h *= m;
  };

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
} 

//-----------------------------------------------------------------------------
// MurmurHashAligned2, by Austin Appleby

// Same algorithm as MurmurHash2, but only does aligned reads - should be safer
// on certain platforms. 

// Performance will be lower than MurmurHash2

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }


uint32_t MurmurHashAligned2 ( const void * key, int len, uint32_t seed )
{
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  const unsigned char * data = (const unsigned char *)key;

  uint32_t h = seed ^ len;

  int align = (uint64_t)data & 3;

  if(align && (len >= 4))
  {
    // Pre-load the temp registers

    uint32_t t = 0, d = 0;

    switch(align)
    {
      case 1:
        t |= data[2] << 16;
        [[fallthrough]];
      case 2: 
        t |= data[1] << 8;
        [[fallthrough]];
      case 3:
        t |= data[0];
    }

    t <<= (8 * align);

    data += 4-align;
    len -= 4-align;

    int sl = 8 * (4-align);
    int sr = 8 * align;

    // Mix

    while(len >= 4)
    {
      d = *(uint32_t *)data;
      t = (t >> sr) | (d << sl);

      uint32_t k = t;

      MIX(h,k,m);

      t = d;

      data += 4;
      len -= 4;
    }

    // Handle leftover data in temp registers

    d = 0;

    if(len >= align)
    {
      switch(align)
      {
        case 3:
          d |= data[2] << 16;
          [[fallthrough]];
        case 2:
          d |= data[1] << 8;
          [[fallthrough]];
        case 1:
          d |= data[0];
      }

      uint32_t k = (t >> sr) | (d << sl);
      MIX(h,k,m);

      data += align;
      len -= align;

      //----------
      // Handle tail bytes

      switch(len)
      {
        case 3:
          h ^= data[2] << 16;
          [[fallthrough]];
        case 2:
          h ^= data[1] << 8;
          [[fallthrough]];
        case 1:
          h ^= data[0];
          h *= m;
      };
    }
    else
    {
      switch(len)
      {
        case 3:
          d |= data[2] << 16;
          [[fallthrough]];
        case 2:
          d |= data[1] << 8;
          [[fallthrough]];
        case 1:
          d |= data[0];
          [[fallthrough]];
        case 0:
            h ^= (t >> sr) | (d << sl);
            h *= m;
      }
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
  else
  {
    while(len >= 4)
    {
      uint32_t k = *(uint32_t *)data;

      MIX(h,k,m);

      data += 4;
      len -= 4;
    }

    //----------
    // Handle tail bytes

    switch(len)
    {
      case 3:
        h ^= data[2] << 16;
        [[fallthrough]];
      case 2:
        h ^= data[1] << 8;
        [[fallthrough]];
      case 1:
        h ^= data[0];
        h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
}

//-----------------------------------------------------------------------------


/*
  SuperFastHash By Paul Hsieh (C) 2004, 2005.  Covered under the Paul Hsieh derivative 
  license. See: 
  http://www.azillionmonkeys.com/qed/weblicense.html for license details.
  http://www.azillionmonkeys.com/qed/hash.html
*/

inline uint16_t get16bits ( const void * p )
{
  return *(const uint16_t*)p;
}

uint32_t SuperFastHash (const signed char * data, int len) {
uint32_t hash = 0, tmp;
int rem;

  if (len <= 0 || data == NULL) return 0;

  rem = len & 3;
  len >>= 2;

  /* Main loop */
  for (;len > 0; len--) {
    hash  += get16bits (data);
    tmp    = (get16bits (data+2) << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2*sizeof (uint16_t);
    hash  += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
    case 3:	hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= data[sizeof (uint16_t)] << 18;
        hash += hash >> 11;
        break;
    case 2:	hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;
}

// lookup3 by Bob Jekins, code is public domain.
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

uint32_t lookup3_impl ( const void * key, int length, uint32_t initval )
{
  uint32_t a,b,c;                                          /* internal state */

  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

  /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
  while (length > 12)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 12;
    k += 3;
  }

  switch(length)
  {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;
    case 11:
      c += k[2] & 0xffffff;
      b += k[1];
      a += k[0];
      break;
    case 10:
      c += k[2] & 0xffff;
      b += k[1];
      a += k[0];
      break;
    case 9:
      c += k[2] & 0xff;
      b += k[1];
      a += k[0];
      break;
    case 8:
      b += k[1];
      a += k[0];
      break;
    case 7:
      b += k[1] & 0xffffff;
      a += k[0];
      break;
    case 6:
      b += k[1] & 0xffff;
      a += k[0];
      break;
    case 5:
      b += k[1] & 0xff;
      a += k[0];
      break;
    case 4:
      a += k[0];
      break;
    case 3:
      a += k[0] & 0xffffff;
      break;
    case 2:
      a += k[0] & 0xffff;
      break;
    case 1:
      a += k[0] & 0xff;
      break;
    case 0: return c; 
  }

  final(a,b,c);

  return c;
}
