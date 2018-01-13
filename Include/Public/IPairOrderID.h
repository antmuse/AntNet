#ifndef APP_IPAIRORDERID_H
#define APP_IPAIRORDERID_H


namespace irr {

template <class A, class B>
class IPairOrderID {
public:
    IPairOrderID(A v1, B v2) : mIP(v1), mPort(v2) {
    }

    IPairOrderID(const IPairOrderID& other) {
        mIP = other.mIP;
        mPort = other.mPort;
    }

    IPairOrderID& operator = (const IPairOrderID& other) {
        mIP = other.mIP;
        mPort = other.mPort;
        return *this;
    }

    bool operator==(const IPairOrderID& other) const {
        return mIP == other.mIP && mPort == other.mPort;
    }

    bool operator!=(const IPairOrderID& other) const {
        return mIP != other.mIP || mPort != other.mPort;
    }

    bool operator>(const IPairOrderID& other) const {
        return mIP > other.mIP || (mIP == other.mIP && mPort > other.mPort);
    }

    bool operator>=(const IPairOrderID& other) const {
        return mIP > other.mIP || (mIP == other.mIP && mPort >= other.mPort);
    }

    bool operator<(const IPairOrderID& other) const {
        return mIP < other.mIP || (mIP == other.mIP && mPort < other.mPort);
    }

    bool operator<=(const IPairOrderID& other) const {
        return mIP < other.mIP || (mIP == other.mIP && mPort <= other.mPort);
    }

    const A& getKey()const {
        return mIP;
    }

    const B& getValue()const {
        return mPort;
    }

    void set(const A& key, const B& value) {
        mIP = key;
        mPort = value;
    }

private:
    A mIP;
    B mPort;
    IPairOrderID() {
    }
};

} //namespace irr

#endif //APP_IPAIRORDERID_H

