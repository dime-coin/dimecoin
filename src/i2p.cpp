// Copyright (c) 2026 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <i2p.h>
#include <i2p_sam.h>

#include <net.h>
#include <netbase.h>
#include <util/system.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

const std::string DEFAULT_I2P_SAM = "127.0.0.1:7656";

static std::thread i2pThread;
static std::atomic<bool> g_i2p_interrupt(false);
static bool g_i2p_started(false);
static std::string g_local_dest;

/** Parse the -i2psam argument (host:port) into separate host and port. */
static bool GetSamAddress(std::string& host, int& port)
{
    std::string arg = gArgs.GetArg("-i2psam", DEFAULT_I2P_SAM);
    size_t pos = arg.rfind(':');
    if (pos == std::string::npos) {
        host = arg;
        port = 7656;
        return true;
    }
    host = arg.substr(0, pos);
    port = atoi(arg.substr(pos + 1).c_str());
    if (port <= 0)
        port = 7656;
    return true;
}

SOCKET ConnectI2P(const std::string& destination, int nTimeout)
{
    std::string host;
    int port;
    if (!GetSamAddress(host, port))
        return INVALID_SOCKET;

    i2p::SamConnection c;
    if (!c.Connect(host, port)) {
        LogPrintf("i2p: failed to connect to SAM proxy %s:%d\n", host, port);
        return INVALID_SOCKET;
    }
    if (!c.Hello()) {
        LogPrintf("i2p: HELLO handshake with SAM proxy failed\n");
        c.Close();
        return INVALID_SOCKET;
    }
    std::string mydest;
    if (!c.CreateSession("i2p", mydest, "TRANSIENT", 7)) {
        LogPrintf("i2p: SESSION CREATE failed\n");
        c.Close();
        return INVALID_SOCKET;
    }
    i2p_socket_t data = c.StreamConnect("i2p", destination);
    if (data == INVALID_SOCKET) {
        c.Close();
        return INVALID_SOCKET;
    }
    return static_cast<SOCKET>(data);
}

bool GetI2PDestination(std::string& destination)
{
    if (g_local_dest.empty())
        return false;
    destination = g_local_dest;
    return true;
}

static void I2PThread()
{
    std::string host;
    int port;
    if (!GetSamAddress(host, port)) {
        LogPrintf("i2p: no SAM proxy address configured\n");
        return;
    }

    while (!g_i2p_interrupt) {
        i2p::SamConnection c;
        if (c.Connect(host, port) && c.Hello()) {
            std::string dest;
            if (c.CreateSession("i2p", dest, "TRANSIENT", 7)) {
                g_local_dest = dest;
                CNetAddr addr;
                addr.SetI2P(dest);
                CService service(addr, GetListenPort());
                LogPrintf("i2p: session established, local destination %s...\n", dest.substr(0, 16));
                AddLocal(service, LOCAL_MANUAL);
                // Keep the SAM control connection open so the session stays
                // alive until interrupted. Inbound I2P traffic is expected to
                // be forwarded to the node's listen port by an external I2P
                // router (e.g. an i2pd server tunnel).
                while (!g_i2p_interrupt) {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
                RemoveLocal(service);
            } else {
                LogPrintf("i2p: SESSION CREATE failed\n");
            }
        } else {
            LogPrintf("i2p: failed to connect/hello to SAM proxy %s:%d\n", host, port);
        }
        if (g_i2p_interrupt)
            break;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void StartI2PControl()
{
    if (g_i2p_started)
        return;
    g_i2p_interrupt = false;
    g_i2p_started = true;
    i2pThread = std::thread(I2PThread);
}

void InterruptI2PControl()
{
    g_i2p_interrupt = true;
}

void StopI2PControl()
{
    if (!g_i2p_started)
        return;
    g_i2p_interrupt = true;
    if (i2pThread.joinable())
        i2pThread.join();
    g_i2p_started = false;
}
