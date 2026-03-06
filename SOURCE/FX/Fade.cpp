#include "Fade.h"

Fade::Fade()
    : mFadeIncrement (1.0 / kFadeLength)
{
}

void Fade::setLength (int numSamples)
{
    mFadeIncrement = 1.0 / numSamples;
}

void Fade::triggerFadeOut()
{
    mState = FadeState::kFadingOut;
}

void Fade::triggerFadeIn()
{
    mState = FadeState::kFadingIn;
}

void Fade::incrementFadeValue (int numSamples)
{
    if (mState == FadeState::kFadingOut)
    {
        mCurrentFadeValue -= mFadeIncrement * numSamples;
        if (mCurrentFadeValue <= 0.0)
        {
            mCurrentFadeValue = 0.0;
            mState = FadeState::kFullFade;
        }
    }
    else if (mState == FadeState::kFadingIn)
    {
        mCurrentFadeValue += mFadeIncrement * numSamples;
        if (mCurrentFadeValue >= 1.0)
        {
            mCurrentFadeValue = 1.0;
            mState = FadeState::kNoFade;
        }
    }
}
