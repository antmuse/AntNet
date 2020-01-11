#ifndef APP_INOCOPY_H
#define APP_INOCOPY_H

namespace irr {

class INoCopy {
protected:
    INoCopy() {
    }
    ~INoCopy() {
    }

private:
    INoCopy(const INoCopy& other) = delete;
    INoCopy& operator = (const INoCopy& other) = delete;
};

} //namespace irr

#endif //APP_INOCOPY_H

