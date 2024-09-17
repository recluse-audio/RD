#include "Window.h"
#include "BufferFiller.h"


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
    mPhaseIncrement = (double)numSamples / (double)mBuffer.getNumSamples();
}

//=================
//
const float Window::getNextSample()
{
    // Either you've read to the end of the window, or went backwards... uh oh
    if(mReadPos >= mBuffer.getNumSamples() || mReadPos < 0)
        return -1.f;

    auto sample = mBuffer.getSample(0, mReadPos);

    mReadPos += mPhaseIncrement;

    return sample;
}