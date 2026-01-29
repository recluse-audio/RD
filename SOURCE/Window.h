/**
 * @file Window.h
 * @author Ryan Devens
 * @brief 
 * @version 0.1
 * @date 2024-09-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include "Util/Juce_Header.h"


static const double kDefaultSize = 44100;

/**
 * @brief This class provides a look-up table for different window functions, First just hanning.
 * 
 * The goal is that we look up a value (0 - 1.0) and use that as an amplitude coefficient to create 
 * a way to interpret a single stored buffer of amplitude for various window lengths.
 * 
 * Inspired by ADSR and LFO wavetables I have made in the past.  I hope this saves on processing.  
 * Re-generating the window with each incoming new length (i.e. pitch period) seems expensive.
 * Storing these windows ahead of time feels like it will cause large memory footprint bloat and reduce instance count
 * 
 * These theories are not tested!  But this method is most logical first step for me.
 * 
 * This window lookup table 
 * 
 */

class Window
{
public:
    enum class Shape
    {
        kNone = 0,
        kHanning = 1,
    };

    Window();
    ~Window();

	void setSizeShapePeriod(int newSize, Window::Shape newShape, int newPeriod);

    // This does not resize the underlying buffer
    // This affects the rate at which we read the buffer
    // we will read the entirety of the window in 'numSamples'
    // interpolating as necessary
    void setPeriod(int numSamples);
	int getPeriod() { return mPeriod; }

    // sets the numSamples of the underlying buffer.
    // always 1 channel until I find a reason otherwise
    void setSize(double newSize);

    // gets the numSamples of the underlying buffer
    const int getSize();
    
    // Fills the buffer with normalized float values 
    // corresponding with various window shapes
    // Does not resize but does clear 
    void setShape (Window::Shape newShape);
	Window::Shape getShape();

    // returns the sample at the internally incremented mReadPos
    const float getNextSample();

	// takes a 0 - 1 float value and updates current read pos such t
	// void setCurrentReadPosWithNormalizedPhase(float normalizedPhase);
    void setReadPos(double readPos);
    void resetReadPos();
	double getReadPos() const { return mReadPos; }
	double getCurrentNormalizedPhase() { return mReadPos / (double)mBuffer.getNumSamples(); }
    void  setCurrentReadPosWithNormalizedPhase(double phase) { mReadPos = (double)mBuffer.getNumSamples() * phase; }

    float getAtReadPos(double readPos);

    // Returns the window value at a specific index within the current period
    // @param index The sample index within the period (0 to period-1)
    // @return The window value at that index
    float getValueAtIndexInPeriod(int index);
    // gets value at index fitted to period argument.
    // this calculates the phase increment on the fly and doesn't modify or refer to a window's mPeriod/mPhaseIncrement value
    float getValueAtIndexInPeriod(int index, int period);

    // Sets mReadPos to 0, and sets mPhaseIncrement to 1.0
    void reset();

	void setLooping(bool shouldLoop);
	const bool getIsLooping();

	// returns read only version of underlying window buffer
	// useful for tests and visualization
	const juce::AudioBuffer<float>& getReadBuffer();
private:
    friend class WindowTester;
    juce::AudioBuffer<float> mBuffer; // using juce buffer for some helpful functions, but could be a simple array

    Window::Shape mCurrentShape;

	int mPeriod = 0;
    // rate at which we read from buffer of samples.
    // Goes up with higher pitch, down with lower
    double mPhaseIncrement = 1.0;  

    // Fractional read index that might require some interpolation!
    // Incremented by mPhaseIncrement
    // Wraps at buffer size, or perhaps hits a "finished" flag
    double mReadPos = 0.0; 

	bool mIsLooping = false;

    void _update();

    // performs interpolation to handle double vals for mReadPos
    // doesn't increment mReadPos
    float _getInterpolatedSampleAtReadPos();
	float _getInterpolatedSampleAtReadPos(float readPos);
	
};