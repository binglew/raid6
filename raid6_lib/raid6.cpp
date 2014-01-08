/***
*raid6.cpp - implementation for raid6 library
*
*       Copyright (c) Bingle	All rights reserved.
*
*Purpose:
*       This file contains the implementations for this generic raid6 library.
*
*Author:
*		Bingle(BinaryBB@hotmail.com)
****/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raid6.hpp"

namespace raid6{

#ifdef LIB_VC10_OPTIMIZE_ENABLED
#define INLINE_FN_GEN1( ret_t ) public: static __forceinline ret_t gen(block_t b)
#define INLINE_FN_GEN2( ret_t ) public: static __forceinline ret_t gen(block_t b, T& s) 
#else
#ifdef LIB_GCC4_1_OPTIMIZE_ENABLED 
#define INLINE_FN_GEN1( ret_t ) public: static inline ret_t gen(block_t b) __attribute__((always_inline))
#define INLINE_FN_GEN2( ret_t ) public: static inline ret_t gen(block_t b, T& s) __attribute__((always_inline))
#else // no optimize enabled
#define INLINE_FN_GEN1( ret_t ) public: static ret_t gen(block_t b)
#define INLINE_FN_GEN2( ret_t ) public: static ret_t gen(block_t b, T& s) 
#endif
#endif

//*****************************************************************************    
//class CGenericRaid6: the generic raid6 class. 
//Purpose:
//  Target to generate raid6 operating code with generic programming technique
//Template arguements naming :
//  T:      the base data type.( ALIGN_TYPE, __int64 in this target implement)
//  P:      the Prime, 2^n+1
//  _ND:    total disk number, include parity disks and data disks
//  _IX:    X direction index, start from 0
//  _IY:    Y direction index, start from 0
//  _CX:    X direction count, start from 1
//  _CY:    Y direction count, start from 1
//  _Of:    Offset value
//  _Ms:    missing disk index,(0 ~ _ND-1).
//  _M1,_M2 first,second missing disk index,assert(_M1 <= _M2)!
//Comment:
//  assert diagonal index is 0, row parity index is 1,data disk index start form 2 to _ND-1.
//*****************************************************************************
template <int _ND>
class CGenericRaid6{
private: //core expressions
	//expression template for row indexer
	template<int _Of, int _CX, int _IY>
	class et_row_indexer {
		INLINE_FN_GEN1( T ) {
			return  b[_Of][_IY] ^ et_row_indexer<_Of+1, _CX-1, _IY>::gen(b) ;
		}
	};
	template<int _Of,  int _IY>
	class et_row_indexer<_Of, 1, _IY> {
		INLINE_FN_GEN1( T ) {
			return b[_Of][_IY];
		}
	};
	template<int _Of,  int _IY>
	class et_row_indexer<_Of, 0, _IY> {
		INLINE_FN_GEN1( T ) {
			return 0;
		}
	};
	//expression template for diagonal indexer
	template<int _Of, int _CX, int _IY>
	class et_diagonal_indexer {
		INLINE_FN_GEN1( T ) {
			return b[_Of][_IY] ^ et_diagonal_indexer<_Of+1, _CX-1, _IY-1>::gen(b);
		}
	};
	template<int _Of,  int _IY>
	class et_diagonal_indexer<_Of, 1, _IY> {
		INLINE_FN_GEN1( T ) {
			return b[_Of][_IY];
		}
	};
	template<int _Of, int _CX>
	class et_diagonal_indexer<_Of, _CX, -1> {
		INLINE_FN_GEN1( T ) {
			return et_diagonal_indexer<_Of+1, _CX-1, P-2>::gen(b);
		}
	};
	template<int _Of >
	class et_diagonal_indexer<_Of, 1, -1> {
		INLINE_FN_GEN1( T ) {
			return 0;
		}
	};
	template<int _Of,  int _IY>
	class et_diagonal_indexer<_Of, 0, _IY> {
		INLINE_FN_GEN1( T ) {
			return 0;
		}
	};
	template<int _Of >
	class et_diagonal_indexer<_Of, 0, -1> {
		INLINE_FN_GEN1( T ) {
			return 0;
		}
	};
	//expression template for increase pointer by (P-1). 
	template <int _IX, int _Of, int _CX> 
	class et_add_ptr {
		INLINE_FN_GEN1( void ) {
			b[_Of] += (P-1); et_add_ptr<_IX, _Of+1, _CX-1>::gen(b);
		}
	};
	template <int _IX, int _Of> 
	class et_add_ptr<_IX, _Of, 1> {
		INLINE_FN_GEN1( void ) {
			b[_IX] += (P-1); b[_Of] += (P-1);
		}
	};
	//expression template for cal syndrome when missing two data
	template<int _IY, int _IX1, int _IX2>
	class et_syndrome{
		INLINE_FN_GEN1( T ) {
			//return b[eDiaIdx][_IY] ^ et_syndrome< _IY-1>::gen(b) ^ b[eRowIdx][_IY] ;
			return et_syndrome<_IY-1, _IX1, _IX2>::gen(b) ^ b[_IX1][_IY] ^ b[_IX2][_IY] ;
		}
	};
	template<int _IX1, int _IX2>
	class et_syndrome<0, _IX1, _IX2>{
		INLINE_FN_GEN1( T ) {
			return b[_IX1][0] ^ b[_IX2][0] ;
		}
	}; //end core expressions
	//expression template for row parity block
	template<int _CX, int _IY>
	class et_row_block {
		INLINE_FN_GEN1( void ) {
			et_row_block<_CX, _IY-1>::gen(b);
			b[eRowIdx][_IY] = et_row_indexer<2, _CX, _IY>::gen(b) ;
		}
	};
	template <int _CX>
	class et_row_block<_CX, 0> {
		INLINE_FN_GEN1( void ) {
			b[eRowIdx][0] = et_row_indexer<2, _CX, 0>::gen(b) ;
		}
	};
	//expression template for recover x from row line
	template<int _IY, int _Ms>
	class et_x_from_row{
		INLINE_FN_GEN1( void ) {
			b[_Ms][_IY] = et_row_indexer<eRowIdx, _Ms-eRowIdx, _IY>::gen(b)
				^ et_row_indexer<_Ms+1, _ND-_Ms-1, _IY>::gen(b);
		}
	};
	//expression template for recover x from row block
	template<int _IY, int _Ms>
	class et_x_from_row_block{
		INLINE_FN_GEN1( void ) {
			et_x_from_row_block<_IY-1, _Ms>::gen(b);
			et_x_from_row<_IY, _Ms>::gen(b);
		}
	};
	template<  int _Ms>
	class et_x_from_row_block<0, _Ms> {
		INLINE_FN_GEN1( void ) {
			et_x_from_row<0, _Ms>::gen(b);
		}
	};
	//expression template for diagonal block
	template<int _CX, int _IY>
	class et_diagonal_block {
		INLINE_FN_GEN2( void ) {
			et_diagonal_block<_CX, _IY-1>::gen(b, s);
			b[eDiaIdx][_IY] = et_diagonal_indexer<2, _CX, _IY>::gen(b) ^ s;
		}
	};
	template<int _CX >
	class et_diagonal_block<_CX, 0> {
		INLINE_FN_GEN2( void ) {
			s = et_diagonal_indexer<2, _CX, -1>::gen(b);
			b[eDiaIdx][0] = et_diagonal_indexer<2, _CX, 0>::gen(b) ^ s;
		}
	};
	//expression template for recover x from diagonal line
	template<int _IYMiss, int _IYDia, int _Ms>
	class et_x_from_dia { // _IYMiss = _IYDia + _Ms-2
		INLINE_FN_GEN2( void ) {
			b[_Ms][_IYMiss] = b[eDiaIdx][_IYDia] ^ s
				^ et_diagonal_indexer<2, _Ms-2, _IYDia>::gen(b)
				^ et_diagonal_indexer<_Ms+1, _ND-_Ms-1, _IYMiss-1>::gen(b);
		}
	};
	template<int _IYMiss, int _Ms>
	class et_x_from_dia <_IYMiss, -1, _Ms> { // _IYMiss = _IYDia + _Ms-2
		INLINE_FN_GEN2( void ) {
			b[_Ms][_IYMiss] = /*b[eDiaIdx][_IYDia] ^*/ s
				^ et_diagonal_indexer<2, _Ms-2, -1 >::gen(b)
				^ et_diagonal_indexer<_Ms+1, _ND-_Ms-1, _IYMiss-1>::gen(b);
		}
	};
	//expression template for recover x from diagonal block
	template<int _IYMiss, int _IYDia, int _Ms>
	class et_x_from_diagonal_block {
		INLINE_FN_GEN2( void ) {
			et_x_from_diagonal_block<_IYMiss-1, _IYDia-1, _Ms>::gen(b, s);
			et_x_from_dia<_IYMiss, _IYDia, _Ms>::gen(b, s);
		}
	};
	template<int _IYMiss, int _Ms>
	class et_x_from_diagonal_block<_IYMiss, -1, _Ms> {
		INLINE_FN_GEN2( void ) {
			et_x_from_diagonal_block<_IYMiss-1, P-2, _Ms>::gen(b, s);
			et_x_from_dia<_IYMiss, -1, _Ms>::gen(b, s);
		}
	};
	template<  int _IYDia, int _Ms>
	class et_x_from_diagonal_block<0, _IYDia, _Ms> {
		INLINE_FN_GEN2( void ) {
			s = b[eDiaIdx][_IYDia-1] ^ et_diagonal_indexer<2, _ND-2, _IYDia-1>::gen(b);
			et_x_from_dia<0, _IYDia, _Ms>::gen(b, s);
		}
	};
	template<  int _Ms>
	class et_x_from_diagonal_block<0, 0, _Ms> {
		INLINE_FN_GEN2( void ) {
			s = /*b[eDiaIdx][_IYDia-1] ^*/ et_diagonal_indexer<2, _ND-2, -1>::gen(b);
			et_x_from_dia<0, 0, _Ms>::gen(b, s);
		}
	};
	template<  int _Ms>
	class et_x_from_diagonal_block<0, -1, _Ms> {
		INLINE_FN_GEN2( void ) {
			s = b[eDiaIdx][P-2] ^ et_diagonal_indexer<2, _ND-2, P-2>::gen(b);
			et_x_from_dia<0, -1, _Ms>::gen(b, s);
		}
	};
	//expression template for recover Miss1 and Miss2
	template<int _CY, int _IY, int _M1, int _M2>
	class et_x1x2_block {
		INLINE_FN_GEN2( void ) {
			et_x1x2_block<_CY-1, (_IY-_M2+_M1+P)%P, _M1, _M2>::gen(b, s);
			et_x_from_dia<_IY, (_IY+_M1-1)%P-1, _M1>::gen(b, s);
			et_x_from_row<_IY, _M2>::gen(b); // now
		}
	};
	template< int _IY, int _M1, int _M2>
	class et_x1x2_block<1, _IY, _M1, _M2> {
		INLINE_FN_GEN2( void ) {
			s = et_syndrome<P-2, eDiaIdx, eRowIdx>::gen(b);
			et_x_from_dia<_IY, (_IY+_M1-1)%P-1, _M1>::gen(b, s);
			et_x_from_row<_IY, _M2>::gen(b); // now
		}
	};
public: //public recover interface
	#define run_head static int run(T** d, int c) { T* a[_ND], **b=a; for(int j=0; j<_ND; ++j) {a[j]=d[j];}

