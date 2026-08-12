// Copyright (c) 2013-2022 The Dimecoin Core developers
// Copyright (c) 2013-2019 Feathercoin developers
// Copyright (c) 2011-2013 PPCoin developers
// Copyright (c) 2013 Primecoin developers
// Distributed under conditional MIT/X11 software license,
// see the accompanying file COPYING
//
// The synchronized checkpoint system is first developed by Sunny King for
// ppcoin network in 2012, giving cryptocurrency developers a tool to gain
// additional network protection against 51% attack.
//
// Primecoin also adopts this security mechanism, and the enforcement of
// checkpoints is explicitly granted by user, thus granting only temporary
// consensual central control to developer at the threats of 51% attack.
//
// Concepts
//
// In the network there can be a privileged node known as 'checkpoint master'.
// This node can send out checkpoint messages signed by the checkpoint master
// key. Each checkpoint is a block hash, representing a block on the blockchain
// that the network should reach consensus on.
//
// Besides verifying signatures of checkpoint messages, each node also verifies
// the consistency of the checkpoints. If a conflicting checkpoint is received,
// it means either the checkpoint master key is compromised, or there is an
// operator mistake. In this situation the node would discard the conflicting
// checkpoint message and display a warning message. This precaution controls
// the damage to network caused by operator mistake or compromised key.
//
// Operations
//
// Any node can be turned into checkpoint master by setting the 'checkpointkey'
// configuration parameter with the private key of the checkpoint master key.
// Operator should exercise caution such that at any moment there is at most
// one node operating as checkpoint master. When switching master node, the
// recommended procedure is to shutdown the master node and restart as
// regular node, note down the current checkpoint by 'getcheckpoint', then
// compare to the checkpoint at the new node to be upgraded to master node.
// When the checkpoint on both nodes match then it is safe to switch the new
// node to checkpoint master.
//
// The configuration parameter 'checkpointdepth' specifies how many blocks
// should the checkpoints lag behind the latest block in auto checkpoint mode.
// A depth of 5 is the minimum auto checkpoint policy and offers the greatest
// protection against 51% attack.
//

#include <checkpointsync.h>

#include <consensus/validation.h>
#include <chainparams.h>
#include <key_io.h>
#include <logging.h>
#include <netmessagemaker.h>
#include <txdb.h>
#include <validation.h>

// Synchronized checkpoint (centrally broadcasted)
std::string CSyncCheckpoint::strMasterPrivKey;
uint256 hashSyncCheckpoint;
static uint256 hashPendingCheckpoint GUARDED_BY(cs_hashSyncCheckpoint);
CSyncCheckpoint checkpointMessage;
static CSyncCheckpoint checkpointMessagePending GUARDED_BY(cs_hashSyncCheckpoint);
CCriticalSection cs_hashSyncCheckpoint;

// Drop every piece of cached sync-checkpoint state.
//
// This MUST be called whenever mapBlockIndex is torn down (UnloadBlockIndex),
// because hashSyncCheckpoint would otherwise keep naming a CBlockIndex that has
// just been deleted. The block-load retry loop in AppInitMain() unloads and
// reloads the index in-process, so a surviving hash there is later seen as
// "checkpoint set but absent from the block index" and used to abort the node.
void UnloadSyncCheckpoint()
{
    LOCK(cs_hashSyncCheckpoint);
    hashSyncCheckpoint = uint256();
    hashPendingCheckpoint = uint256();
    checkpointMessage.SetNull();
    checkpointMessagePending.SetNull();
}

// Only descendant of current sync-checkpoint is allowed
bool ValidateSyncCheckpoint(uint256 hashCheckpoint)
{
    // cs_main is held for the whole body: the pprev walks below traverse the block
    // index, and releasing it mid-walk would race the validation thread. Taking it
    // before cs_hashSyncCheckpoint also keeps the lock order consistent with every
    // other site in this file.
    LOCK2(cs_main, cs_hashSyncCheckpoint);

    CBlockIndex* pindexSyncCheckpoint = LookupBlockIndex(hashSyncCheckpoint);
    if (!pindexSyncCheckpoint)
        return error("%s: block index missing for current sync-checkpoint %s", __func__, hashSyncCheckpoint.ToString());

    CBlockIndex* pindexCheckpointRecv = LookupBlockIndex(hashCheckpoint);
    if (!pindexCheckpointRecv)
        return error("%s: block index missing for received sync-checkpoint %s", __func__, hashCheckpoint.ToString());

    if (pindexCheckpointRecv->nHeight <= pindexSyncCheckpoint->nHeight)
    {
        // Received an older checkpoint, trace back from current checkpoint
        // to the same height of the received checkpoint to verify
        // that current checkpoint should be a descendant block
        CBlockIndex* pindex = pindexSyncCheckpoint;
        while (pindex->nHeight > pindexCheckpointRecv->nHeight)
            if (!(pindex = pindex->pprev))
                return error("%s: pprev1 null - block index structure failure", __func__);
        if (pindex->GetBlockHash() != hashCheckpoint)
            return error("%s: new sync-checkpoint %s is conflicting with current sync-checkpoint %s", __func__, hashCheckpoint.ToString(), hashSyncCheckpoint.ToString());

        return false; // ignore older checkpoint
    }

    // Received checkpoint should be a descendant block of the current
    // checkpoint. Trace back to the same height of current checkpoint
    // to verify.
    CBlockIndex* pindex = pindexCheckpointRecv;
    while (pindex->nHeight > pindexSyncCheckpoint->nHeight)
        if (!(pindex = pindex->pprev))
            return error("%s: pprev2 null - block index structure failure", __func__);

    if (pindex->GetBlockHash() != hashSyncCheckpoint)
        return error("%s: new sync-checkpoint %s is not a descendant of current sync-checkpoint %s", __func__, hashCheckpoint.ToString(), hashSyncCheckpoint.ToString());

    return true;
}

