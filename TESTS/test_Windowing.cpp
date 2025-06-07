#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "../SOURCE/Window.h"
#include "../SOURCE/BufferFiller.h"

/**
 * @brief Helps check stuff under the hood, especially really fundamental stuff like buffer resizing with sample rate
 * 
 * 
 */

class WindowTester
{
public:
    WindowTester(Window& windower) : mWindow(windower){}
    ~WindowTester(){}

    int getWindowSize() { return mWindow.mBuffer.getNumSamples(); }
    // int getWindowShape() { return mWindow.mCurrentShape; } // hack testing I know
    juce::AudioBuffer<float>& getBuffer() { return mWindow.mBuffer; }
private:
    Window& mWindow;
};


//=========================
//
TEST_CASE("Window can handle sample rate change")
{
    Window w;
    WindowTester wt(w);

    double sampleRate = 48000;
    w.setSize(sampleRate);
    CHECK( wt.getWindowSize() == sampleRate );

    sampleRate = 88200;
    w.setSize(sampleRate);
    CHECK( wt.getWindowSize() == sampleRate );

    sampleRate = 96000;
    w.setSize(sampleRate);
    CHECK( wt.getWindowSize() == sampleRate );

    sampleRate = 176400;
    w.setSize(sampleRate);
    CHECK( wt.getWindowSize() == sampleRate );

    sampleRate = 192000;
    w.setSize(sampleRate);
    CHECK( wt.getWindowSize() == sampleRate );
}



//=====================
// Testing read position updating by passing in a buffer who's samples are equal to their index
TEST_CASE("Can get sample from window")
{
    Window w;
    WindowTester wt(w);

    int bufferSize = 100;
    w.setSize((double)bufferSize);
    BufferFiller::fillIncremental(wt.getBuffer()); // fill the underlying buffer incrementally

    for(int sampleIndex = 0; sampleIndex < bufferSize; sampleIndex++ )
    {
        auto sample = w.getNextSample();
        CHECK( sample == (float)sampleIndex );
    }

    // At this point the Window's read pos is past the length of the buffer
    CHECK (w.getNextSample() == 0.f );

}

//=======================
// Test that we can adjust the period of the window properly
TEST_CASE("Can set and reset period of window")
{
    Window w;
    WindowTester wt(w);

    int bufferSize = 100;
    w.setSize((double)bufferSize);
    BufferFiller::fillIncremental(wt.getBuffer()); // fill the underlying buffer incrementally

    // Set period to 1/2 the size of underlying buffer in window
    // So mPhaseIncrement should double, and mReadPos increment 2x as fast
    w.setPeriod(bufferSize / 2);

    for(int sampleIndex = 0; sampleIndex < bufferSize / 2; sampleIndex++ )
    {
        auto sample = w.getNextSample();
        CHECK( sample == (float)sampleIndex * 2.f );
    }

    // This should reset the read pos and phase increment
    w.reset();
    // now the sample should not be double of sampleIndex
    for(int sampleIndex = 0; sampleIndex < bufferSize / 2; sampleIndex++ )
    {
        auto sample = w.getNextSample();
        CHECK( sample == (float)sampleIndex );
    }


    w.reset();
    w.setPeriod(bufferSize * 2);
    for(int sampleIndex = 0; sampleIndex < bufferSize / 2; sampleIndex++ )
    {
        auto sample = w.getNextSample();
        CHECK( sample == (float)sampleIndex * 0.5f );
    }

}

//=====================
//
TEST_CASE("Can set and get shape of window ")
{
    Window w;

    w.setShape(Window::Shape::kHanning);
	REQUIRE(w.getShape() == Window::Shape::kHanning);

    w.setShape(Window::Shape::kNone);
	REQUIRE(w.getShape() == Window::Shape::kNone);
}



//=====================
// Testing read position updating by passing in a buffer who's samples are equal to their index
TEST_CASE("Can set window to loop")
{
    Window window;
    WindowTester wt(window);

    int bufferSize = 100;
   	window.setSize((double)bufferSize);
    BufferFiller::fillIncremental(wt.getBuffer()); // fill the underlying buffer incrementally
	window.setPeriod(bufferSize);
	window.setLooping(true);

	int samplesOfTenBuffers = bufferSize * 10;

    for(int sampleIndex = 0; sampleIndex < samplesOfTenBuffers; sampleIndex++ )
    {
		int loopedIndex = sampleIndex % bufferSize;
        auto sample = window.getNextSample();
        REQUIRE( sample == (float)loopedIndex );
    }



}


