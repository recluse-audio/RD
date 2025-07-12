#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "../SOURCE/BufferRange.h"


TEST_CASE("Can set range basics and retrieve range")
{
	RD::BufferRange bufferRange;
	bufferRange.setStartIndex(10);
	bufferRange.setEndIndex(20);

	CHECK(bufferRange.getStartIndex() == 10 );
	CHECK(bufferRange.getEndIndex() == 20 );
	CHECK(bufferRange.getLengthInSamples() == 11 );

}