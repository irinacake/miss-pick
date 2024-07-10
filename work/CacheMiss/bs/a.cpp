#include <iostream>
using namespace std;

int main() {

    int swapTables4[16] = {0,1,2,3,
                            1,0,2,3,
                            2,3,0,1,
                            3,2,1,0};

    cout << swapTables4[3] << endl;

    int* reftotable;
    reftotable = swapTables4;

    cout << reftotable[1] << endl;
    
    return 0;
}