bool WriteSyncCheckpoint(const uint256& hashCheckpoint)
{
    if (!pblocktree->WriteSyncCheckpoint(hashCheckpoint))
        return error("%s: failed to write to txdb sync checkpoint %s", __func__, hashCheckpoint.ToString());

    hashSyncCheckpoint = hashCheckpoint;
    return true;
}

bool AcceptPendingSyncCheckpoint()
{
    uint256 hashPendingCheckpointTmp;
    {
        LOCK2(cs_main, cs_hashSyncCheckpoint);
        if (hashPendingCheckpoint.IsNull() || !LookupBlockIndex(hashPendingCheckpoint))
            return false;
        hashPendingCheckpointTmp = hashPendingCheckpoint;
    }

    if (!ValidateSyncCheckpoint(hashPendingCheckpointTmp))
    {
        LOCK(cs_hashSyncCheckpoint);
        hashPendingCheckpoint = uint256();
        checkpointMessagePending.SetNull();
        return false;
    }

    CSyncCheckpoint checkpointRelay;
    {
        LOCK2(cs_main, cs_hashSyncCheckpoint);

        // Re-resolve under the lock we are about to act on: the pending hash may have
        // been cleared or replaced while ValidateSyncCheckpoint() ran unlocked.
        //
        // Commit only the hash that was actually validated. Re-reading the global here
        // would allow a checkpoint that arrived during the validation window to be
        // written on the strength of the previous one's validation, and would also pair
        // it with the wrong checkpointMessagePending. If it changed, bail out and let
        // the next call validate the new pending checkpoint on its own merits.
        if (hashPendingCheckpoint != hashPendingCheckpointTmp)
            return false;

        CBlockIndex* pindexPending = LookupBlockIndex(hashPendingCheckpointTmp);
        if (!pindexPending || !chainActive.Contains(pindexPending))
            return false;

        if (!WriteSyncCheckpoint(hashPendingCheckpointTmp)) {
            return error("%s: failed to write sync checkpoint %s", __func__, hashPendingCheckpointTmp.ToString());
        }

        hashPendingCheckpoint = uint256();
        checkpointMessage = checkpointMessagePending;
        checkpointMessagePending.SetNull();
        checkpointRelay = checkpointMessage;
    }

    // Relay the checkpoint outside cs_main so we never hold it across network I/O.
    if (g_connman && !checkpointRelay.IsNull())
    {
        g_connman->ForEachNode([&checkpointRelay](CNode* pnode) {
            if (pnode->supportACPMessages)
                checkpointRelay.RelayTo(pnode);
        });
    }

    return true;
}

// Automatically select a suitable sync-checkpoint
uint256 AutoSelectSyncCheckpoint()
{
    LOCK(cs_main);
    // Search backward for a block with specified depth policy
    const CBlockIndex *pindex = chainActive.Tip();
    if (!pindex)
        return uint256();
    while (pindex->pprev && pindex->nHeight + gArgs.GetArg("-checkpointdepth", DEFAULT_AUTOCHECKPOINT) > chainActive.Tip()->nHeight)
        pindex = pindex->pprev;
    return pindex->GetBlockHash();
}