	class recover_d { public: //recover diagonal parity
		run_head
		T syndrome = 0;
		for( int i=c/(P-1); i>0; --i){
			et_diagonal_block<_ND-2, P-2>::gen(b, syndrome);
			et_add_ptr<eDiaIdx, 2, _ND-2>::gen(b);
		}
		return errOK;
	}};
	class recover_r { public: //recover row parity
		run_head
		for( int i=c/(P-1); i>0; --i) {
			et_row_block<_ND-2, P-2>::gen(b);
			et_add_ptr<eRowIdx, 2, _ND-2>::gen(b);
		}
		return errOK;
	}};
	template<int _Ms>
	class recover_x_from_dia { public: //recover one data from diagonal
		run_head
		T syndrome = 0;
		for( int i=c/(P-1); i>0; --i){
			et_x_from_diagonal_block<P-2, (_Ms-3+P)%P-1, _Ms>::gen(b, syndrome);
			et_add_ptr<eDiaIdx, 2, _ND-2>::gen(b);
		}
		return errOK;
	}};
	template<int _Ms>
	class recover_x_from_row { public: //recover one data from row
		run_head
		for( int i=c/(P-1); i>0; --i){
			et_x_from_row_block<P-2, _Ms>::gen(b);
			et_add_ptr<eRowIdx, 2, _ND-2>::gen(b);
		}
		return errOK;
	}};
	template<int _Ms>
	class recover_x { public: //recover one data, both from dia and from row are ok
		static int run(T** b, int c) {            
			return recover_x_from_dia<_Ms>::run(b, c);	//use diagonal here since recover_bigN need this!
			//return recover_x_from_row<_Ms>::run(b, c);
		}
	};
	class recover_dr { public: //recover both diagonal and row parity
		run_head
		T syndrome;
		for( int i=c/(P-1); i>0; --i){
			et_row_block<_ND-2, P-2>::gen(b);
			et_diagonal_block<_ND-2, P-2>::gen(b, syndrome);
			et_add_ptr<eDiaIdx, eRowIdx, _ND-1>::gen(b);
		}
		return errOK;
	}};
	template<int _Ms>
	class recover_dx { public: //recover diagonal and one data
		run_head
		T syndrome;
		for( int i=c/(P-1); i>0; --i){
			et_x_from_row_block<P-2, _Ms>::gen(b);
			et_diagonal_block<_ND-2, P-2>::gen(b, syndrome);
			et_add_ptr<eDiaIdx, eRowIdx, _ND-1>::gen(b);
		}
		return errOK;
	}};
	template<int _Ms>
	class recover_rx { public: //recover row and one data
		run_head
		T syndrome = 0;
		for(int i=c/(P-1); i>0; --i){
			et_x_from_diagonal_block<P-2, (_Ms-3+P)%P-1, _Ms>::gen(b, syndrome);
			et_row_block<_ND-2, P-2>::gen(b);
			et_add_ptr<eDiaIdx, eRowIdx, _ND-1>::gen(b);
		}
		return errOK;
	}};
	template<int _M1, int _M2>
	class recover_xx { public:  //recover two data disk
		run_head
		T syndrome = 0;
		for(int i=c/(P-1); i>0; --i){
			et_x1x2_block<P-1, P-1-_M2+_M1, _M1, _M2>::gen(b, syndrome);
			et_add_ptr<eDiaIdx, eRowIdx, _ND-1>::gen(b);
		}
		return errOK;
	}};
