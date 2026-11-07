#include <iostream>

#include "chatroom/version.h"

int main() {
    std::cout << "chatroom server " << chatroom::version() << " ("
              << chatroom::build_type() << ")\n";
    return 0;
}
