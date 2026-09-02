#include<bits/stdc++.h>
using namespace std;

using ll = long long;
/*
关键处理:    ans[l-1] += s;
            ans[l] += d-s;
            ans[r] -= e+d;
            ans[r+1] += e;
*/

void solve() {
    int n,m;
    cin >> n >> m;

    vector<ll> ans(n+5,0);

    while(m--) {
        int l,r;
        ll s,e;
        cin >> l >> r >> s >> e;

        if(l == r) {
            ans[l-1] += s;
            ans[r] -= s;
            continue;
        }

        ll d = (e-s)/(r-l);

        ans[l-1] += s;
        ans[l] += d-s;
        ans[r] -= e+d;
        ans[r+1] += e;
    }

    for(int i=1;i<n;i++) {
        ans[i] += ans[i-1];
    }

    for(int i=1;i<n;i++) {
        ans[i] += ans[i-1];
    }

    ll mx = ans[0];
    ll xo = 0;

    for(int i=0;i<n;i++) {
        mx = max(mx, ans[i]);
        xo ^= ans[i];
    }

    cout << xo << " " << mx << '\n';
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    solve();

    return 0;
}
