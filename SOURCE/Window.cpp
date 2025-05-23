#include "Window.h"
#include "BufferFiller.h"
#include "Interpolator.h"


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
float Window::getAtReadPos(double readPos)
{
    
}

//=================
//
const float Window::getNextSample()
{
    // Either you've read to the end of the window, or went backwards... uh oh
    if(mReadPos >= mBuffer.getNumSamples() || mReadPos < 0)
        return 0.f;


    auto sample = _getInterpolatedSampleAtReadPos();

    mReadPos += mPhaseIncrement;

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

    int nextReadPos = (int)mReadPos + 1;
    // keep it in range
    if(nextReadPos >= mBuffer.getNumSamples())
        nextReadPos = nextReadPos - mBuffer.getNumSamples();

    double sample1 = mBuffer.getSample(0, (int)mReadPos);
    double sample2 = mBuffer.getSample(0, nextReadPos);
    double delta = mReadPos - (int)mReadPos;

    return (float)Interpolator::linearInterp(sample1, sample2, delta);

}