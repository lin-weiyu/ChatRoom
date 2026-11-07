#include "chatroom/version.h"

#include <iostream>

int main() {
    std::cout << "chatroom server " << chatroom::version() << " (" << chatroom::build_type()
              << ")\n";
    return 0;
}
