/***
*raid6_test.cpp - validate the raid6 library and show usage
*
*       Copyright (c) Bingle	All rights reserved.
*
*Purpose:
*       Validate the library's recover functionality works as expected.
*       User can invoke other raid6 library in recover_wrapper and implement 
*		own timer for testing the time efficiency against this library. 
*
*Author:
*		Bingle(BinaryBB@hotmail.com)
****/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "../raid6_lib/raid6.hpp"

using namespace raid6;


class CCycleTimer {
protected:
	typedef unsigned long long 	UINT64;
	typedef unsigned int 		UINT32;
	
	UINT64	mT0;
	UINT64	mT1;		
	UINT64	mCount;
	static  UINT64	mAvrSelfTime;
    static  UINT64	mCpuFreq;

	union S{ 
		UINT64	i;
		struct {
			UINT32	lo;
			UINT32	hi;
		};
		S(){}
		S(UINT64 v) {
			i = v;
		}
	};

public:
	CCycleTimer() {
		assert(sizeof(UINT64)==8 && sizeof(UINT32)==4 );		
		if( 0==mAvrSelfTime ) 
			calAvrSelfTime();
		if( 0==mCpuFreq ) 
			calCpuFreq();
		clear();
	}
	CCycleTimer(const CCycleTimer& rop)
		: mT0(rop.mT0), mT1(rop.mT1), mCount(rop.mCount)
	{}
	~CCycleTimer() 
	{}

public:		
	void start() {
		clear();
		resume();
	}

	void pause() {
		UINT64 t;
		++mCount;
		get_cpu_tick(&t);
		mT0 += (t - mT1);
	}

	void resume() {
		get_cpu_tick(&mT1);;
	}

	const UINT64& getTime() {
		return mT0;
	}

	void getTime(UINT32* pLow, UINT32* pHigh) {
		*pLow  = ((S*)(&mT0))->lo;
		*pHigh = ((S*)(&mT0))->hi;
	}

	int calAvrSelfTime() {
		mAvrSelfTime = 0;

		start();
		pause(); resume();
		pause(); resume();
		pause(); resume();
		pause(); resume();

		pause(); resume();
		pause(); resume();
		pause(); resume();
		pause(); resume();

		pause(); resume();
		pause(); resume();
		pause(); resume();
		pause(); resume();

		pause(); resume();
		pause(); resume();
		pause(); resume();
		pause(); 

		mAvrSelfTime = mT0 / 16;
		
		clear();
		return S(mAvrSelfTime).lo;
	}
    
    unsigned long calCpuFreq(){
        start();
        
        clock_t t0 = clock();
        //consumeCpu()
        //_sleep(1500);
        int t = rand();
        for(int i=0; i<16*1024*1024;++i){ //about 320 ms on 3G intel cpu
            t=( t/11 + (int)rand() ) % 7; 
        }
        clock_t t1 = clock();
        pause();
		double dt = difftime(t1,t0);
        mCpuFreq = UINT64( mT0/dt*CLOCKS_PER_SEC  + t ) ; 

        clear();
        return (long)mCpuFreq - t;
    }
    
    double getTimeInMs(){
        return double(mT0)/double(mCpuFreq)*1000;
    }

	void report(const char* msg) {
		const char* p = msg ? msg : "CPU_Cycle_Timer:";
//			char  buf[1024];
		S	t(mT0);
        printf("%s Total:%.2f, Count:%d, RealAvr:%.2f, Self:%d\nAvr in ms:%.4f, cpu freq: %.2fG\n", 
				p,
				double (mT0),
				(int)	mCount,
				double	(mT0 - mCount*mAvrSelfTime)/(double)mCount,
				(int)	mAvrSelfTime,
                double	(mT0 - mCount*mAvrSelfTime)/(double)mCount/(double)mCpuFreq * 1000,
                double  (mCpuFreq /double(1024*1024*1024) )
			);
		return;
	}

protected:

#ifdef WIN32
#define rdtsc_x86( V )		\
__asm {						\
   _asm push eax			\
   _asm push edx			\
   _asm _emit 0Fh			\
   _asm _emit 31h			\
   _asm mov V.lo, eax		\
   _asm mov V.hi, edx		\
   _asm pop edx				\
   _asm pop eax				\
}
#endif 

#define rdtsc_mips(dest) \
__asm__ __volatile__ ("mfc0 %0,$9; nop" : "=r" (dest) ) 

#ifdef LINUX
	void get_cpu_tick(UINT64* p64) {
	#if defined(__i386__)
		unsigned long long int x;
		__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x)); 
		*p64 = x;
	#endif

	#if defined(__x86_64__)
		unsigned hi, lo;
		__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
		*p64 = ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
	#endif
}
#endif

