/**
 * @file       BlynkProtocol.h
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Jan 2015
 * @brief      Blynk protocol implementation
 *
 */

#ifndef BlynkProtocol_h
#define BlynkProtocol_h

#include <string.h>
#include <stdlib.h>
#include "BlynkDebug.h"
#include "BlynkProtocolDefs.h"
#include "BlynkApi.h"
#include "BlynkUtility.h"

template <class Transp>
class BlynkProtocol
    : public BlynkApi< BlynkProtocol<Transp> >
{
    friend class BlynkApi< BlynkProtocol<Transp> >;
public:
    enum BlynkState {
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
    };

    BlynkProtocol(Transp& transp)
        : conn(transp)
        , authkey(NULL)
        , lastActivityIn(0)
        , lastActivityOut(0)
        , lastHeartbeat(0)
#ifdef BLYNK_MSG_LIMIT
        , deltaCmd(0)
#endif
        , currentMsgId(0)
        , state(CONNECTING)
    {}

    bool connected() { return state == CONNECTED; }

    bool connect(uint32_t timeout = BLYNK_TIMEOUT_MS*3) {
    	conn.disconnect();
    	state = CONNECTING;
    	millis_time_t started = this->getMillis();
    	while ((state != CONNECTED) &&
    	       (this->getMillis() - started < timeout))
    	{
    		run();
    	}
    	return state == CONNECTED;
    }

    void disconnect() {
        conn.disconnect();
        state = DISCONNECTED;
        BLYNK_LOG1(BLYNK_F("Disconnected"));
    }

    bool run(bool avail = false);

    // TODO: Fixme
    void startSession() {
        conn.connect();
    	state = CONNECTING;
#ifdef BLYNK_MSG_LIMIT
        deltaCmd = 1000;
#endif
    	currentMsgId = 0;
    	lastHeartbeat = lastActivityIn = lastActivityOut = this->getMillis(); // TODO: - 5005UL
    }

    void sendCmd(uint8_t cmd, uint16_t id = 0, const void* data = NULL, size_t length = 0, const void* data2 = NULL, size_t length2 = 0);

private:

    void internalReconnect() {
        state = CONNECTING;
        conn.disconnect();
        BlynkOnDisconnected();
    }

    int readHeader(BlynkHeader& hdr);
    uint16_t getNextMsgId();

protected:
    void begin(const char* auth) {
        BLYNK_LOG1(BLYNK_F("Blynk v" BLYNK_VERSION " on " BLYNK_INFO_DEVICE));
        this->authkey = auth;
    }
    bool processInput(void);

    Transp& conn;

private:
    const char* authkey;
    millis_time_t lastActivityIn;
    millis_time_t lastActivityOut;
    union {
    	millis_time_t lastHeartbeat;
    	millis_time_t lastLogin;
    };
#ifdef BLYNK_MSG_LIMIT
    millis_time_t deltaCmd;
#endif
    uint16_t currentMsgId;
protected:
    BlynkState state;
};

template <class Transp>
bool BlynkProtocol<Transp>::run(bool avail)
{
#if !defined(BLYNK_NO_YIELD)
    yield();
#endif

    if (state == DISCONNECTED) {
        return false;
    }

    const bool tconn = conn.connected();

    if (tconn) {
        if (avail || conn.available() > 0) {
            //BLYNK_LOG2(BLYNK_F("Available: "), conn.available());
            //const unsigned long t = micros();
            if (!processInput()) {
// TODO: Only when in direct mode?
#ifdef BLYNK_USE_DIRECT_CONNECT
                internalReconnect();
#endif
                return false;
            }
            //BLYNK_LOG2(BLYNK_F("Proc time: "), micros() - t);
        }
    }

    const millis_time_t t = this->getMillis();

    if (state == CONNECTED) {
        if (!tconn) {
            state = CONNECTING;
            lastHeartbeat = t;
            BlynkOnDisconnected();
            return false;
        }

        if (t - lastActivityIn > (1000UL * BLYNK_HEARTBEAT + BLYNK_TIMEOUT_MS*3)) {
#ifdef BLYNK_DEBUG
            BLYNK_LOG6(BLYNK_F("Heartbeat timeout: "), t, BLYNK_F(", "), lastActivityIn, BLYNK_F(", "), lastHeartbeat);
#else
            BLYNK_LOG1(BLYNK_F("Heartbeat timeout"));
#endif
            internalReconnect();
            return false;
        } else if ((t - lastActivityIn  > 1000UL * BLYNK_HEARTBEAT ||
                    t - lastActivityOut > 1000UL * BLYNK_HEARTBEAT) &&
                    t - lastHeartbeat   > BLYNK_TIMEOUT_MS)
        {
            // Send ping if we didn't either send or receive something
            // for BLYNK_HEARTBEAT seconds
            sendCmd(BLYNK_CMD_PING);
            lastHeartbeat = t;
        }
#ifndef BLYNK_USE_DIRECT_CONNECT
    } else if (state == CONNECTING) {
        if (tconn && (t - lastLogin > BLYNK_TIMEOUT_MS)) {
            BLYNK_LOG1(BLYNK_F("Login timeout"));
            conn.disconnect();
            state = CONNECTING;
            return false;
        } else if (!tconn && (t - lastLogin > 5000UL)) {
            conn.disconnect();
            if (!conn.connect()) {
                lastLogin = t;
                return false;
            }

#ifdef BLYNK_MSG_LIMIT
            deltaCmd = 1000;
#endif
            sendCmd(BLYNK_CMD_LOGIN, 1, authkey, strlen(authkey));
            lastLogin = lastActivityOut;
            return true;
        }
#else
    } else if (state == CONNECTING) {
    	if (!tconn)
    		conn.connect();
#endif
    }
    return true;
}

