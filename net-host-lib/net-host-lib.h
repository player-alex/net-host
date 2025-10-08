#pragma once

// Declaration of the main function from the library
#ifdef _WIN32
extern "C" __declspec(dllexport) int run_net_host();
extern "C" __declspec(dllexport) int run_net_host_in_background();
#else
extern "C" __attribute__((visibility("default"))) int run_net_host();
extern "C" __attribute__((visibility("default"))) int run_net_host_in_background();
#endif
