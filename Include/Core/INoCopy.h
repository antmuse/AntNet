#ifndef APP_INOCOPY_H
#define APP_INOCOPY_H

namespace app {

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

} //namespace app

#endif //APP_INOCOPY_H

