/**
 * @file Interpolator
 * @author Ryan Devens
 * @brief My own interpolation algorithms, that may overlap with more functional options available elsewhere
 * 
 * 
 */

class Interpolator
{
public:

    /**
     * @brief linear interpolation between val1 and val2
     * @param delta - how far between val1 and val2 we are.
     * 
     * @example
     * val1 corresponds with index [0] in a buffer, sample val of 0.f
     * val2 corresponds with index [1] in a buffer, sample val of 1.f
     * delta is 0.6, meaning we want sample value at the [0.6] index (doesn't exist)
     */
    // Does a simple linear interpolation between val1 an
    static double linearInterp(double val1, double val2, double deltaPercent)
    {
        double delta = (val2 - val1) * deltaPercent;
        return val1 + delta;

    }

    /**
     * @brief This does the parabolic interpolation between the three points
     * 
     * Taken from a YIN implementation, in which this is a way to find the x1 value (aka bestTau)
     * when it might not be a 
     * 
    */
   static double parabolicInterp(double y0, double y1, double y2)
   {

   }
};