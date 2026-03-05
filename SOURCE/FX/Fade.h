#pragma once

class Fade
{
public:
    enum class FadeState
    {
        kNoFade,
        kFadingOut,
        kFullFade,
        kFadingIn
    };

    static constexpr int kFadeLength = 4096;

    Fade();

    void triggerFadeOut();
    void triggerFadeIn();
    void incrementFadeValue (int numSamples);

    FadeState getCurrentState() const    { return mState; }
    double    getCurrentFadeValue() const { return mCurrentFadeValue; }

private:
    FadeState mState            { FadeState::kNoFade };
    double    mCurrentFadeValue { 1.0 };
    double    mFadeIncrement    { 0.0 };
};
