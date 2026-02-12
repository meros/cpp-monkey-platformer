#include "TMatrix.h"
#include "Point.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static bool approxEqual(float a, float b, float eps = 0.001f) {
	return std::fabs(a - b) < eps;
}

static void test_identity() {
	TMatrix m;
	Point p;
	p.myX = 3.0f;
	p.myY = 4.0f;

	m.Apply(p);

	assert(approxEqual(p.myX, 3.0f));
	assert(approxEqual(p.myY, 4.0f));
	printf("  PASS: identity transform\n");
}

static void test_rotate_90() {
	TMatrix m;
	m.Rotate(static_cast<float>(M_PI / 2.0));

	Point p;
	p.myX = 1.0f;
	p.myY = 0.0f;

	m.Apply(p);

	assert(approxEqual(p.myX, 0.0f));
	assert(approxEqual(p.myY, 1.0f));
	printf("  PASS: 90 degree rotation\n");
}

static void test_rotate_180() {
	TMatrix m;
	m.Rotate(static_cast<float>(M_PI));

	Point p;
	p.myX = 1.0f;
	p.myY = 0.0f;

	m.Apply(p);

	assert(approxEqual(p.myX, -1.0f));
	assert(approxEqual(p.myY, 0.0f));
	printf("  PASS: 180 degree rotation\n");
}

static void test_rotate_360() {
	TMatrix m;
	m.Rotate(static_cast<float>(2.0 * M_PI));

	Point p;
	p.myX = 5.0f;
	p.myY = 3.0f;

	m.Apply(p);

	assert(approxEqual(p.myX, 5.0f));
	assert(approxEqual(p.myY, 3.0f));
	printf("  PASS: 360 degree rotation (identity)\n");
}

static void test_double_rotation() {
	TMatrix m;
	m.Rotate(static_cast<float>(M_PI / 4.0));
	m.Rotate(static_cast<float>(M_PI / 4.0));

	Point p;
	p.myX = 1.0f;
	p.myY = 0.0f;

	m.Apply(p);

	assert(approxEqual(p.myX, 0.0f));
	assert(approxEqual(p.myY, 1.0f));
	printf("  PASS: two 45-degree rotations = 90 degrees\n");
}

int main() {
	printf("TMatrix tests:\n");
	test_identity();
	test_rotate_90();
	test_rotate_180();
	test_rotate_360();
	test_double_rotation();
	printf("All TMatrix tests passed.\n");
	return 0;
}