private: //to help generic wraper
	template<int _M1, int _M2, bool> class traits_2 {public: typedef recover_xx<_M1, _M2> imp; };
	template<int _M1, int _M2> class traits_2<_M1, _M2, true> {public: typedef recover_x<_M1> imp; };
public:  //generic wraper, the NOUSE just to prevent gcc error
	template<int _M1, int _M2, int NOUSE> class traits {public: typedef typename traits_2<_M1, _M2, _M1==_M2>::imp imp; };
	template<int _Ms, int NOUSE> class traits<0, _Ms, NOUSE> {public: typedef recover_dx<_Ms> imp; };
	template<int _Ms, int NOUSE> class traits<1, _Ms, NOUSE> {public: typedef recover_rx<_Ms> imp; };	
	template<int NOUSE> class traits<0, 0, NOUSE> {public: typedef recover_d	imp; };
	template<int NOUSE> class traits<1, 1, NOUSE> {public: typedef recover_r	imp; };
	template<int NOUSE> class traits<0, 1, NOUSE> {public: typedef recover_dr	imp; };

};//generic raid6 

//*****************************************************************************
//class CFuncTableGenerator
//Purpose:
//  To generate function set in compile time
//Naming:
//  ND1, ND2: the first, second demention of array
//  D1,D2,D3: current index for each demetion.
//  _FN:      function pointer type 
//*****************************************************************************
template <class _FN, int ND2, int ND1>
class CFuncTableGenerator {
public:
	typedef _FN table_t[][ND2][ND1]; 
	template<int D3, int D2, int D1>
	class et_recover_d31 { public:
		static void gen(table_t t) {
			et_recover_d31<D3, D2, D1-1>::gen(t);
			t[D3-3][D1][D2] = CGenericRaid6<D3>::template traits<D1, D2, 0>::imp::run;
		}
	};
	template<int D3, int D2 /*int D1*/>
	class et_recover_d31<D3, D2, 0> { public:
		static void gen(table_t t) {
			t[D3-3][0][D2] = CGenericRaid6<D3>::template traits<0, D2, 0>::imp::run;
		}
	};
	template<int D3, int D2, int D1>
	class et_recover_d32 { public:
		static void gen(table_t t) {
			et_recover_d32<D3, D2-1, D2-1>::gen(t);
			et_recover_d31<D3, D2, D1>::gen(t);
		}
	};
	template<int D3, /*int D2,*/ int D1>
	class et_recover_d32<D3, 0, D1> { public:
		static void gen(table_t t) {
			et_recover_d31<D3, 0, 0>::gen(t);
		}
	};
	template<int D3, int D2, int D1>
	class et_recover_d33 { public:
		static void gen(table_t t) {
			et_recover_d33<D3-1, D3-2, D3-2>::gen(t);
			et_recover_d32<D3, D2, D1>::gen(t);
		}
	};
	template</*int D3,*/ int D2, int D1>
	class et_recover_d33<3, D2, D1> { public:
		static void gen(table_t t) {
			et_recover_d32<3, 2, 2>::gen(t);
		}
	};
public:
	static void init_recover( table_t fnSet ) {
		enum {eFnInstNum = eImpDiskNum};	//Index of template recover function instanced by lib
		et_recover_d33<eFnInstNum, eFnInstNum-1, eFnInstNum-1>::gen( fnSet );        
	}
};//end CFuncTableGenerator

