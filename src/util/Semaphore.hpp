#ifndef SEMAPHORE_UJFQCGWA
#define SEMAPHORE_UJFQCGWA

#include <mutex>

namespace StackOverflow {

    // http://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads
    class Semaphore {
    public:
        Semaphore (int count = 0)
            : _count(count) {}

        inline void notify()
        {
            std::unique_lock<std::mutex> lock(_mtx);
            ++_count;
            _cv.notify_one();
        }

        inline void wait()
        {
            std::unique_lock<std::mutex> lock(_mtx);

            while (_count == 0) {
                _cv.wait(lock);
            }
            _count--;
        }

    private:
        std::mutex _mtx;
        std::condition_variable _cv;
        int _count;
    };

}

#endif /* end of include guard: SEMAPHORE_UJFQCGWA */
