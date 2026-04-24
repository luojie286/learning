#include <bits/stdc++.h>
using namespace std;
using ll = long long;
using pill = pair<int, int>;

// Prim 算法，返回最小生成树总权值，不连通时返回 -1
ll prim(int n, const vector<vector<pill>>& graph) {
    vector<int> dis(n + 1, INT_MAX);   // 记录连接到 MST 的最小边权
    vector<bool> visit(n + 1, false);
    // 优先队列存放 (边权, 节点编号)
    priority_queue<pill, vector<pill>, greater<pill>> pq;

    dis[1] = 0;
    pq.push({0, 1});    // (距离, 节点)

    ll ans = 0;
    int cnt = 0;        // 已加入 MST 的节点数

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (visit[u]) continue;
        visit[u] = true;
        cnt++;
        ans += d;

        // 遍历邻边
        for (auto [v, w] : graph[u]) {
            if (!visit[v] && w < dis[v]) {
                dis[v] = w;
                pq.push({dis[v], v});
            }
        }
    }

    return (cnt == n) ? ans : -1;
}

void solve() {
    int n, m;
    cin >> n >> m;
    vector<vector<pill>> graph(n + 1);

    for (int i = 0; i < m; i++) {
        int x, y, z;
        cin >> x >> y >> z;
        graph[x].push_back({y, z});
        graph[y].push_back({x, z});
    }

    ll ans = prim(n, graph);
    if(ans==-1){
        cout << "orz" <<"\n";
        return ;
    }
    cout << ans << "\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    solve();
    return 0;
}