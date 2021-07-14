#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <map>
#include <queue>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct BaseHandler {
    virtual ~BaseHandler() {}
};

template<typename... Args>
struct Handler : public BaseHandler {
    using func_t = std::function<void(Args...)>;
    func_t handler;
    Handler(func_t&& f) : handler{std::move(f)} {}
    Handler(func_t& f) : handler{f} {}
    void operator()(Args... args) {
        handler(args...);
    }
};

struct BaseEvent {
    virtual void operator()() = 0;
    virtual ~BaseEvent() {}
};

template<typename Func>
struct Event : BaseEvent {
    Func func;
    Event(Func&& f) : func{std::forward<Func>(f)} {}
    void operator()() {
        func();
    }
};

class EventHandler final {
public:
    EventHandler() {
        handlerThread = std::thread{&EventHandler::run, this};
    }
    ~EventHandler() {
        handlerThread.join();
    }
    template<typename... Args, typename Func>
    EventHandler& on(std::string eventName, Func&& f) {
        std::lock_guard lock{mtx};
        std::function<void(Args...)> func = std::forward<Func>(f);
        eventMap[eventName] = std::make_unique<Handler<Args...>>(std::move(func));
        return *this;
    }
    template<typename... Args>
    void emit(std::string eventName, Args... args) {
        std::lock_guard lock{mtx};
        auto baseHandler = eventMap[eventName].get();
        Handler<Args...>* handler = dynamic_cast<Handler<Args...>*> (baseHandler);
        auto bindedHandler = std::bind(&Handler<Args...>::operator(), handler, args...);
        eventQueue.push(std::make_unique<Event<decltype(bindedHandler)>>(std::move(bindedHandler)));
        cv.notify_one();
    }
    void run() {
        while(!done) {
            std::unique_lock lock{mtx};
            cv.wait(lock, [this] { return !eventQueue.empty() || done; });
            if (!eventQueue.empty()) {
                auto handler = std::move(eventQueue.front());
                eventQueue.pop();
                lock.unlock();
                (*handler)();
            }
        }
    }
    void exit() {
        done = true;
        cv.notify_one();
    }
private:
    std::map<std::string, std::unique_ptr<BaseHandler>> eventMap;
    std::queue<std::unique_ptr<BaseEvent>> eventQueue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{false};
    std::thread handlerThread;
};

#endif
