#include "Point.h"

#include <cassert>
#include <cstdio>

static void test_default_construction() {
	Point p;
	p.myX = 0.0f;
	p.myY = 0.0f;
	assert(p.myX == 0.0f);
	assert(p.myY == 0.0f);
	printf("  PASS: default construction and assignment\n");
}

static void test_copy() {
	Point a;
	a.myX = 1.5f;
	a.myY = -2.5f;

	Point b = a;
	assert(b.myX == 1.5f);
	assert(b.myY == -2.5f);
	printf("  PASS: copy semantics\n");
}

static void test_assignment() {
	Point a;
	a.myX = 10.0f;
	a.myY = 20.0f;

	Point b;
	b.myX = 0.0f;
	b.myY = 0.0f;

	b = a;
	assert(b.myX == 10.0f);
	assert(b.myY == 20.0f);
	printf("  PASS: assignment operator\n");
}

int main() {
	printf("Point tests:\n");
	test_default_construction();
	test_copy();
	test_assignment();
	printf("All Point tests passed.\n");
	return 0;
}
