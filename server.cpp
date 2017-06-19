#include "tinysocket.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdio.h>
#include <time.h>
int main()
{
    ts::socket s(ts::protocol_type::udp);
    ts::ip_end_point point(ts::ip_address_any, 9090);
    s.bind(point);

    s.set_noblocking(true);

    printf("asdasda");

    char buffer[1024];

    time_t tm = time(nullptr);
    unsigned long long i = 0;
    while(1)
    {
        ts::ip_end_point p(ts::ip_address_any, 9090);
        i++;
        int sz = s.receive_from_some(buffer, 1024, p);
        if((time(nullptr) - tm) == 1)
        {
            tm = time(nullptr);
            std::cout << tm << " " << p << std::endl;
            i = 0;
        }



    }

    return 0;
}

