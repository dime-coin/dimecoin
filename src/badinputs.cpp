#include <badinputs.h>

std::vector<COutPoint> bad_prevout;

bool is_prevout_bad(const COutPoint& out)
{
    for (auto& l : bad_prevout) {
        if (out == l) {
            return true;
        }
    }

    return false;
}

void record_bad_prevout(const COutPoint& out)
{
    if (!is_prevout_bad(out)) {
        bad_prevout.push_back(out);
    }
}
