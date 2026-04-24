#include <iostream>
#include<vector>
#include<queue>
using namespace std;
//拓扑排序本质是BFS

int main() {
    // int a, b;
    // while (cin >> a >> b) { // 注意 while 处理多个 case
    //     cout << a + b << endl;
    // }
    vector<int>ans;
    int n,m;
    cin>>n>>m;
    vector<vector<int>>grap(n+1,vector<int>());
    vector<int>cnt(n+1,0);
    queue<int>q;
    while(m--){
        int u,v;
        cin>>u>>v;
        grap[u].push_back(v);
        cnt[v]++;
    }
    for(int i=1;i<=n;i++){
        if(cnt[i]==0) q.push(i);
    }
    while(!q.empty()){
        int p=q.front();
        q.pop();
        ans.push_back(p);
        for(int x:grap[p]){
            --cnt[x];
            if(cnt[x]==0) q.push(x);
        }
    }
    if(ans.size()!=n) cout<<-1<<endl;
    else{
        for(int i=0;i<ans.size()-1;i++) cout<<ans[i]<<" ";
        cout<<ans[ans.size()-1]<<endl;
    }
}
// 64 位输出请用 printf("%lld")