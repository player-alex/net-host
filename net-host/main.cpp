#include <iostream>
#include "net-host-lib.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#pragma comment(lib, "net-host-lib.lib")

using namespace std;

// Entry point
#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    cout << "=== Test Application ===" << endl;
    cout << "Calling library main function..." << endl;
    cout << endl;

    // Call the library's main function
    int result = run_net_host();

    cout << endl;
    cout << "Library main function returned: " << result << endl;

    return result;
}
