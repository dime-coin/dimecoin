#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmath>

typedef int64_t CAmount;
static const CAmount COIN = 100000;

//! to compile:
//! g++ -O2 8pc.cpp -o 8pc

CAmount decayBlockReward(int nowHeight)
{
   CAmount nSubsidy = 15400 * COIN;
   const double dailyDecay = 0.99978;
   const int blocksDaily = 86400 / 64;

   //! lets er.. recurse
   int blocksPassed = nowHeight - 1000;
   while (blocksPassed > 0) {
      blocksPassed -= blocksDaily;
      if (blocksPassed > 0) {
          nSubsidy *= dailyDecay;
      }
   }

   return std::floor((nSubsidy / COIN) * COIN);
}

int main()
{
   CAmount lastResult = 0;

   for (int i=100; i<10000000000; i++) {
       if (lastResult != decayBlockReward(i)) {
           printf("height %08d subsidy is %llu\n", i, decayBlockReward(i) / COIN);
           lastResult = decayBlockReward(i);
       }
   }

   return 0;
}

