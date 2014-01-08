/***
*raid6_config.hpp - generic raid6 library configurations
*
*       Copyright (c) Bingle	All rights reserved.
*
*Purpose:
*       This file contains the configurations of this raid6 library.
*       User can modify the definitions to satisfy different compile time requirement.
*
*Author:
*		Bingle(BinaryBB@hotmail.com)
****/


#ifndef _RAID6_CONFIG_HPP_INCLUDE_
#define _RAID6_CONFIG_HPP_INCLUDE_

namespace raid6{

	//modify following definitions to config this library
	struct raid6_config_tag{

		typedef long long base_type;	//the base data unit type used by raid6 engine.

		enum {
			ePrime				= 17,	//the prime used by raid6 algorithm, should be 2^N+1.

			eSupportDiskNum		= 8,	//maximun disk numbers the library could support when compiled out. 
			//eSupportDiskNum should <=ePrime+2 !!!
			eDoPrefetch			= 0,	//whether do prefetch instruction. not implemented in this version.
		};

		//optimizing on different compiler
		#ifdef WIN32
		#define LIB_VC10_OPTIMIZE_ENABLED
		#else 
		#ifdef LINUX
		#define LIB_GCC4_1_OPTIMIZE_ENABLED
		#endif
		#endif
	};

}//end namespace raid6
#endif//_RAID6_CONFIG_HPP_INCLUDE_