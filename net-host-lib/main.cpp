#define NETHOST_USE_AS_STATIC

#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include "json.hpp"
#include "net-host-lib.h"

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "libnethost.lib")
using char_t = wchar_t;
#define STR(s) L##s
#else
#include <dlfcn.h>
#include <limits.h>
using char_t = char;
#define STR(s) s
#define MAX_PATH PATH_MAX
#endif
#include <thread>

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Function pointer type definition (in case it's not in hostfxr.h)
typedef int32_t(*custom_init_for_cmd_line_fn)(
    int32_t argc,
    const char_t** argv,
    const hostfxr_initialize_parameters* parameters,
    hostfxr_handle* host_context_handle);

// Global variables (for storing host runtime function pointers)
hostfxr_initialize_for_runtime_config_fn init_for_config_fptr;
custom_init_for_cmd_line_fn init_for_cmd_line_fptr;
hostfxr_get_runtime_delegate_fn get_delegate_fptr;
hostfxr_run_app_fn run_app_fptr;
hostfxr_close_fn close_fptr;

// C# Main method delegate types
using MainDelegate = void(CORECLR_DELEGATE_CALLTYPE*)(int argc, const char_t** argv);
using MainIntDelegate = int(CORECLR_DELEGATE_CALLTYPE*)();
using MainVoidDelegate = void(CORECLR_DELEGATE_CALLTYPE*)();

// Configuration structure
struct HostConfig {
#ifdef _WIN32
    wstring assemblyPath;
    wstring typeName;
    wstring methodName;
    vector<wstring> arguments;
#else
    string assemblyPath;
    string typeName;
    string methodName;
    vector<string> arguments;
#endif
};

// Read JSON configuration file
bool load_config(const fs::path& config_path, HostConfig& config)
{
    try
    {
        ifstream config_file(config_path);
        if (!config_file.is_open())
        {
            cerr << "Error: Cannot open net-host.json: " << config_path.string() << endl;
            return false;
        }

        json j;
        config_file >> j;

        // Read configuration from JSON
        string assemblyPath = j.value("assemblyPath", "");
        string typeName = j.value("typeName", "");
        string methodName = j.value("methodName", "Main");

        if (assemblyPath.empty() || typeName.empty())
        {
            cerr << "Error: assemblyPath and typeName are required" << endl;
            return false;
        }

#ifdef _WIN32
        // Convert string to wstring on Windows
        config.assemblyPath = wstring(assemblyPath.begin(), assemblyPath.end());
        config.typeName = wstring(typeName.begin(), typeName.end());
        config.methodName = wstring(methodName.begin(), methodName.end());

        // Read arguments
        if (j.contains("arguments") && j["arguments"].is_array())
        {
            for (const auto& arg : j["arguments"])
            {
                string arg_str = arg.get<string>();
                config.arguments.push_back(wstring(arg_str.begin(), arg_str.end()));
            }
        }

        cout << "Configuration loaded successfully:" << endl;
        wcout << L"  - Assembly: " << config.assemblyPath << endl;
        wcout << L"  - Type: " << config.typeName << endl;
        wcout << L"  - Method: " << config.methodName << endl;
        cout << "  - Arguments: " << config.arguments.size() << endl;
#else
        // Use string directly on POSIX
        config.assemblyPath = assemblyPath;
        config.typeName = typeName;
        config.methodName = methodName;

        // Read arguments
        if (j.contains("arguments") && j["arguments"].is_array())
        {
            for (const auto& arg : j["arguments"])
            {
                config.arguments.push_back(arg.get<string>());
            }
        }

        cout << "Configuration loaded successfully:" << endl;
        cout << "  - Assembly: " << config.assemblyPath << endl;
        cout << "  - Type: " << config.typeName << endl;
        cout << "  - Method: " << config.methodName << endl;
        cout << "  - Arguments: " << config.arguments.size() << endl;
#endif

        return true;
    }
    catch (const exception& ex)
    {
        cerr << "Error: JSON parsing failed: " << ex.what() << endl;
        return false;
    }
}

// ----------------------------------------------------------------------
// Load hostfxr and get function pointers
// ----------------------------------------------------------------------
bool load_hostfxr()
{
    get_hostfxr_parameters params = { sizeof(get_hostfxr_parameters), nullptr, nullptr };
    char_t buffer[MAX_PATH];
    size_t buffer_size = sizeof(buffer) / sizeof(char_t);
    int rc = get_hostfxr_path(buffer, &buffer_size, &params);

    if (rc != 0)
    {
        cerr << "Error: get_hostfxr_path failed with code " << rc << endl;
        return false;
    }

#ifdef _WIN32
    HMODULE lib = LoadLibraryW(buffer);
    if (!lib)
    {
        cerr << "Error: Failed to load hostfxr.dll" << endl;
        return false;
    }

    // Get function pointers
    init_for_config_fptr = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(lib, "hostfxr_initialize_for_runtime_config");
    init_for_cmd_line_fptr = (custom_init_for_cmd_line_fn)GetProcAddress(lib, "hostfxr_initialize_for_dotnet_command_line");
    get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GetProcAddress(lib, "hostfxr_get_runtime_delegate");
    run_app_fptr = (hostfxr_run_app_fn)GetProcAddress(lib, "hostfxr_run_app");
    close_fptr = (hostfxr_close_fn)GetProcAddress(lib, "hostfxr_close");
#else
    void* lib = dlopen(buffer, RTLD_LAZY | RTLD_LOCAL);
    if (!lib)
    {
        cerr << "Error: Failed to load hostfxr: " << dlerror() << endl;
        return false;
    }

    // Get function pointers
    init_for_config_fptr = (hostfxr_initialize_for_runtime_config_fn)dlsym(lib, "hostfxr_initialize_for_runtime_config");
    init_for_cmd_line_fptr = (custom_init_for_cmd_line_fn)dlsym(lib, "hostfxr_initialize_for_dotnet_command_line");
    get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)dlsym(lib, "hostfxr_get_runtime_delegate");
    run_app_fptr = (hostfxr_run_app_fn)dlsym(lib, "hostfxr_run_app");
    close_fptr = (hostfxr_close_fn)dlsym(lib, "hostfxr_close");
#endif

    return (init_for_cmd_line_fptr && get_delegate_fptr && run_app_fptr && close_fptr);
}