// Check against synchronized checkpoint
bool CheckSyncCheckpoint(const uint256 hashBlock, const int nHeight, const CBlockIndex* pindexPrev)
{
    // Genesis block
    if (nHeight == 0) {
        return true;
    }

    // Return true if still in IBD/importing
    if (fReindex || fImporting || !ibd_complete) {
        return true;
    }

    {
        LOCK(cs_hashSyncCheckpoint);

        // Checkpoint on default
        if (hashSyncCheckpoint == uint256()) {
            return true;
        }
    }

    // cs_main is held for the remainder of the function: pindexSync and the pprev
    // walk below both read the block index, and dropping the lock mid-walk would
    // race a concurrent reorg. Acquiring cs_main first also matches the order used
    // by ValidateSyncCheckpoint(), avoiding an AB-BA deadlock between the two.
    LOCK2(cs_main, cs_hashSyncCheckpoint);

    // The sync-checkpoint should always name an accepted block, but a rebuilt or
    // truncated block index can leave a stale hash behind (see UnloadSyncCheckpoint).
    // Recover by falling back to genesis rather than aborting the process.
    // Note this returns true: the caller responds to false with DoS(100), which would
    // ban peers for what is purely local state damage.
    const CBlockIndex* pindexSync = LookupBlockIndex(hashSyncCheckpoint);
    if (!pindexSync) {
        LogPrintf("%s: WARNING: sync-checkpoint %s missing from block index, resetting to genesis\n", __func__, hashSyncCheckpoint.ToString());
        if (!WriteSyncCheckpoint(Params().GetConsensus().hashGenesisBlock))
            LogPrintf("%s: ERROR: failed to reset sync-checkpoint to genesis block\n", __func__);
        return true;
    }

    if (nHeight > pindexSync->nHeight)
    {
        const CBlockIndex* pindex = pindexPrev ? pindexPrev : chainActive.Tip();
        if (!pindex)
            return error("%s: no block index available to compare against sync-checkpoint", __func__);

        // Our reference chain has not yet reached the sync-checkpoint height, so we
        // hold no evidence either way about this block's ancestry. That is the normal
        // state while catching up — the checkpoint arrives from the network long
        // before we have connected the blocks up to it. Express "no opinion" by
        // accepting: returning false here rejects perfectly valid headers and charges
        // the peer that sent them DoS points for our own missing chain data.
        if (pindex->nHeight < pindexSync->nHeight)
            return true;

        // Trace back to same height as sync-checkpoint
        while (pindex->nHeight > pindexSync->nHeight)
            if (!(pindex = pindex->pprev))
                return error("%s: pprev null - block index structure failure", __func__);

        if (pindex->GetBlockHash() != hashSyncCheckpoint)
            return error("%s: mismatched block hash at sync height. height %d block hash %s sync-checkpoint %s", __func__, pindex->nHeight, pindex->GetBlockHash().ToString(), hashSyncCheckpoint.ToString());
    }

    if (nHeight == pindexSync->nHeight && hashBlock != hashSyncCheckpoint)
        return error("%s: Same height with sync-checkpoint", __func__);

    if (nHeight < pindexSync->nHeight && !LookupBlockIndex(hashBlock))
        return error("%s: Lower height than sync-checkpoint", __func__);

    return true;
}

// Reset synchronized checkpoint to the genesis block
bool ResetSyncCheckpoint()
{
    LOCK(cs_hashSyncCheckpoint);

    if (!WriteSyncCheckpoint(Params().GetConsensus().hashGenesisBlock))
        return error("%s: failed to reset sync checkpoint to genesis block", __func__);

    return true;
}

// Verify sync checkpoint master pubkey and reset sync checkpoint if changed
bool CheckCheckpointPubKey()
{
    std::string strPubKey = "";
    std::string strMasterPubKey = Params().GetConsensus().checkpointPubKey;

    if (!pblocktree->ReadCheckpointPubKey(strPubKey) || strPubKey != strMasterPubKey)
    {
        // write checkpoint master key to db
        if (!ResetSyncCheckpoint())
            return error("%s: failed to reset sync-checkpoint", __func__);
        if (!pblocktree->WriteCheckpointPubKey(strMasterPubKey))
            return error("%s: failed to write new checkpoint master key to db", __func__);
    }

    return true;
}

bool SetCheckpointPrivKey(std::string strPrivKey)
{
    CKey key = DecodeSecret(strPrivKey);
    if (!key.IsValid())
        return false;

    // Mock signing to see if key valid
    CSyncCheckpoint checkpoint;
    checkpoint.hashCheckpoint = Params().GetConsensus().hashGenesisBlock;
    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << static_cast<CUnsignedSyncCheckpoint>(checkpoint);
    checkpoint.vchMsg = std::vector<unsigned char>(sMsg.begin(), sMsg.end());

    if (!key.Sign(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig))
        return false;

    std::string strMasterPubKey = Params().GetConsensus().checkpointPubKey;
    CPubKey pubkey(ParseHex(strMasterPubKey));
    if (!pubkey.Verify(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig))
        return false;

    CSyncCheckpoint::strMasterPrivKey = strPrivKey;
    return true;
}

