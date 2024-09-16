## RD ( Ryan's DSP )

This is a dsp library that is dependent upon JUCE.  

You are best to include this as a submodule in your git tracking

Current plan is to then simply include these files like regular source.

This avoids some worrisome juce includes and linking of catch2 and such.
Speaking of which, test coverage of this repo will be accomplished with a test plug-in sort of thing.  Either that or something that includes it as a library.



### Plans to Accomadate different versions of juce

The would be test plug-in will include `RD` as a submodules.  In addition to this it will include a JUCE submodule.  

The problem is, if you want to use a different version of the juce repo than the test plug-in, then you risk a loss in test coverage.  And you don't get it automatically.  You'd have to update the test plug-in's juce submodule to whereever, build it, build the tests target, then run the tests again.  

Or, perhaps you can include the TestFiles.cmake from `RD` into your test target.