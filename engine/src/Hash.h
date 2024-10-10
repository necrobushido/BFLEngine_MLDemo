#pragma once

#include "types.h"

u32 JenkinsHash(const void *key, size_t keySize);

template<class KEY> u32 JenkinsHash(const KEY &key);
u32 JenkinsHash(u32 key);
u32 JenkinsHash(const char *key);
u32 JenkinsHash(char *key);


#include "Hash.inl"
