// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MESSAGESIGNER_H
#define MESSAGESIGNER_H

#include <key.h>
#include <script/standard.h>

/** Accept masternode-layer signatures made under an alternate "DarkCoin"/"Dimecoin" message magic
 *  in addition to the currently configured strMessageMagic.
 *
 *  Default ON. The live network -- including the shipped 2.3.0.0 release and every masternode on
 *  it -- signs with "DarkCoin Signed Message:\n", so that is what strMessageMagic must remain and
 *  what this node signs with. Note the "Dimecoin" magic present on main was never part of the
 *  2.3.0.0 release, so a tree built from main is NOT a valid compatibility reference.
 *
 *  This fallback additionally accepts the "Dimecoin" magic on verification only. That is the
 *  first step of an accept-before-emit migration: every node must accept the new magic before any
 *  node is allowed to emit it. Signing deliberately stays single-magic -- a node that signed the
 *  new magic today would be invisible to the entire network and, if it were a masternode, would
 *  stop being paid.
 *
 *  Verification order keeps the cost of this at zero for real traffic: the configured magic is
 *  tried first and succeeds for all current network messages; the alternate is only attempted
 *  after that fails. Operators can set -legacysigmagic=0 to enforce the configured magic strictly.
 *  GetLegacyMagicAcceptCount() is the trigger for the final phase: once a release cycle passes
 *  with a non-zero count the network has migrated, and once it is zero after the emit flip the
 *  fallback can be deleted. */
static const bool DEFAULT_LEGACY_SIG_MAGIC = true;

/** Helper class for signing messages and checking their signatures
 */
class CMessageSigner
{
public:
    /// Set the private/public key values, returns true if successful
    static bool GetKeysFromSecret(const std::string strSecret, CKey& keyRet, CPubKey& pubkeyRet);
    /// Sign the message, returns true if successful
    static bool SignMessage(const std::string strMessage, std::vector<unsigned char>& vchSigRet, const CKey key);
    /// Verify the message signature, returns true if succcessful
    static bool VerifyMessage(const CPubKey pubkey, const std::vector<unsigned char>& vchSig, const std::string strMessage, std::string& strErrorRet);
    /// How many signatures have only verified under a legacy magic string. Zero over a full
    /// release cycle is the signal that the fallback above can be removed.
    static int64_t GetLegacyMagicAcceptCount();
};

/** Helper class for signing hashes and checking their signatures
 */
class CHashSigner
{
public:
    /// Sign the hash, returns true if successful
    static bool SignHash(const uint256& hash, const CKey key, std::vector<unsigned char>& vchSigRet);
    /// Verify the hash signature, returns true if succcessful
    static bool VerifyHash(const uint256& hash, const CPubKey pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
};

#endif