template <class Transp>
BLYNK_FORCE_INLINE
bool BlynkProtocol<Transp>::processInput(void)
{
    BlynkHeader hdr;
    const int ret = readHeader(hdr);

    if (ret == 0) {
        return true; // Considered OK (no data on input)
    }

    if (ret < 0 || hdr.msg_id == 0) {
#ifdef BLYNK_DEBUG
        BLYNK_LOG2(BLYNK_F("Bad hdr len: "), ret);
#endif
        return false;
    }

    if (hdr.type == BLYNK_CMD_RESPONSE) {
        lastActivityIn = this->getMillis();

#ifndef BLYNK_USE_DIRECT_CONNECT
        if (state == CONNECTING && (1 == hdr.msg_id)) {
            switch (hdr.length) {
            case BLYNK_SUCCESS:
            case BLYNK_ALREADY_LOGGED_IN:
                BLYNK_LOG3(BLYNK_F("Ready (ping: "), lastActivityIn-lastHeartbeat, BLYNK_F("ms)."));
                lastHeartbeat = lastActivityIn;
                state = CONNECTED;
                this->sendInfo();
#if !defined(BLYNK_NO_YIELD)
                yield();
#endif
                BlynkOnConnected();
                return true;
            case BLYNK_INVALID_TOKEN:
                BLYNK_LOG1(BLYNK_F("Invalid auth token"));
                break;
            default:
                BLYNK_LOG2(BLYNK_F("Connect failed. code: "), hdr.length);
            }
            return false;
        }
        if (BLYNK_NOT_AUTHENTICATED == hdr.length) {
            return false;
        }
#endif
        // TODO: return code may indicate App presence
        return true;
    }

    if (hdr.length > BLYNK_MAX_READBYTES) {
#ifdef BLYNK_DEBUG
        BLYNK_LOG2(BLYNK_F("Packet too big: "), hdr.length);
#endif
        // TODO: Flush
        conn.connect();
        return true;
    }

    uint8_t inputBuffer[hdr.length+1]; // Add 1 to zero-terminate
    if (hdr.length != conn.read(inputBuffer, hdr.length)) {
#ifdef DEBUG
        BLYNK_LOG1(BLYNK_F("Can't read body"));
#endif
        return false;
    }
    inputBuffer[hdr.length] = '\0';

    BLYNK_DBG_DUMP(">", inputBuffer, hdr.length);

    lastActivityIn = this->getMillis();

    switch (hdr.type)
    {
    case BLYNK_CMD_LOGIN: {
#ifdef BLYNK_USE_DIRECT_CONNECT
        if (!strncmp(authkey, (char*)inputBuffer, 32)) {
            BLYNK_LOG1(BLYNK_F("Ready"));
            state = CONNECTED;
            sendCmd(BLYNK_CMD_RESPONSE, hdr.msg_id, NULL, BLYNK_SUCCESS);
            this->sendInfo();
            BlynkOnConnected();
        } else {
            BLYNK_LOG1(BLYNK_F("Invalid token"));
            sendCmd(BLYNK_CMD_RESPONSE, hdr.msg_id, NULL, BLYNK_INVALID_TOKEN);
        }
#else
        BLYNK_LOG1(BLYNK_F("Ready"));
		state = CONNECTED;
		sendCmd(BLYNK_CMD_RESPONSE, hdr.msg_id, NULL, BLYNK_SUCCESS);
		this->sendInfo();
#endif
    } break;
    case BLYNK_CMD_PING: {
        sendCmd(BLYNK_CMD_RESPONSE, hdr.msg_id, NULL, BLYNK_SUCCESS);
    } break;
    case BLYNK_CMD_HARDWARE:
    case BLYNK_CMD_BRIDGE: {
        currentMsgId = hdr.msg_id;
        this->processCmd(inputBuffer, hdr.length);
        currentMsgId = 0;
    } break;
    default: {
#ifdef BLYNK_DEBUG
        BLYNK_LOG2(BLYNK_F("Invalid header type: "), hdr.type);
#endif
        // TODO: Flush
        conn.connect();
    } break;
    }

    return true;
}

