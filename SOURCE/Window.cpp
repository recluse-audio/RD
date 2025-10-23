#include "Window.h"
#include "BufferFiller.h"
#include "Interpolator.h"
#include "BufferHelper.h"


//===================
//
Window::Window()
{
    mBuffer.clear();
    mBuffer.setSize(1, (int)kDefaultSize); // sizing these according to 
    
}

//====================
//
Window::~Window()
{
    mBuffer.clear();
}

//====================
//
void Window::setSizeShapePeriod(int newSize, Window::Shape newShape, int newPeriod)
{
	this->reset();
	this->setSize(newSize);
	this->setShape(newShape);
	this->setPeriod(newPeriod);
}

//====================
//
void Window::setSize(double newSize)
{
    mBuffer.setSize(1, (int) newSize); // 1 second worth of samples by default
}

//=====================
//
const int Window::getSize()
{
    return mBuffer.getNumSamples();
}

//==================
//
void Window::setShape(Window::Shape newShape)
{
    if(mCurrentShape != newShape)
    {
        mCurrentShape = newShape;
        _update();
    }
}

//==================
//
Window::Shape Window::getShape()
{
	return mCurrentShape;
}

//==================
//
void Window::_update()
{
    if(mCurrentShape == Window::Shape::kHanning)
        BufferFiller::generateHanning(mBuffer);
    else if(mCurrentShape == Window::Shape::kNone)
        BufferFiller::fillWithAllOnes(mBuffer);
}

//==================
//
void Window::setPeriod(int numSamples)
{
    // don't do that
    mPeriod = numSamples;
    mPhaseIncrement = (double)mBuffer.getNumSamples() / (double)numSamples;
}

//=================
//
void Window::setReadPos(double readPos)
{
    mReadPos = readPos;
}

//=================
//
void Window::resetReadPos()
{
    mReadPos = 0.0;
}

//=================
//
float Window::getAtReadPos(double readPos) 
{
    return _getInterpolatedSampleAtReadPos(readPos);
}


//=================
//
const float Window::getNextSample()
{
    // Either you've read to the end of the window, or went backwards... uh oh
    if(mReadPos >= mBuffer.getNumSamples() || mReadPos < 0)
        return 0.f;

	float sample = 0.f;

	if(mCurrentShape == Window::Shape::kNone)
		sample = 1.f;
	else
    	sample = _getInterpolatedSampleAtReadPos();

    mReadPos += mPhaseIncrement;

	// wrap mReadPos around our total num samples, but only if we are set to loop
	if(mReadPos >= mBuffer.getNumSamples() && mIsLooping)
		mReadPos = mReadPos - mBuffer.getNumSamples();

    return sample;
}


//====================
// 
void Window::reset()
{
    mReadPos = 0.0;
    mPhaseIncrement = 1.0;
}


//====================
//
float Window::_getInterpolatedSampleAtReadPos()
{

    // int nextReadPos = (int)mReadPos + 1;
    // // keep it in range
    // if(nextReadPos >= mBuffer.getNumSamples())
    //     nextReadPos = nextReadPos - mBuffer.getNumSamples();

    // double sample1 = mBuffer.getSample(0, (int)mReadPos);
    // double sample2 = mBuffer.getSample(0, nextReadPos);
    // double delta = mReadPos - (int)mReadPos;

    // return (float)Interpolator::linearInterp(sample1, sample2, delta);
	return BufferHelper::getLinearInterpolatedSampleAtIndex(mBuffer, mReadPos);
}

//====================
//
float Window::_getInterpolatedSampleAtReadPos(float readPos)
{

    // int nextReadPos = (int)readPos + 1;
    // // keep it in range
    // if(nextReadPos >= mBuffer.getNumSamples())
    //     nextReadPos = nextReadPos - mBuffer.getNumSamples();

    // double sample1 = mBuffer.getSample(0, (int)readPos);
    // double sample2 = mBuffer.getSample(0, nextReadPos);
    // double delta = readPos - (int)readPos;

    return BufferHelper::getLinearInterpolatedSampleAtIndex(mBuffer, readPos);

}

//============
//
void Window::setLooping(bool shouldLoop)
{
	mIsLooping = shouldLoop;
}

//=============
//
const bool Window::getIsLooping()
{
	return mIsLooping;
}

//=============
//
const juce::AudioBuffer<float>& Window::getReadBuffer()
{
	return mBuffer;
}

//=============
//
float Window::getValueAtIndexInPeriod(int indexInPeriod)
{
	// honestly don't even do this, but if you do we will handle it
	// modulos are expensive or so they say, so I'm not gonna do it unless you mess up.
	if(indexInPeriod >= mPeriod)
		indexInPeriod = indexInPeriod % mPeriod;

	float readPos = (float)indexInPeriod * mPhaseIncrement;
	float valueAtIndexInPeriod = _getInterpolatedSampleAtReadPos(readPos);

	return valueAtIndexInPeriod;
}