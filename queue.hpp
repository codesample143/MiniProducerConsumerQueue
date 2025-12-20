#include <queue>
#include <functional>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <thread>


using namespace std;

class JobQueue{
    private:
        queue<function<void()>> jobs;
        mutex mtx;
        condition_variable cv;
    public:
        void push(function<void()> job);
        function<void()> pop();
        void worker();

        
};