//*****************************************************************************
// class CRaid6
// the wrapper class for instantiate and using the generic raid6 recover engine.
//*****************************************************************************
R6RecoverFnType CRaid6::msRecoverFnSet[eImpDiskNum-2][eImpDiskNum][eImpDiskNum];
int CRaid6::msInitialized = 0;

CRaid6::CRaid6() {
	init();
}

CRaid6::~CRaid6() {
}

int CRaid6::init() {  
	if(!msInitialized){
		//static init msRecoverFnSet
		memset( (void*)msRecoverFnSet, 0, sizeof(msRecoverFnSet) );
		CFuncTableGenerator< R6RecoverFnType, eImpDiskNum, eImpDiskNum>::init_recover( msRecoverFnSet );

		msInitialized = 1;
	}
	return errOK;
}

int  CRaid6::check_input(T** block, int numBytes, int numDisk, int missingDisk1, int missingDisk2) {
	if(numDisk<3 || numDisk>eImpDiskNum )		return errInvalidDiskNum;
	if(missingDisk1<0 || missingDisk1>=numDisk) return errInvalidMissIdx;
	if(missingDisk2<0 || missingDisk2>=numDisk) return errInvalidMissIdx;
	if( (numBytes<=0) || (numBytes%((P-1)*sizeof(T)))!=0 )	return errSizeNotAligned;
	if( !block ) return errNullBlockPointer;
	enum {ePtrMask = sizeof(T)-1 };
	for(int i=0; i<numDisk; ++i) {
		if( 0==block[i]) return errNullBlockPointer;
		if( ( (long)(void*)(block[i]) & ePtrMask) !=0 )		return errBufferNotAligned;
	}
	return errOK;
}

