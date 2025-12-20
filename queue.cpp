#include "queue.hpp"

void JobQueue::push(function<void()> job){
    {
        unique_lock<mutex> lock(mtx);
        jobs.push(job);
    }
    cv.notify_one();

}

function<void()> JobQueue::pop(){
    unique_lock<mutex> lock(mtx);

    cv.wait(lock, [this] {
        return !jobs.empty();
    });

    auto job = jobs.front();
    jobs.pop();
    lock.unlock();
    return job;

}

void JobQueue::worker(){   
    while(true){
        std::function<void()> job = pop();  
        job();  
    }
}

