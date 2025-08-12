#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

int main()
{
    srand(time(0));

    vector<int> testVec = {1, 2, 3};

    for (int i = 0; i < testVec.size(); i++)
    {
        cout << testVec.at(i) << endl;
    }

    return 0;
}