//*****************************************************************************
//Function:
//		recover missing block, other block's data not changed. 
//Param:
//		block:		buffers on all disks, buffer ptr should aligned to base_type, 
//					index from 0~ numDisk-1.
//		numBytes:	length of each buffer, length should aligned to base_type.
//		numDisk:	total disk number.
//		missingDisk1:	index of first missing disk.
//		missingDisk2:	index of second missing disk, set same to missingDisk1 if 
//						one disk missing.
//Return:
//		return errOK if success, otherwise, return error code
//Comment:
//		note we may have faster impl if only some change happend on some disk, this
//		is what the update() interface what to do. any way you can always use the 
//		recover function to update the parities.
//*****************************************************************************
int  CRaid6::recover(T** block, int numBytes, int numDisk, int missingDisk1, int missingDisk2){
	T* b[eMaxDiskNum+1];
	int result = check_input(block, numBytes, numDisk, missingDisk1, missingDisk2);    
	if (errOK==result) {		
		for(int i=numDisk-1; i>=0; --i) {
			b[i] = block[i];
		}
		//missingDisk1 shoud <= missingDisk2
		if(missingDisk1 > missingDisk2) {
			int tmp = missingDisk1;
			missingDisk1 = missingDisk2;
			missingDisk2 = missingDisk1;
		}
		//simple with 3 disks
		if(numDisk==3) {
			int notMiss = missingDisk1>0? 0 : (missingDisk2<2? 2 : 1);
			memcpy( (void*) b[missingDisk1], (void*)b[notMiss], numBytes);
			memcpy( (void*) b[missingDisk2], (void*)b[notMiss], numBytes);			
		}
		else if(msRecoverFnSet[numDisk-3][missingDisk1][missingDisk2]) {
			result = msRecoverFnSet[numDisk-3][missingDisk1][missingDisk2](b, numBytes/sizeof(T) );
		}
		else { //should never go here
			printf("recover function not set, index=(%d,%d,%d)!", numDisk, missingDisk1, missingDisk2);
			result = errFAIL;
		}
	}
	return result;
}

}//end namspace raid6