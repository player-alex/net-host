<div align="center">

# 🚀 .NET Generic Host

![Platform](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows&logoColor=white)
![.NET](https://img.shields.io/badge/.NET-8.0-512BD4?logo=.net&logoColor=white)
![C++](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

**A lightweight native C++ host for running .NET assemblies without modification. Perfect for hosting WPF applications, console apps, or any .NET DLL with a `Main` method.** 🎯

</div>

---

## ✨ Features

- 🔄 **Zero Assembly Modification** - Run any .NET DLL without changing source code
- 🎨 **WPF Support** - Full support for WPF applications with proper resource loading
- 🧵 **STA Threading** - Automatic COM initialization for UI applications
- ⚙️ **JSON Configuration** - Simple configuration via `net-host.json`
- 📦 **Entry Assembly Support** - Properly sets entry assembly for resource loading
- 💬 **Command-line Arguments** - Pass arguments to the hosted application

## 🎯 Use Cases

- 🖥️ Host WPF applications from native C++ code
- 📚 Run .NET DLLs as standalone applications
- 🚀 Create custom .NET application launchers
- 🔗 Bridge native and managed code seamlessly

## 🛠️ Building

### Prerequisites

- 🔧 **Visual Studio 2022** or later
- 🔷 **.NET 8.0 SDK** - [Download](https://dotnet.microsoft.com/download/dotnet/8.0)
- ⚡ **C++20 support**

### Build Steps

1. Open `net-host.sln` in Visual Studio
2. Build the solution (Release/x64 recommended)
3. Output: `net-host.exe` in `x64/Release` or `x64/Debug`

## 📖 Usage

### 1️⃣ Create Configuration File

Create `net-host.json` in the same directory as `net-host.exe`:

```json
{
  "assemblyPath": "YourApp.dll",
  "typeName": "YourNamespace.Program",
  "methodName": "Main",
  "arguments": []
}
```

### 2️⃣ Prepare Your .NET Assembly

Ensure your .NET application has a `public static void Main()` or `public static void Main(string[] args)` method:

```csharp
namespace YourNamespace
{
    public class Program
    {
        public static void Main(string[] args)
        {
            // Your application code
            var app = new App();
            app.Run();
        }
    }
}
```

### 3️⃣ Deploy Files

Place in the same directory:
- `net-host.exe`
- `net-host.json`
- `nethost.dll` (from .NET SDK)
- `YourApp.dll` and all its dependencies
- `YourApp.runtimeconfig.json`

### 4️⃣ Run

```bash
net-host.exe
```

## ⚙️ Configuration Reference

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `assemblyPath` | string | ✅ Yes | Path to the .NET DLL (relative or absolute) |
| `typeName` | string | ✅ Yes | Fully qualified type name (e.g., `Namespace.Class`) |
| `methodName` | string | ❌ No | Entry method name (default: `Main`) |
| `arguments` | array | ❌ No | Command-line arguments to pass to Main |

### Example with Arguments

```json
{
  "assemblyPath": "MyApp.dll",
  "typeName": "MyApp.Program",
  "methodName": "Main",
  "arguments": [
    "--config", "production.json",
    "--verbose"
  ]
}
```

## 🏗️ How It Works

1. **Configuration Loading** - Reads `net-host.json` to get assembly information
2. **hostfxr Initialization** - Loads .NET hosting library (`hostfxr.dll`)
3. **Runtime Initialization** - Initializes .NET runtime using `hostfxr_initialize_for_dotnet_command_line`
4. **COM/STA Setup** - Initializes COM in STA mode for WPF support
5. **Application Execution** - Calls `hostfxr_run_app` to execute the Main method
6. **Entry Assembly Setup** - Automatically sets entry assembly for resource loading

## 🎨 WPF Applications

For WPF applications, the host automatically:
- ✅ Initializes COM in STA (Single-Threaded Apartment) mode
- ✅ Sets up entry assembly for `pack://` URI resource loading
- ✅ Handles `Application.ResourceAssembly` resolution
- ✅ Supports XAML resource loading

No modifications needed to your WPF application!

## 🐛 Troubleshooting

<details>
<summary><b>❌ Assembly Not Found</b></summary>

- ✅ Ensure the DLL path in `net-host.json` is correct
- ✅ Check that all dependency DLLs are in the same directory
- ✅ Verify the assembly path is relative to `net-host.exe` location

</details>

<details>
<summary><b>⚠️ Runtime Initialization Failed</b></summary>

- ✅ Verify .NET 8.0 Runtime is installed
- ✅ Check that `runtimeconfig.json` exists next to the DLL
- ✅ Ensure `rollForward` policy in `runtimeconfig.json` allows runtime version
- ✅ Verify `nethost.dll` is present in the same directory

</details>

<details>
<summary><b>🎨 WPF Resource Loading Issues</b></summary>

- ✅ Verify all resource DLLs are present
- ✅ Check that XAML resources are embedded correctly
- ✅ Ensure `Build Action` for resources is set to `Resource`
- ✅ Verify pack URIs are correct in your XAML files

</details>

<details>
<summary><b>🔧 COM Initialization Failed</b></summary>

- ✅ Close other applications that may conflict
- ✅ Run as administrator if necessary
- ✅ Check if COM is already initialized in a different mode

</details>

## 🔧 Technical Details

### Architecture

```
net-host.exe (Native C++)
    ↓ hostfxr API
.NET Runtime
    ↓ load_assembly_and_get_function_pointer
YourApp.dll
    ↓ Main() execution
WPF Application / Console App
```

### Key APIs Used

| API | Purpose |
|-----|---------|
| `hostfxr_initialize_for_dotnet_command_line` | Simulates `dotnet YourApp.dll` |
| `hostfxr_run_app` | Executes the application's Main method |
| `CoInitializeEx` | Initializes COM for UI threading |

## 📦 Example Projects

### Console Application

```json
{
  "assemblyPath": "ConsoleApp.dll",
  "typeName": "ConsoleApp.Program",
  "methodName": "Main",
  "arguments": ["--help"]
}
```

### WPF Application

```json
{
  "assemblyPath": "WpfApp.dll",
  "typeName": "WpfApp.App",
  "methodName": "Main",
  "arguments": []
}
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [.NET Hosting APIs](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting) - Built with .NET Hosting APIs
- [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++
- The .NET community for excellent documentation and support

## 📚 Additional Resources

- [hostfxr API Reference](https://github.com/dotnet/runtime/blob/main/src/native/corehost/hostfxr.h)
- [Write a custom .NET runtime host](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)

---
