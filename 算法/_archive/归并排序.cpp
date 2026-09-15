#include<iostream>
#include<algorithm>
using namespace std;
const int N=1e5;
int a[N];
int help[N];

void merge(int a[],int l,int m,int r){
    int i=l,j=m+1;
    int index=0;
    while(i<=m&&j<=r){
        if(a[i]<=a[j]){
            help[index+l]=a[i];
            i++;
            index++;
        }else{
            help[index+l]=a[j];
            j++;
            index++;
        }
    }
    while(i<=m){
        help[index+l]=a[i];
        i++;
        index++;
    }
    while(j<=r){
        help[index+l]=a[j];
        j++;
        index++;
    }
    for(int i=l;i<=r;i++){
        a[i]=help[i];
    }
}

void mergesort(int a[],int l,int r){
    if(l==r) return;
    int mid=l+(r-l)/2;
    mergesort(a,l,mid);
    mergesort(a,mid+1,r);
    merge(a,l,mid,r);
}

int main(){
    int n;
    cin>>n;
    for(int i=0;i<n;i++) cin>>a[i];
    mergesort(a,0,n-1);
    for(int i=0;i<n;i++){
        cout<<a[i]<<" ";
    }
    return 0;
}