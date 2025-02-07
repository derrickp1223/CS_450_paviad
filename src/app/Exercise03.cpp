#include <iostream>
#include <string>
using namespace std;

int main(int argc, char** argv) {

    if(argc >= 2) {
        string filename = string(argv[1]);
        cout << "FILENAME: " << filename << endl;
    }
    
    cout << "BEGIN THE EXERCISE!" << endl;

    return 0;
}