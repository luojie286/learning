#include<iostream>
#include<algorithm>
#include<vector>
using namespace std;
using ll=long long;
const int N=1e5;
const int base=10;

ll help[N];
ll a[N];

int main() {
    int n;
    cin>>n;
    for(int i=0;i<n;i++) cin>>a[i];
    ll ma=*max_element(a,a+n);
    int offset=1;
    int bits=0;
    while(ma){
        ma/=10;
        bits++;
    }
    while(bits--){
        vector<int>cnt(base,0);
        for(int i=0;i<n;i++){
            cnt[a[i]/offset%base]++;
        }
        for(int i=1;i<base;i++){
            cnt[i]+=cnt[i-1];//计算前缀和，目的是为了能够确定数组应重排的区间
        }
        for(int i=n-1;i>=0;i--){
            help[--cnt[a[i]/offset%base]]=a[i];//从后往前，相同数位(因为cnt是从最后下标开始的)保证相对次序，提高排序的稳定性
        }
        for(int i=0;i<n;i++){
            a[i]=help[i];
        }
        offset*=10;
    }
    for(int i=0;i<n;i++) cout<<a[i]<<" ";
    return 0;
}