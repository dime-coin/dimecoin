#ifndef BADINPUTS_H
#define BADINPUTS_H

#include <miner.h>
#include <validation.h>

bool is_prevout_bad(const COutPoint& out);
void record_bad_prevout(const COutPoint& out);

#endif // BADINPUTS_H