// ----------------------------------------------------------------------
// Main function
// ----------------------------------------------------------------------
int run_net_host()
{
    // Force flush console output buffers
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    cout << "=== .NET Generic Host ===" << endl;
    cout.flush();

    // Load configuration
#ifdef _WIN32
    fs::path config_path = fs::current_path() / L"net-host.json";
#else
    fs::path config_path = fs::current_path() / "net-host.json";
#endif
    HostConfig config;
    if (!load_config(config_path, config))
    {
        cerr << "Failed to load configuration" << endl;
        return -1;
    }

    // Load hostfxr
    if (!load_hostfxr())
    {
        cerr << "Failed to load hostfxr" << endl;
        return -1;
    }
    cout << "hostfxr loaded successfully" << endl;

    // Resolve assembly path (support relative paths)
    fs::path assembly_path = fs::current_path() / config.assemblyPath;

    if (!fs::exists(assembly_path))
    {
        cerr << "Error: Assembly not found: " << assembly_path.string() << endl;
        return -1;
    }
    cout << "Assembly path: " << assembly_path.string() << endl;

    // Initialize runtime (simulate "dotnet [DLL]" command)
    cout << "Initializing .NET runtime..." << endl;

    // Build command-line arguments: DLL path + user arguments
#ifdef _WIN32
    vector<wstring> args_vec = { assembly_path.wstring() };
#else
    vector<string> args_vec = { assembly_path.string() };
#endif
    args_vec.insert(args_vec.end(), config.arguments.begin(), config.arguments.end());

    vector<const char_t*> args_ptrs;
    for (const auto& arg : args_vec)
    {
        args_ptrs.push_back(arg.c_str());
    }

    hostfxr_handle cxt = nullptr;

    // Set initialization parameters
    hostfxr_initialize_parameters params;
    params.size = sizeof(hostfxr_initialize_parameters);
    params.host_path = assembly_path.c_str();
    params.dotnet_root = nullptr;

    int rc = init_for_cmd_line_fptr(
        static_cast<int>(args_ptrs.size()),
        args_ptrs.data(),
        &params,
        &cxt);

    if (rc != 0 || cxt == nullptr)
    {
        cerr << "Error: Runtime initialization failed with code: " << rc << endl;
        return -1;
    }

    cout << "Runtime initialized successfully" << endl;

#ifdef _WIN32
    // Initialize COM in STA mode (required for WPF on Windows)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
    {
        cerr << "Error: COM initialization failed. HRESULT: " << hr << endl;
        close_fptr(cxt);
        return -1;
    }
#endif

    cout << "Running .NET application..." << endl;

    // Call run_app - automatically finds and executes Main method
    rc = run_app_fptr(cxt);

#ifdef _WIN32
    // Cleanup COM
    CoUninitialize();
#endif

    close_fptr(cxt);

    if (rc != 0)
    {
        cerr << "Error: Application execution failed with code: " << rc << endl;
        return -1;
    }

    cout << "Application completed successfully" << endl;
    return 0;
}

// ----------------------------------------------------------------------
// Helper function for running the .NET host in a new thread
// ----------------------------------------------------------------------
int run_net_host_in_background()
{
    try
    {
        // Create and launch a new thread to run the `run_net_host` function
        std::thread host_thread([]() {
            // Call the original run_net_host function inside this thread
            int result = run_net_host();
            if (result != 0)
            {
                std::cerr << "Error occurred in .NET Host thread execution" << std::endl;
            }
            });

        // Detach the thread so that it can run independently
        host_thread.detach();

        // Return a success code, as the host is running in the background
        std::cout << "Running .NET Host in a new thread..." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception occurred while creating the thread: " << e.what() << std::endl;
        return -1;
    }
}