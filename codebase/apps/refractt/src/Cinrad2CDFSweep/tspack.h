#ifndef RDA_TASPACK_H
#define RDA_TASPACK_H
#include "rdats.h"
void depackIQ_kernel(unsigned short *code, float *iq,size_t count);
void depackIQ(const unsigned short *code, float *iq,size_t count);
size_t depackSweeps(TSSweepHeader *swpin,size_t len,TSSweepHeader *swpout);
#endif
