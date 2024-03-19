#pragma once

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4239) // A non-const reference may only be bound to an lvalue; assignment operator takes a reference to non-const
#pragma warning(disable:4324) // structure was padded due to __declspec(align())

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
//#define NODRAWTEXT
//#define NOGDI
//#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include <dxgidebug.h>

#include <algorithm>
#include <iostream>


#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler.lib")

#pragma warning(disable:5082)

#include <wrl/client.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint32_t uint;

#define Safe_Release(x) if(x) { x->Release(); x = nullptr;}

template<typename T>
using comptr = Microsoft::WRL::ComPtr<T>;

#if defined(DEBUG) | defined(_DEBUG)
#define HR(x) __hr(x, __FILE__, __LINE__)
#else
#define HR(x) (x)
#endif

#define Log_Error(x,...) __error(x, __FILE__ , __LINE__, __VA_ARGS__)

inline void __hr(HRESULT hr, LPCSTR filename, int line)
{
    if (FAILED(hr)) {
        char* buffer = {};
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&buffer, 0, NULL
        );
        LocalFree(buffer);
        TCHAR totalCuffer[512];
        filename = (::strrchr(filename, '\\') + 1);
        sprintf_s(totalCuffer, "%s\n[File : %s]\n[Line : %d]\n\n", buffer, filename, line);

        if (MessageBoxA(NULL, totalCuffer, "HRESULT Error", MB_OK)) {
            ExitProcess(0);
        }
    }
}

inline void __error(LPCSTR code, LPCSTR filename, int line, ...)
{
    va_list args;
    va_start(args, code);
    TCHAR codeBuffer[256]{};

    vsnprintf_s(codeBuffer, -1, code, args);

    filename = (::strrchr(filename, '\\') + 1);

    TCHAR totalBuffer[256]{};

    sprintf_s(totalBuffer, "file : %s\nline : %d\n\n%s\n", filename, line, codeBuffer);
    va_end(args);

    int res = MessageBoxA(NULL, totalBuffer, "Error", MB_OK | MB_ICONWARNING);
    if (res == IDOK)
    {
        ExitProcess(0);
    }
};

template<typename... Args>
inline void SetNameToUTF16(ID3D12Object* pObj, const char* code, Args&&... args)
{
    char buffer[128];
    sprintf_s(buffer, code, args...);
    std::string name = buffer;
    std::wstring nameUTF16(name.begin(), name.end());
    pObj->SetName(nameUTF16.c_str());
}

inline LPCSTR UTF16ToUTF8(const wchar_t* wstr)
{
    static std::string code;
    code.clear();
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1,
             nullptr, 0,nullptr, nullptr);
    code.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, 
        &code[0], len, nullptr, nullptr);
    return (LPCSTR)code.c_str();
};

#define LOG std::cout
#define ENDL std::endl