#ifdef WIN32
	void get_cpu_tick(UINT64* p64) {
		S s;
		#ifdef	__mips__ 
		rdtsc_mips( s )
		#else  
		rdtsc_x86( s );	
		#endif

		*p64 = s.i;
	}
#endif

	void clear() {
		mT0 = mT1 = mCount = 0;
	}
}; //end class timer
CCycleTimer::UINT64	CCycleTimer::mAvrSelfTime;
CCycleTimer::UINT64	CCycleTimer::mCpuFreq;


class CRaid6_Test{
public:
	enum {
		eUseMy=0, eUseEx=1,
		eRandOne=0, eRandAll=1,
		eCompareMixedBoth=-2, eCompareBoth, 		
		eCompareMyOnly=eUseMy, eCompareExOnly=eUseEx,
	};
protected:
	int mBlockSize;
	int mIter;
	int mNumDisk;
	int mMissingDisk1;
	int mMissingDisk2;
	int mCompareMode;

	char**      mRawBuf;
	char**      mBuf;
	int         mNumBuf;

	CRaid6		mR6;

	double mTime[2][20][8];
	int	mCount[2][20][8];
	CCycleTimer timer[2];

public:
	CRaid6_Test(){};
	~CRaid6_Test(){};
public:
	template <class T> T bound(T& v, T lo, T hi) { v = (v<lo? lo : (v>hi? hi : v) ); return v;}

	int initParam( int blockSize, int iter, int numDisk, int missDisk1, int missDisk2, int mode=-1) {
		mBlockSize      = blockSize;
		mIter           = iter;
		mNumDisk        = numDisk;
		mMissingDisk1   = missDisk1;
		mMissingDisk2   = missDisk2;
		mCompareMode    = mode;

		memset( (void*)mTime, 0, sizeof(mTime) );
		memset( (void*)mCount, 0, sizeof(mCount) );

		bound(mIter, 1, 1024);
		bound(blockSize, 256, 1024*1024*1024);
		bound(mNumDisk, 3, (int)raid6::eImpDiskNum);
		bound(missDisk1, 0, mNumDisk-1);
		bound(missDisk2, 0, mNumDisk-1);
		bound(mCompareMode, -2, 1);

		return 0;
	}
	void dump(){
		printf("raid6 tester: diskNum=%d, iterate=%d, useMde=%d, blockSize=%d\n",
			mNumDisk, mIter, mCompareMode, mBlockSize);
		return;
	}

	template<class T, int align> 
	T** prepareBuf(int bufBytes, int numBuf, int init){
		int i, j;
		mNumBuf = numBuf + 2;
		mRawBuf = (char**) malloc( sizeof(char*) * mNumBuf );
		mBuf    = (char**) malloc( sizeof(char*) * numBuf );
		for(i=0; i<mNumBuf; ++i) {
			mRawBuf[i] = (char*) malloc( sizeof(char) * (bufBytes + 1024) ); 
			// init data
			for (j=(bufBytes+1024)/sizeof(short) -1; j>=0; j-=sizeof(short) ) {
				*(short*)(mRawBuf[i]+j) = (short)rand(); 	
			}

			if (i>=1 && i<mNumBuf-1) {
				mBuf[i-1] = get_aligned_ptr<char, char, align>(mRawBuf[i]+512); //make aligned
			}
		}    
		return (T**)&mBuf[0];
	}

