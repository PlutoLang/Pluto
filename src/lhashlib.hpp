#include <cstdint>


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
