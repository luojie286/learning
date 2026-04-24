#include<bits/stdc++.h>
using namespace std;
using ll=long long;
using ld=long double;
const ll MOD=1e9+7;

struct path{
    int u,v,weight;
    path(int x,int y,int z) :u(x),v(y),weight(z){}
};

class DSU{
    vector<int>parent;
public:
    DSU (int n) {
        parent.resize(n+1);
        for(int i=0;i<=n;i++) parent[i] = i;
    }
    int find(int x){
        if(parent[x]!=x){
            parent[x]=find(parent[x]);
        }
        return parent[x];
    }
    void unite(int x,int y){
        parent[find(x)]=find(y);
    }
    bool same(int x,int y) {
        return find(x) == find(y);
    }
};

void solve(){
    int n,m;
    cin>>n>>m;
    DSU du(n);
    vector<path>pathes;
    while(m--){
        int x,y,z;
        cin>>x>>y>>z;
        path p(x,y,z);
        pathes.push_back(p);
    }
    sort(pathes.begin(),pathes.end(),[](const path &a,const path &b){
        return a.weight<b.weight;
    });
    int ans=0;
    int cnt=0;
    for(path x:pathes){
        if(!du.same(x.u,x.v)){
            du.unite(x.u,x.v);
            ans+=x.weight;
            cnt++;
        }
    }
    if(cnt!=n-1){//判断是否出现环（连通性检查）
        cout<<"orz"<<endl;
        return;
    }
    cout<<ans<<endl;
}
int main(){
    ios::sync_with_stdio(false);
    cin.tie(NULL);
    solve();
    return 0;
}