	void freeBuf() {
		int i;
		for(i=0; i<mNumBuf; ++i) {
			free( (void*)mRawBuf[i] );
		}
		free( (void*)mRawBuf );
		free( (void*)mBuf );
		return;
	}

	template<class T>
	void randBuffer(T* bufPtr, int numBytes, int val, int mode ) {
		//do not use bufPtr
		int i;
		int range = (numBytes-sizeof(short))/sizeof(short);
		if (eRandAll==mode) { 
			for(i=range; i>=0; --i) {
				((short*)bufPtr)[i] = (short)rand();
			}
		} else
			if (eRandOne==mode) {
				*(short*)( (short*)bufPtr + rand() % range ) = rand();		
			}
	}
	int runTest() {
		enum{
			eN = eMaxDiskNum+4,
		};
		//init buffer
		T** p = prepareBuf<T, 16>(mBlockSize, eN, 1);

		int errorFlag = 0;
		int provider;
		srand( (unsigned int)time(0) );

		memset( (void*)mTime, 0, sizeof(mTime) );
		memset( (void*)mCount, 0, sizeof(mCount) );

		//test all	
		for(int iter=0; iter<mIter; ++iter) {
			if (mCompareMode >= 0 ) {
				provider = mCompareMode & 0x01;
			}
			else {
				provider = rand() & 0x01;
			}            

			for( int nDisk= mNumDisk; nDisk>=3; --nDisk ) {
				randBuffer( p[rand()%nDisk ], mBlockSize, 0, eRandAll);

				//build diagonal/row parity
				recover_wrapper( (T**)p, mBlockSize, nDisk, 0, 0,  provider); 
				recover_wrapper( (T**)p, mBlockSize, nDisk, 1, 1,  provider); 

				for( int miss1 = nDisk-1; miss1>=0; --miss1) {
					for( int miss2 = miss1; miss2<nDisk; ++miss2) {
						//for( int miss2 = miss1; miss2<=miss1; ++miss2) {
						memcpy(p[eN-2], p[miss1], mBlockSize);	//save original data
						memcpy(p[eN-1], p[miss2], mBlockSize);	//save original data

						randBuffer( p[miss1], mBlockSize, 0, eRandOne);
						randBuffer( p[miss2], mBlockSize, 0, eRandOne);

						if(mCompareMode==-2) { //mix new old engine for test             
							provider = rand() & 0x01;
						}
						//call provider's recover function
						recover_wrapper( (T**)p, mBlockSize, nDisk, miss1, miss2, provider );

						printf("(%2d,%2d,%2d)", nDisk, miss1, miss2 );

						//check recover error
						if ( memcmp(p[eN-2], p[miss1], mBlockSize) 
							|| memcmp(p[eN-1], p[miss2], mBlockSize) ) {
								printf("\nrecover error at:size=%dK, NDisk=%d,miss:(%d,%d)\n",
									mBlockSize/1024, nDisk, miss1, miss2); 
								randBuffer( p[miss1], mBlockSize, 0, eRandAll);
								randBuffer( p[miss2], mBlockSize, 0, eRandAll);
								recover_wrapper( (T**)p, mBlockSize, nDisk, 0, 0,  provider); 
								recover_wrapper( (T**)p, mBlockSize, nDisk, 1, 1,  provider); 
								errorFlag = 1;
						}
					}//miss disk 2					
				}//miss disk1
			}//Ndisk
		}//iteration            

		if (errorFlag) {
			printf("\nHas error!!!!\n");
		}
		else {
			printf("\nAll OK.^^^^\n");
		}

		printReport();

		freeBuf();
		return 0;
	}