template <class Transp>
int BlynkProtocol<Transp>::readHeader(BlynkHeader& hdr)
{
    size_t rlen = conn.read(&hdr, sizeof(hdr));
    if (rlen == 0) {
        return 0;
    }

    if (sizeof(hdr) != rlen) {
        return -1;
    }

    BLYNK_DBG_DUMP(">", &hdr, sizeof(BlynkHeader));

    hdr.msg_id = ntohs(hdr.msg_id);
    hdr.length = ntohs(hdr.length);

    return rlen;
}

#ifndef BLYNK_SEND_THROTTLE
#define BLYNK_SEND_THROTTLE 0
#endif

#ifndef BLYNK_SEND_CHUNK
#define BLYNK_SEND_CHUNK 1024 // Just a big number
#endif

template <class Transp>
void BlynkProtocol<Transp>::sendCmd(uint8_t cmd, uint16_t id, const void* data, size_t length, const void* data2, size_t length2)
{
    if (0 == id) {
        id = getNextMsgId();
    }

    if (!conn.connected() || (cmd != BLYNK_CMD_RESPONSE && cmd != BLYNK_CMD_PING && cmd != BLYNK_CMD_LOGIN && state != CONNECTED) ) {
#ifdef BLYNK_DEBUG
        BLYNK_LOG2(BLYNK_F("Cmd skipped:"), cmd);
#endif
        return;
    }

    const int full_length = (sizeof(BlynkHeader)) +
                            (data  ? length  : 0) +
                            (data2 ? length2 : 0);

#if defined(BLYNK_SEND_ATOMIC) || defined(ESP8266) || defined(SPARK) || defined(PARTICLE) || defined(ENERGIA)
    // Those have more RAM and like single write at a time...

    uint8_t buff[full_length];

    BlynkHeader* hdr = (BlynkHeader*)buff;
    hdr->type = cmd;
    hdr->msg_id = htons(id);
    hdr->length = htons(length+length2);

    size_t pos = sizeof(BlynkHeader);
    if (data && length) {
        memcpy(buff + pos, data, length);
        pos += length;
    }
    if (data2 && length2) {
        memcpy(buff + pos, data2, length2);
    }

    size_t wlen = 0;
    while (wlen < full_length) {
        const size_t chunk = BlynkMin(size_t(BLYNK_SEND_CHUNK), full_length - wlen);
		BLYNK_DBG_DUMP("<", buff + wlen, chunk);
        const size_t w = conn.write(buff + wlen, chunk);
        ::delay(BLYNK_SEND_THROTTLE);
    	if (w == 0) {
#ifdef BLYNK_DEBUG
            BLYNK_LOG1(BLYNK_F("Cmd error"));
#endif
            conn.disconnect();
            state = CONNECTING;
            //BlynkOnDisconnected();
            return;
    	}
        wlen += w;
    }

#else

    BlynkHeader hdr;
    hdr.type = cmd;
    hdr.msg_id = htons(id);
    hdr.length = htons(length+length2);

	BLYNK_DBG_DUMP("<", &hdr, sizeof(hdr));
    size_t wlen = conn.write(&hdr, sizeof(hdr));
    ::delay(BLYNK_SEND_THROTTLE);

    if (cmd != BLYNK_CMD_RESPONSE) {
        if (length) {
            BLYNK_DBG_DUMP("<", data, length);
            wlen += conn.write(data, length);
            ::delay(BLYNK_SEND_THROTTLE);
        }
        if (length2) {
            BLYNK_DBG_DUMP("<", data2, length2);
            wlen += conn.write(data2, length2);
            ::delay(BLYNK_SEND_THROTTLE);
        }
    }

#endif

    if (wlen != full_length) {
#ifdef BLYNK_DEBUG
        BLYNK_LOG4(BLYNK_F("Sent "), wlen, '/', full_length);
#endif
        internalReconnect();
        return;
    }

#if defined BLYNK_MSG_LIMIT && BLYNK_MSG_LIMIT > 0
    const millis_time_t ts = this->getMillis();
    BlynkAverageSample<32>(deltaCmd, ts - lastActivityOut);
    lastActivityOut = ts;
    //BLYNK_LOG2(BLYNK_F("Delta: "), deltaCmd);
    if (deltaCmd < (1000/BLYNK_MSG_LIMIT)) {
        BLYNK_LOG_TROUBLE(BLYNK_F("flood-error"));
        internalReconnect();
    }
#else
    lastActivityOut = this->getMillis();
#endif

}

template <class Transp>
uint16_t BlynkProtocol<Transp>::getNextMsgId()
{
    static uint16_t last = 0;
    if (currentMsgId != 0)
        return currentMsgId;
    if (++last == 0)
        last = 1;
    return last;
}

#endif
