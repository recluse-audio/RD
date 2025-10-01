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

TEST_CASE("RD::BufferRange updates its default isEmpty==true state automatically by setting a start or end")
{
	RD::BufferRange bufferRange;
	CHECK(bufferRange.isEmpty() == true);

	bufferRange.setStartIndex(10);
	CHECK(bufferRange.isEmpty() == false);

	bufferRange.setEndIndex(20);


	CHECK(bufferRange.getStartIndex() == 10 );
	CHECK(bufferRange.getEndIndex() == 20 );
	CHECK(bufferRange.getLengthInSamples() == 11 );
	CHECK(bufferRange.isEmpty() == false);


}

TEST_CASE("Can set buffer range by passing a buffer", "test_BufferRange")
{
	RD::BufferRange bufferRange;
	bufferRange.setStartIndex(20);
	bufferRange.setEndIndex(40);
	CHECK(bufferRange.getStartIndex() == 20);
	CHECK(bufferRange.getEndIndex() == 40);
}

TEST_CASE("RD::BufferRange::setEmpty(); Can set BufferRange as empty")
{
// 	RD::BufferRange bufferRange;
// 	bufferRange.setStartIndex(10);
// 	bufferRange.setEndIndex(20);


// 	CHECK(bufferRange.getStartIndex() == 10 );
// 	CHECK(bufferRange.getEndIndex() == 20 );
// 	CHECK(bufferRange.getLengthInSamples() == 11 );

}