	int recover_wrapper(T** block, int numBytes, int numDisk, int miss1, int miss2, int provider){        
		int categray=0;	//which type of recover
		if(miss1==miss2) {
			if (0==miss1)           
				categray = 0;
			else if (1==miss1)      
				categray = 1;
			else                    
				categray = 2; 
		}else if(0==miss1) {
			if(1==miss2)            
				categray = 3;
			else                    
				categray = 4;
		}else if(1==miss1){         
			categray = 5;
		}   
		else { 
			categray = 6;
		}

		provider &= 0x01;  
		//enable timer here for compare efficiency
		timer[provider].start();
		if(provider==eUseEx) {
			//call other raid6 provider's recover function here

		}else{
			mR6.recover( block, numBytes, numDisk, miss1, miss2 );
		}        
		timer[provider].pause();
		mTime[provider][numDisk][categray] += timer[provider].getTimeInMs();
		mCount[provider][numDisk][categray] += 1;
		return 0;
	}

	void printReport() {
		const char* categray_name[7] = {"dia_only", "row_only", "one_data", "dia_row", "dia_data", "row_data", "two_data"};        
		int cat, nd;
		printf("\nmiss:    |");
		for(cat=0; cat<7; ++cat){
			printf("%12s  |", categray_name[cat] );
		}
		printf("\n         |");
		for(cat=0; cat<7; ++cat){
			printf("%14s", "   MY:    US: |" );
		}

		for(nd=3; nd<=eImpDiskNum; ++nd) {            
			printf("\ndisk:%3d |", nd);
			for(cat=0; cat<=6; ++cat){
				printf("%6.3f,%6.3f |", mTime[0][nd][cat]/mCount[0][nd][cat], mTime[1][nd][cat]/mCount[1][nd][cat] );
			}
		}
		timer[0].report("\ntimer:0");
		timer[1].report("\ntimer:1");
		printf("\n");
	}

};//end CRaid6_Test

int getValue() {
	int v=0;
	int c;
	do{
		c = getchar();
		if(c>='0'&&c<='9'){
			v = v*10 + c - '0';
		}else {
			break;
		}
	}while(1);
	return v;
}

void printHelp() {
	printf("Bingle's raid6 tester, help:..."
		"\nr(run)"
		"\nq(quit)"
		"\ni<number>(iteration times)"
		"\nn<number>(max disk number)"
		"\ns<number>(block size in KB)"
		"\nd(dump current raid6 setting)"
		"\nh(help)\n"
		);
}


int main(int argc, char* argv[])
{
	printHelp();

	CRaid6_Test aTest;

	int iter    = 10;
	int mode    = CRaid6_Test::eUseMy;
	int ndisk   = raid6::eImpDiskNum;
	int size    = 512*1024;
	aTest.initParam(size, iter, ndisk, -1, -1, mode);

	int c = 0;
	int bEnd = 0;
	do {		
		switch (c) {
		case 'q':
			printf("exist....\n");
			bEnd = 1;
			break;
		case 'n':
			ndisk = getValue();
			printf("max disk number:%d\n", ndisk);
			break;
		case 'i':  
			iter = getValue();
			printf("iteration count:%d\n", iter);               
			break;
		case 's':
			size = getValue() * 1024;
			printf("block size:%d \n", size); 
			break;
		case 'u':
			printf("use your implementation...\n");
			mode=CRaid6_Test::eCompareExOnly;
			break;
		case 'm':
			printf("use my implementation...\n");
			mode=CRaid6_Test::eCompareMyOnly;
			break;
		case 'b':
			printf("compare two implementations...\n");
			mode=CRaid6_Test::eCompareBoth;
			break;
		case 'x':
			printf("compare two implementations...and the testing order is fully mixed.\n");
			mode=CRaid6_Test::eCompareMixedBoth;
			break;
		case 'd':
			aTest.dump();
			break;
		case 'r':
			aTest.initParam(size, iter, ndisk, -1, -1, mode);
			aTest.dump();
			aTest.runTest();				
			break;
		case '\n':
			break;
		case 'h':
			printHelp();
		default:                
			break;
		}
	} while(!bEnd && (c=getchar()) );

	return 0;
}
