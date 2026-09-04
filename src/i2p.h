// Copyright (c) 2026 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_I2P_H
#define BITCOIN_I2P_H

#include <compat.h>

#include <string>

/** Default SAM proxy address exposed by i2pd / Java I2P. */
extern const std::string DEFAULT_I2P_SAM;
static const bool DEFAULT_LISTEN_I2P = false;

/** Open a stream to an I2P destination through the SAM proxy.
 *  Returns a connected SOCKET (the SAM data socket) on success, or
 *  INVALID_SOCKET on failure. The destination is either a raw base64
 *  I2P destination or a ".i2p" hostname.
 */
SOCKET ConnectI2P(const std::string& destination, int nTimeout);

/** Returns the local I2P destination (base64) if a session is established. */
bool GetI2PDestination(std::string& destination);

void StartI2PControl();
void InterruptI2PControl();
void StopI2PControl();

#endif // BITCOIN_I2P_H
