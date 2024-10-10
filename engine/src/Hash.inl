
#define jenkinshash_mix(a, b, c) { \
	a -= b; a -= c; a ^= (c >> 13); \
	b -= c; b -= a; b ^= (a << 8); \
	c -= a; c -= b; c ^= (b >> 13); \
	a -= b; a -= c; a ^= (c >> 12);  \
	b -= c; b -= a; b ^= (a << 16); \
	c -= a; c -= b; c ^= (b >> 5); \
	a -= b; a -= c; a ^= (c >> 3);  \
	b -= c; b -= a; b ^= (a << 10); \
	c -= a; c -= b; c ^= (b >> 15); \
}

inline u32 JenkinsHash(const void *key, size_t keySize)
{
	// Hash in 12 byte blocks.
	u32 a, b, c, len;
	const u8* k = reinterpret_cast<const u8 *>(key);
	len = u32(keySize);
	a = b = 0x9e3779b9;
	c = 0;
	while(len >= 12) {
		a += (u32(k[0]) + (u32(k[1]) << 8) + (u32(k[2]) << 16) + (u32(k[3]) << 24));
		b += (u32(k[4]) + (u32(k[5]) << 8) + (u32(k[6]) << 16) + (u32(k[7]) << 24));
		c += (u32(k[8]) + (u32(k[9]) << 8) + (u32(k[10]) << 16) + (u32(k[11]) << 24));
		jenkinshash_mix(a, b, c);
		k += 12;
		len -= 12;
	}

	// Trailing 11 bytes.
	c += u32(keySize);
	switch(len) {
		case 11: c += u32(k[10]) << 24;
		case 10: c += u32(k[9]) << 16;
		case 9: c += u32(k[8]) << 8;
		case 8: b += u32(k[7]) << 24;
		case 7: b += u32(k[6]) << 16;
		case 6: b += u32(k[5]) << 8;
		case 5: b += u32(k[4]);
		case 4: a += u32(k[3]) << 24;
		case 3: a += u32(k[2]) << 16;
		case 2: a += u32(k[1]) << 8;
		case 1: a += u32(k[0]);
	}
	jenkinshash_mix(a, b, c);
	return c;
}


template<class KEY> u32 JenkinsHash(const KEY &key)
{
	return JenkinsHash(&key, sizeof(key));
}


inline u32 JenkinsHash(u32 key)
{
	u32 a, b, c;
	a = b = 0x9e3779b9;
	a += key;
	c = 4;
	jenkinshash_mix(a, b, c);
	return c;
}


inline u32 JenkinsHash(const char *key)
{
	return JenkinsHash(key, strlen(key));
}


inline u32 JenkinsHash(char *key)
{
	return JenkinsHash(key, strlen(key));
}