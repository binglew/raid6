raid6
=====

This library implement the raid6 recovery algorithms with generic programming technique.

compile passed on:
* gcc 4.6.1 (Ubuntu/Linaro 4.6.1-9ubuntu3) 
* Visual Studio 2010 SP1 (Windows7 Enterprise SP1 64bit)

Directories:
-------------
* raid6_lib:
	contains the implementation of this library.
	modify raid6.config.hpp for change compile time config

* raid6_test:
	the testing/sample for using this library


Usage: 
-------
step 1, Config .\raid6_lib\raid6_config.hpp as your requirement.  
  
To build row parity and diagonal parity: 

    #include "raid6.hpp"
    using namespace raid6;
    ...
        CRaid6 R6; 
        R6.recover( pointerArrayToTheBuffersOnEachDisk, numBytesOfEachBuffer, numDisk, 
        	eRowIdx, eDiaIdx);

To recover data on any missing disks.(make missingDiskIndex1==missingDiskIndex2 in case one disk missing)

        R6.recover( pointerArrayToTheBuffersOnEachDisk, numBytesOfEachBuffer, numDisk,
        	missingDiskIndex1, missingDiskIndex2);
Author:
-------
Bingle (binarybb@hotmail.com)

2012.10.18

https://drive.google.com/file/d/1ToH2WJvYTi2bEeixipbSIVcXNclw3Vx9/view?usp=sharing
