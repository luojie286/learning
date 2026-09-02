#include <iostream>

using namespace std;

const int MAXN = 1000001;

int arr[MAXN];

// 单调栈
int stk[MAXN];
int top = 0;

// 相同值链表
int nextSame[MAXN];

// 答案
int ans[MAXN][2];

int n;

int main() {

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> n;

    for (int i = 0; i < n; i++) {
        cin >> arr[i];
        nextSame[i] = -1;
    }

    for (int i = 0; i < n; i++) {

        // 当前值更小
        while (top > 0 &&
               arr[stk[top - 1]] > arr[i]) {

            int cur = stk[--top];

            // 整组一起弹
            while (nextSame[cur] != -1) {

                int t = nextSame[cur];

                ans[cur][0] =
                    (top > 0 ? stk[top - 1] : -1);

                ans[cur][1] = i;

                cur = t;
            }

            ans[cur][0] =
                (top > 0 ? stk[top - 1] : -1);

            ans[cur][1] = i;
        }

        // 相等元素
        if (top > 0 &&
            arr[stk[top - 1]] == arr[i]) {

            nextSame[i] = stk[top - 1];
            stk[top - 1] = i;
        }
        else {
            stk[top++] = i;
        }
    }

    // 清算
    while (top > 0) {

        int cur = stk[--top];

        while (nextSame[cur] != -1) {

            int t = nextSame[cur];

            ans[cur][0] =
                (top > 0 ? stk[top - 1] : -1);

            ans[cur][1] = -1;

            cur = t;
        }

        ans[cur][0] =
            (top > 0 ? stk[top - 1] : -1);

        ans[cur][1] = -1;
    }

    for (int i = 0; i < n; i++) {
        cout << ans[i][0]
             << " "
             << ans[i][1]
             << '\n';
    }

    return 0;
}