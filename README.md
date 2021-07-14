# EventHandler
a simple event handler written in c++ <br/>
this event handler run async on its own thread
# Usage
``` c++
#include <iostream>
#include <string>

#include "EventHandler.h"

int main() {
    EventHandler eventHandler;
    eventHandler
            .on("Hello", [] {
        std::cout << "Hello World" << std::endl;
    })
            .on<int>("print", [](int i) {
        std::cout << i << std::endl;
    });
    eventHandler.emit("Hello");
    eventHandler.emit("print", 10);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    eventHandler.exit();
}
```
# Note
this code may have logic or performance problems
