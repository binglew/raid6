/***
*raid6.hpp - definitions/declarations for raid6 library
*
*       Copyright (c) Bingle	All rights reserved.
*
*Purpose:
*       This file contains the class, constances, values, definitions for using this raid6 library.
*
*Author:
*		Bingle(BinaryBB@hotmail.com)
****/

#ifndef _RAID6_HPP_INCLUDE_
#define _RAID6_HPP_INCLUDE_

#include "raid6_config.hpp"

namespace raid6{

	//lib constances 
	enum EnumLibConsts
	{
		P           = raid6_config_tag::ePrime, //the Prime
		eImpDiskNum = raid6_config_tag::eSupportDiskNum, //max disk num supported in this implementation
		eDiaIdx     = 0,                        //diagonal parity index
		eRowIdx     = 1,                        //row parity index
		eMaxDiskNum = P+2,                      //max disk num limit supported by P
	};

	//error code
	enum EnumLibErrorCode 
	{
		errFAIL             =-1,				//general fail code
		errOK               = 0,				//OK
		errInvalidDiskNum   = 1,				//invalid disk number
		errInvalidMissIdx   = 2,				//invalid missing disk index	
		errNullBlockPointer = 3,				//a data buffer pointer is NULL	
		errBufferNotAligned = 4,				//data buffer not start at aligned address. should aligned with the raid6_config_tag::base_type
		errSizeNotAligned   = 5,				//data length not aligned. should aligned with the base_type * (P-1)
	};

	//base type definition
	typedef raid6_config_tag::base_type		T;
	typedef T**&                            block_t;
	typedef int ( *R6RecoverFnType )(T** block, int numBytes);

	//helper function
	template <class DST_T, class SRC_T, int Align>
	DST_T* get_aligned_ptr(SRC_T* ptr ) {
	#if (Align & (Align-1) )
	#error "Align should be a value of 2^n"
	#endif
		return (DST_T*)(void*) ( ( (long long)(void*)(ptr) + (Align-1) ) & ( ~(long long)(Align-1) ) );
	}

	//the generic wrapper raid6 class
	class CRaid6{
	private:
		//the recover function table:
		//index meaning							[numDisk-3];	[miss1 index];		[miss2 index]	//miss1 <= miss2
		//avaiable set:							[3~eImpDiskNum] [0~eImpDiskNum-1]	[0~eImpDiskNum-1] 
		static R6RecoverFnType msRecoverFnSet	[eImpDiskNum-2]	[eImpDiskNum]		[eImpDiskNum]; 	

		static int msInitialized;				//whether the msRecoverFnSet initialized 	

	public:
		CRaid6();
		~CRaid6();

	public:
		int check_input(T** block, int numBytes, int numDisk, int missingDisk1, int missingDisk2);
		int recover(T** block, int numBytes, int numDisk, int missingDisk1, int missingDisk2);
		//int update(T* parity, int numBytes, T* dataOld, T* dataNewOrDiff, int dataIdx, int mode);

	private:
		int init();

	};//end CRaid6

}//end namespace

#endif//_RAID6_HPP_INCLUDE_
