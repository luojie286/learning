#include<bits/stdc++.h>
using namespace std;

void check(){

}

int main(){
deque<int> q;

    for(int i = 0; i < n; i++) {

        // 维护单调性
        while(!q.empty() && check(nums[i], nums[q.back()])) {
            q.pop_back();
        }

        q.push_back(i);

        // 删除过期元素
        while(!q.empty() && q.front() <= i - window_size) {
            q.pop_front();
        }

        // 使用队首
        if(i >= window_size - 1) {
            // q.front()
        }
    }
}