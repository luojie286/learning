#include <iostream>
#include<vector>
using namespace std;

class DSU{
public:
    vector<int>parent;
    DSU(int n){
        parent.resize(n+1);
        for(int i=0;i<=n;i++){
            parent[i]=i;
        }
    }
    int find(int x) {
        if(parent[x]!=x){
            parent[x]=find(parent[x]);
        }
        return parent[x];
    }
    void unite(int x,int y){
        parent[find(x)]=find(y);
    }
    bool same(int x,int y){
        return find(x)==find(y);
    }
};

int main() {
    int n,m;
    cin>>n>>m;
    DSU du(n);
    while(m--){
        int opt;
        cin>>opt;
        if(opt==1){
            int x,y;
            cin>>x>>y;
            if(du.same(x,y)) cout<<"Yes"<<endl;
            else cout<<"No"<<endl;
        }else if(opt==2){
            int x,y;
            cin>>x>>y;
            du.unite(x,y);
        }
    }
}
// 64 位输出请用 printf("%lld")[]