#pragma once

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <err.h>
#include <assert.h>

#include <sys/stat.h>

#include <sgx.h>
#include <sgx-user.h>
#include <sgx-kern.h>
#include <sgx-lib.h>

#define is_aligned(addr, bytes) \
    ((((uintptr_t)(const void *)(addr)) & (bytes - 1)) == 0)

uint64_t gcd64(uint64_t a, uint64_t b) {
	while (b) {
		uint64_t t = a % b;
		a = b;
		b = t;
	}
	return a;
}

uint64_t lcm64(uint64_t a, uint64_t b) {
	return (a / gcd64(a, b)) * b;
}

uint64_t mul_mod64(uint64_t a, uint64_t b, uint64_t mod) {
	uint64_t res = 0;
	a %= mod;
	while (b > 0) {
		if (b & 1)
			res = (res + a) % mod;
		a = (a * 2) % mod;
		b >>= 1;
	}
	return res;
}

uint64_t pow_mod64(uint64_t base, uint64_t exp, uint64_t mod) {
	uint64_t res = 1;
	base %= mod;
	while (exp) {
		res = mul_mod64(res, base, mod);
		exp--;
	}
	return res;
}

int64_t extgcd(int64_t a, int64_t b, int64_t *x, int64_t *y) {
	if (b == 0) {
		*x = 1;
		*y = 0;
		return a;
	}
	int64_t x1, y1;
	int64_t g = extgcd(b, a % b, &x1, &y1);
	*x = y1;
	*y = x1 - (a / b) * y1;
	return g;
}

int modinv(uint64_t a, uint64_t m, uint64_t *out) {
	int64_t x, y;
	int64_t g = extgcd((int64_t)a, (int64_t)m, &x, &y);
	if (g != 1 && g != -1) return 0;
	int64_t inv = x % (int64_t)m;
	if (inv < 0) inv += m;
	*out = (uint64_t)inv;
	return 1;
}
