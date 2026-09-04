// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPORK_H
#define SPORK_H

#include <hash.h>
#include <net.h>
#include <util/strencodings.h>

class CSporkMessage;
class CSporkManager;

namespace Spork {

static const int SPORK_START                                            = 10001;

enum {
    /*
    Don't ever reuse these IDs for other sporks
    - This would result in old clients getting confused about which spork is for what
*/

    SPORK_2_INSTANTSEND_ENABLED                            = SPORK_START,
    SPORK_3_INSTANTSEND_BLOCK_FILTERING                    = 10002,
    SPORK_5_INSTANTSEND_MAX_VALUE                          = 10004,
    SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT                 = 10007,
    SPORK_9_SUPERBLOCKS_ENABLED                            = 10008,
    SPORK_10_MASTERNODE_PAY_UPDATED_NODES                  = 10009,
    SPORK_12_RECONSIDER_BLOCKS                             = 10011,
    SPORK_13_OLD_SUPERBLOCK_FLAG                           = 10012,
    SPORK_14_REQUIRE_SENTINEL_FLAG                         = 10013,
    SPORK_15_POS_DISABLED                                  = 10014,
    SPORK_END
};

}

/** Upper bound on a spork's nValue.
 *
 *  Every spork value in use is either an activation timestamp (0 = on, 4070908800 = off) or a
 *  small threshold such as SPORK_5's coin limit. Nothing legitimate comes near this cap, so it
 *  costs no flexibility while denying an attacker the ability to claim an absurdly long value. */
static const int64_t MAX_SPORK_VALUE = 1000000000000LL;

/** How far into the future a spork's nTimeSigned may be relative to adjusted network time.
 *
 *  ProcessSpork only ever compared nTimeSigned for monotonicity, with no ceiling, so a spork
 *  carrying a year-2099 timestamp could never be superseded by the real key holder. */
static const int64_t SPORK_TIME_MAX_FUTURE_DRIFT = 2 * 60 * 60;

extern std::map<uint256, CSporkMessage> mapSporks;
extern CSporkManager sporkManager;

//
// Spork classes
// Keep track of all of the network spork settings
//

class CSporkMessage
{
private:
    std::vector<unsigned char> vchSig;

public:
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    CSporkMessage(int nSporkID, int64_t nValue, int64_t nTimeSigned) :
        nSporkID(nSporkID),
        nValue(nValue),
        nTimeSigned(nTimeSigned)
        {}

    CSporkMessage() :
        nSporkID(0),
        nValue(0),
        nTimeSigned(0)
        {}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << nSporkID;
        ss << nValue;
        ss << nTimeSigned;
        return ss.GetHash();
    }

    bool Sign(std::string strSignKey);
    bool CheckSignature();
    void Relay(CConnman *connman);

    /* Reject structurally impossible sporks before the signature is checked.
       CheckSignature() signs the undelimited concatenation of nSporkID, nValue and nTimeSigned,
       so distinct field triples can render to the same signed string. Constraining each field to
       its legitimate domain makes that rendering unambiguous. See doc/release-notes for detail. */
    bool IsWellFormed(std::string& strErrorRet) const;
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;
    std::string strMasterPrivKey;
    std::map<int, CSporkMessage> mapSporksActive;

public:
    using Executor = std::function<void(void)>;
    CSporkManager() {}

    void ProcessSpork(CNode* pfrom, const std::string &strCommand, CDataStream& vRecv, CConnman *connman);
    bool UpdateSpork(int nSporkID, int64_t nValue, CConnman *connman);
    void ExecuteSpork(int nSporkID, int nValue);

    bool IsSporkActive(int nSporkID);
    int64_t GetSporkValue(int nSporkID);
    int GetSporkIDByName(std::string strName);
    std::string GetSporkNameByID(int nSporkID);
    static bool IsValidSporkID(int nSporkID);

    bool SetPrivKey(std::string strPrivKey);
};

#endif
