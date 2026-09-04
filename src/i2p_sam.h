// Copyright (c) 2026 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_I2P_SAM_H
#define BITCOIN_I2P_SAM_H

#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>

#ifdef WIN32
#include <winsock2.h>
typedef SOCKET i2p_socket_t;
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef int i2p_socket_t;
#endif

namespace i2p {

bool ParseReply(const std::string& reply, std::string& result,
                std::map<std::string, std::string>& kv) {
    result.clear();
    kv.clear();
    // Tokenize by whitespace.
    std::vector<std::string> tokens;
    std::string cur;
    for (char c : reply) {
        if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    if (tokens.size() < 3) return false;          // need CMD REPLY|STATUS RESULT=...
    if (tokens[1] != "REPLY" && tokens[1] != "STATUS") return false;
    // tokens[2] is "RESULT=OK" or "RESULT=NOVERSION" etc.
    const std::string& r = tokens[2];
    if (r.rfind("RESULT=", 0) != 0) return false;
    result = r.substr(std::string("RESULT=").size());
    for (size_t i = 3; i < tokens.size(); ++i) {
        auto eq = tokens[i].find('=');
        if (eq != std::string::npos) {
            kv[tokens[i].substr(0, eq)] = tokens[i].substr(eq + 1);
        }
    }
    return true;
}

std::string BuildHello() {
    return "HELLO VERSION MIN=3.0 MAX=3.0\n";
}

std::string BuildSessionCreate(const std::string& id,
                               const std::string& destination,
                               int signature_type) {
    std::string s = "SESSION CREATE ID=" + id +
                    " STYLE=STREAM DESTINATION=" + destination +
                    " SIGNATURE_TYPE=" + std::to_string(signature_type) + "\n";
    return s;
}

std::string BuildStreamConnect(const std::string& id, const std::string& dest) {
    return "STREAM CONNECT ID=" + id + " DESTINATION=" + dest + "\n";
}

std::string BuildStreamAccept(const std::string& id) {
    return "STREAM ACCEPT ID=" + id + "\n";
}

class SamConnection {
public:
    SamConnection() : m_sock(-1) {}

    bool Connect(const std::string& host, int port) {
        Close();
        struct addrinfo hints, *res = nullptr;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        std::string portstr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portstr.c_str(), &hints, &res) != 0)
            return false;
        for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
            m_sock = (i2p_socket_t)::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (m_sock < 0) continue;
            if (::connect(m_sock, p->ai_addr, p->ai_addrlen) == 0) break;
            Close();
        }
        if (res) freeaddrinfo(res);
        return m_sock >= 0;
    }

    bool Hello() {
        if (m_sock < 0) return false;
        if (!SendLine(BuildHello())) return false;
        std::string line;
        if (!RecvLine(line)) return false;
        std::string result; std::map<std::string, std::string> kv;
        return ParseReply(line, result, kv) && result == "OK";
    }

    bool CreateSession(const std::string& id, std::string& myDestination,
                       const std::string& destination, int signature_type) {
        if (m_sock < 0) return false;
        if (!SendLine(BuildSessionCreate(id, destination, signature_type))) return false;
        std::string line;
        if (!RecvLine(line)) return false;
        std::string result; std::map<std::string, std::string> kv;
        if (!ParseReply(line, result, kv)) return false;
        if (result != "OK") return false;
        auto it = kv.find("DESTINATION");
        if (it == kv.end()) return false;
        myDestination = it->second;
        return true;
    }

    i2p_socket_t StreamConnect(const std::string& id, const std::string& dest) {
        if (m_sock < 0) return -1;
        if (!SendLine(BuildStreamConnect(id, dest))) return -1;
        std::string line;
        if (!RecvLine(line)) return -1;
        std::string result; std::map<std::string, std::string> kv;
        if (!ParseReply(line, result, kv)) return -1;
        if (result != "OK") return -1;
        i2p_socket_t data = m_sock;
        m_sock = -1;            // control socket becomes the data socket
        return data;
    }

    i2p_socket_t StreamAccept(const std::string& id) {
        if (m_sock < 0) return -1;
        if (!SendLine(BuildStreamAccept(id))) return -1;
        std::string line;
        if (!RecvLine(line)) return -1;
        std::string result; std::map<std::string, std::string> kv;
        if (!ParseReply(line, result, kv)) return -1;
        if (result != "OK") return -1;
        i2p_socket_t data = m_sock;
        m_sock = -1;
        return data;
    }

    void Close() {
        if (m_sock >= 0) {
#ifdef WIN32
            closesocket(m_sock);
#else
            ::close(m_sock);
#endif
            m_sock = -1;
        }
    }

    i2p_socket_t ControlSocket() const { return m_sock; }

private:
    bool SendLine(const std::string& line) {
        size_t sent = 0;
        while (sent < line.size()) {
            ssize_t n = ::send(m_sock, line.data() + sent, line.size() - sent, 0);
            if (n <= 0) return false;
            sent += (size_t)n;
        }
        return true;
    }

    bool RecvLine(std::string& line) {
        line.clear();
        char c;
        while (true) {
            ssize_t n = ::recv(m_sock, &c, 1, 0);
            if (n <= 0) return false;
            if (c == '\n') break;
            if (c == '\r') continue;
            line += c;
            if (line.size() > 8192) return false; // sanity bound
        }
        return true;
    }

    i2p_socket_t m_sock;
};

} // namespace i2p

#endif /* BITCOIN_I2P_SAM_H */
