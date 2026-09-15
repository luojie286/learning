    #include<iostream>
    #include<queue>
    #include<vector>
    #include<unordered_map>
    #include<algorithm>
    using namespace std;
    struct path{
        int u,v,weight;
        path(int x=0,int y=0,int z=0): u(x),v(y),weight(z){}
    };
    path help[100010];

    class DSU{
        vector<int>parent;
    public:
        vector<int>height;
        unordered_map<int,int>mp;
        DSU(int n){
            parent.resize(n+1);
            height.resize(n+1,1);
            for(int i=0;i<=n;i++) parent[i]=i;
        }
        int find(int x){
            if(parent[mp[x]]!=mp[x]){
                parent[mp[x]]=find(parent[mp[x]]);
            }
            return parent[mp[x]];
        }
        void unite(int x,int y){
            int rootX = find(x);
            int rootY = find(y);
            if(rootX == rootY) return;
            // 按高度合并
            if(height[rootX] < height[rootY]){
                parent[rootX] = rootY;
            } else if(height[rootX] > height[rootY]){
                parent[rootY] = rootX;
            } else {
                parent[rootY] = rootX;
                height[rootX]++;
            }
        }
        bool same(int x,int y){
            return find(x)==find(y);
        }
    };

    void merge(vector<path> &p,int l,int m,int r){
        int i=l,j=m+1;
        int indx=l;
        while(i<=m&&j<=r){
            if(p[i].weight<=p[j].weight) help[indx++]=p[i++];
            else help[indx++]=p[j++];
        }
        while(i<=m) help[indx++]=p[i++];
        while(j<=r) help[indx++]=p[j++];
        for(int k=l;k<=r;k++) p[k]=help[k];
    }

    void mysort(vector<path> &p,int l,int r){
        if(l>=r) return; 
        int m=l+(r-l)/2;
        mysort(p,l,m);
        mysort(p,m+1,r);
        merge(p,l,m,r);
    }

    int main(){
        int n,m;
        cin>>n>>m;
        DSU d(n);
        vector<path>pathes;
        int ans=0;
        int cnt=0;
        int index=0;
        int t=m;
        while(t--){
            int x,y,z;
            cin>>x>>y>>z;
            path p(x,y,z);
            if(!d.mp.count(x)) d.mp[x]=index++;
            if(!d.mp.count(y)) d.mp[y]=index++;
            pathes.push_back(p);
        }
        mysort(pathes,0,m-1);
        for(auto x:pathes){
            if(!d.same(x.u,x.v)){
                d.unite(x.u,x.v);
                ans+=x.weight;
                cnt++;
            }
        }
        if(cnt!=n-1){
            cout<<"不连通"<<endl;
        }else
        cout<<ans<<endl;
        return 0;
    }