# witpp
a wrapper around wit.ai for C++

## introduction
I was looking for a sdk for [wit.ai](http://wit.ai) in order to interact with my application but nothing was available for C++. so, i decided to roll my own
currently, it supports text request, voice request, and some more and it will be completed in fewture

## requirements
* a C++ compiler capable of compiling C++11
* jsoncpp (i've included the amalgamated version in the repository, so it shouldnt be a problem)
* libcurl
* libwit's vad (optional) which i've included this as well

## compiling
* add jsoncpp.cpp , witpp.h and the header files related to jsoncpp in your project, makefile, or anything that you use
* add vad.c and vad.h if you want the voice detector (optional)
*if you want the voice detection, define VAD_ENABLED as well
* add the path to where witpp.h is located.
* link with libcurl as well

## contributing
* if you've found a bug or have a suggestione, open an issue
* if you want to contribute code, open a pull request

## license
see [license.md](./license.md)
