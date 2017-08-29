#ifndef APP_INOCOPY_H
#define APP_INOCOPY_H

namespace irr {

    class INoCopy {
        //INoCopy(const INoCopy& other) = delete;
        //INoCopy& operator = (const INoCopy& other) = delete;
    private:
        INoCopy(const INoCopy& other){
        };

        INoCopy& operator = (const INoCopy& other){
        };
    };

} //namespace irr

#endif //APP_INOCOPY_H

