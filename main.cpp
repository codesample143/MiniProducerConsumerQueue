#include "queue.hpp"

#include <iostream>

using namespace std;
int main(){
    JobQueue jq;
    vector<thread> workers;
    mutex log_mtx;

    for(int i = 0; i < 4; ++i)
        workers.emplace_back([&jq]{
            while(true){
                auto job = jq.pop();
                job();
            }
    });

    for(int i = 0; i < 100; ++i){
    jq.push([&log_mtx](){
        float x = rand() / float(RAND_MAX);
        std::lock_guard<std::mutex> lock(log_mtx);
        cout << "Job processed: " << x << endl;
    });
    }

    for (auto& t : workers) {
        t.join();   
    }

}