bool SendSyncCheckpoint(uint256 hashCheckpoint)
{
    // P2P disabled
    if (!g_connman)
        return true;

    // No connections
    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0)
        return true;

    // Do not send dummy checkpoint
    if (hashCheckpoint == uint256())
        return true;

    CSyncCheckpoint checkpoint;
    checkpoint.hashCheckpoint = hashCheckpoint;
    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << static_cast<CUnsignedSyncCheckpoint>(checkpoint);
    checkpoint.vchMsg = std::vector<unsigned char>(sMsg.begin(), sMsg.end());

    if (CSyncCheckpoint::strMasterPrivKey.empty())
        return error("%s: Checkpoint master key unavailable.", __func__);

    CKey key = DecodeSecret(CSyncCheckpoint::strMasterPrivKey);
    if (!key.IsValid())
        return error("%s: Checkpoint master key invalid", __func__);

    if (!key.Sign(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig))
        return error("%s: Unable to sign checkpoint, check private key?", __func__);

    if(!checkpoint.ProcessSyncCheckpoint())
        return error("%s: Failed to process checkpoint.", __func__);

    // Relay checkpoint
    g_connman->ForEachNode([checkpoint](CNode* pnode) {
        checkpoint.RelayTo(pnode);
    });

    return true;
}


void CUnsignedSyncCheckpoint::SetNull()
{
    nVersion = 1;
    hashCheckpoint = uint256();
}

std::string CUnsignedSyncCheckpoint::ToString() const
{
    return strprintf(
            "CSyncCheckpoint(\n"
            "    nVersion       = %d\n"
            "    hashCheckpoint = %s\n"
            ")\n",
        nVersion,
        hashCheckpoint.ToString());
}

CSyncCheckpoint::CSyncCheckpoint()
{
    SetNull();
}

void CSyncCheckpoint::SetNull()
{
    CUnsignedSyncCheckpoint::SetNull();
    vchMsg.clear();
    vchSig.clear();
}

bool CSyncCheckpoint::IsNull() const
{
    return (hashCheckpoint == uint256());
}

uint256 CSyncCheckpoint::GetHash() const
{
    return Hash(this->vchMsg.begin(), this->vchMsg.end());
}

void CSyncCheckpoint::RelayTo(CNode* pfrom) const
{
    if (g_connman && pfrom->hashCheckpointKnown != hashCheckpoint && pfrom->supportACPMessages)
    {
        pfrom->hashCheckpointKnown = hashCheckpoint;
        g_connman->PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::CHECKPOINT, *this));
    }
}

// Verify signature of sync-checkpoint message
bool CSyncCheckpoint::CheckSignature()
{
    std::string strMasterPubKey = Params().GetConsensus().checkpointPubKey;
    CPubKey key(ParseHex(strMasterPubKey));
    if (!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
        return error("%s: verify signature failed", __func__);

    // Now unserialize the data
    CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sMsg >> *static_cast<CUnsignedSyncCheckpoint*>(this);
    return true;
}

// Process synchronized checkpoint
bool CSyncCheckpoint::ProcessSyncCheckpoint()
{
    if (!ibd_complete)
        return true;

    if (!CheckSignature())
        return false;

    {
        LOCK2(cs_main, cs_hashSyncCheckpoint);

        if (!LookupBlockIndex(hashCheckpoint)) {
            // We haven't received the checkpoint chain, keep the checkpoint as pending
            hashPendingCheckpoint = hashCheckpoint;
            checkpointMessagePending = *this;
            LogPrintf("%s: pending for sync-checkpoint %s\n", __func__, hashCheckpoint.ToString());

            return false;
        }
    }

    if (!ValidateSyncCheckpoint(hashCheckpoint)) {
        return false;
    }

    CBlockIndex* bad_fork = nullptr;
    CBlockIndex* index = nullptr;
    {
        LOCK2(cs_main, cs_hashSyncCheckpoint);

        // Check if we're on a fork
        index = LookupBlockIndex(hashCheckpoint);
        if (!index)
            return error("%s: block index missing for sync-checkpoint %s", __func__, hashCheckpoint.ToString());

        if (!chainActive.Contains(index)) {
            const CBlockIndex* tip = chainActive.Tip();
            const CBlockIndex* ancestor = tip ? LastCommonAncestor(index, tip) : nullptr;
            if (ancestor)
                bad_fork = chainActive.Next(ancestor);
        }
    }

    if (bad_fork && index && index->GetAncestor(bad_fork->nHeight) != bad_fork) {
        CValidationState state;
        InvalidateBlock(state, Params(), bad_fork);

        if (state.IsValid()) {
            ActivateBestChain(state, Params());
        }
    }

    {
        LOCK(cs_hashSyncCheckpoint);
        if (!WriteSyncCheckpoint(hashCheckpoint)) {
            return error("%s: failed to write sync checkpoint %s\n", __func__, hashCheckpoint.ToString());
        }

        checkpointMessage = *this;
        hashPendingCheckpoint = uint256();
        checkpointMessagePending.SetNull();
    }

    return true;
}
