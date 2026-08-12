// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/moneystr.h>

#include <primitives/transaction.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <limits>

std::string FormatMoney(const CAmount& n)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    // Negating the value to take its magnitude is undefined for INT64_MIN,
    // whose positive counterpart is not representable. Divide first instead:
    // COIN > 1, so neither the quotient nor the remainder can overflow, and
    // both are safe to negate afterwards.
    int64_t quotient = n / COIN;
    int64_t remainder = n % COIN;
    if (n < 0) {
        quotient = -quotient;
        remainder = -remainder;
    }
    std::string str = strprintf("%d.%05d", quotient, remainder);

    // Right-trim excess zeros before the decimal point:
    int nTrim = 0;
    for (int i = str.size()-1; (str[i] == '0' && isdigit(str[i-2])); --i)
        ++nTrim;
    if (nTrim)
        str.erase(str.size()-nTrim, nTrim);

    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    return str;
}


bool ParseMoney(const std::string& str, CAmount& nRet)
{
    return ParseMoney(str.c_str(), nRet);
}

bool ParseMoney(const char* pszIn, CAmount& nRet)
{
    std::string strWhole;
    int64_t nUnits = 0;
    const char* p = pszIn;
    while (isspace(*p))
        p++;
    for (; *p; p++)
    {
        if (*p == '.')
        {
            p++;
            int64_t nMult = CENT*10;
            while (isdigit(*p) && (nMult > 0))
            {
                nUnits += nMult * (*p++ - '0');
                nMult /= 10;
            }
            break;
        }
        if (isspace(*p))
            break;
        if (!isdigit(*p))
            return false;
        strWhole.insert(strWhole.end(), *p);
    }
    for (; *p; p++)
        if (!isspace(*p))
            return false;
    // The previous guard capped the whole part at 10 digits, a constant carried
    // over from Bitcoin's 8-decimal COIN. Dimecoin's COIN has 5 decimals, so 10
    // digits rejects perfectly valid amounts that FormatMoney will produce --
    // MAX_MONEY alone is 50,000,000,000 DIME, which is 11 digits -- and the
    // round trip fails. Bound the result against what int64_t can actually hold
    // rather than inferring it from the digit count.
    if (strWhole.size() > 19) // atoi64 itself cannot represent more than this
        return false;
    if (nUnits < 0 || nUnits > COIN)
        return false;
    int64_t nWhole = atoi64(strWhole);
    if (nWhole < 0 || nWhole > (std::numeric_limits<int64_t>::max() - nUnits) / COIN)
        return false;
    CAmount nValue = nWhole*COIN + nUnits;

    nRet = nValue;
    return true;
}
