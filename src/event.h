#pragma once

#include <memory>
#include <queue>
#include <utility>

#include "base.h"

// ripped off from PolyM, stripped of thread features = stupid
// but do look towards using core design for rest of stuff. smooth stuff feels a bit too-much-and-also-little
// or I just dont know my shit. that'd be #1.
//
// but also can use for message passing in non-threaded context...
//
// prob just put the real lib back first and giv it a spin lol.
namespace tol {
namespace event {

// using MsgUID = uint32_t; /** Type for Msg unique identifiers

/** Msg represents a simple message that doesn't have any payload data.
 * Msg ID identifies the type of the message. Msg ID can be queried with getMsgId().  */
class Msg: public Named {
public:
    // Msg(int msgId);
    Msg(int msgId):
      Named(), msgId_(msgId) {}

    virtual ~Msg() = default;
    Msg(const Msg&) = delete;
    Msg& operator=(const Msg&) = delete;

    virtual std::unique_ptr<Msg> move() { /** "Virtual move constructor" */
      return std::unique_ptr<Msg>(new Msg(std::move(*this)));
    }

    /** Get Msg ID, identifies message type.  Multiple Msg instances can have the same Msg ID.  */
    int getMsgId() const { return msgId_; };
    /* Get Msg UID.  Msg UID is the unique ID associated with this message.  All Msg instances have a unique Msg UID.  */
    // MsgUID getUniqueId() const { return uniqueId_; }

protected:
    Msg(Msg&&) = default;
    Msg& operator=(Msg&&) = default;

private:
    int msgId_;
    // MsgUID uniqueId_;
};

/** DataMsg<PayloadType> is a Msg with payload of type PayloadType.
 * Payload is constructed when DataMsg is created and the DataMsg instance owns the payload data.  */
template<typename PayloadType>
class DataMsg: public Msg {
public:
    /** Construct DataMsg
     * @param msgId Msg ID
     * @param args Arguments for PayloadType ctor */
    template <typename ... Args>
    DataMsg(int msgId, Args&& ... args):
     Msg(msgId), pl_(new PayloadType(std::forward<Args>(args) ...)) {}
    virtual ~DataMsg() = default;
    DataMsg(const DataMsg&) = delete;
    DataMsg& operator=(const DataMsg&) = delete;

    virtual std::unique_ptr<Msg> move() override { /** "Virtual move constructor" */
        return std::unique_ptr<Msg>(new DataMsg<PayloadType>(std::move(*this)));
    }

    PayloadType& getPayload() const { return *pl_; } /** Get the payload data */

protected:
    DataMsg(DataMsg&&) = default;
    DataMsg& operator=(DataMsg&&) = default;

private:
    std::unique_ptr<PayloadType> pl_;
};


class Queue {
public:
    Queue() {}
    ~Queue() {}

    void put(Msg&& msg);
    std::unique_ptr<Msg> get();

private:
    std::queue<std::unique_ptr<Msg>> queue_; // Queue for the Msgs
    // std::map<MsgUID, std::unique_ptr<Queue>> responseMap_; // Map to keep track of which response handler queues are associated with which request Msgs
};

extern Queue bus;

}

}
