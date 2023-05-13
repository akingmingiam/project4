#include <iostream>
#include <cstring>
#include "datablob.hpp"
using namespace std;

int main(){
    int *ma=new int[4]{1,2,3,4};
    // DataBlob a=DataBlob(2,2,1,INT,ma);
    // DataBlob b=a;
    // DataBlob c=a*b;
    // // cout<<a.at<int>(1,1,1) <<endl;
    // // cout<<b.at<int>(1,1,1) <<endl;
    // cout<<c.at<int>(0,0,0)<<endl;
    cout<<ma[0]<<endl;

}
