#include "ez_pdb.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <iomanip>
#include <sstream>
#include <winhttp.h>
#include <codecvt>
#include <wininet.h>
#include <iostream>
#include <fstream>
#include <direct.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "dbghelp.lib")

bool ez_pdb::init()
{
    SYMSRV_INDEX_INFO info = {};

    info.sizeofstruct = sizeof(SYMSRV_INDEX_INFO);
    if (!SymSrvGetFileIndexInfo(target_path_.c_str(), &info, 0))
    {

        printf("error : SymSrvGetFileIndexInfo:[%d]", GetLastError());
        return FALSE;
    }

    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setfill('0')
        << std::setw(8) << info.guid.Data1
        << std::setw(4) << info.guid.Data2
        << std::setw(4) << info.guid.Data3
        << std::setw(2) << static_cast<int>(info.guid.Data4[0])
        << std::setw(2) << static_cast<int>(info.guid.Data4[1])
        << std::setw(2) << static_cast<int>(info.guid.Data4[2])
        << std::setw(2) << static_cast<int>(info.guid.Data4[3])
        << std::setw(2) << static_cast<int>(info.guid.Data4[4])
        << std::setw(2) << static_cast<int>(info.guid.Data4[5])
        << std::setw(2) << static_cast<int>(info.guid.Data4[6])
        << std::setw(2) << static_cast<int>(info.guid.Data4[7])
        << std::setw(1) << info.age;


    std::string file_guid = ss.str();
    std::string download_url = symbol_server_ + "/" + info.pdbfile + "/" + file_guid + "/" + info.pdbfile;
    std::string pdb_store_path = pdb_path_ + "/" + info.pdbfile + "/" + file_guid + ".pdb";


    if (!download_url_to_file(download_url, pdb_store_path))
    {
        return false;
    }

    pdb_path_ = pdb_store_path;

    std::ifstream written_file(pdb_store_path, std::ios::binary | std::ios::ate);
    if (!written_file.good())
    {
        std::cerr << "Failed to get the size of the downloaded file: " << pdb_store_path << std::endl;
        return false;
    }

    auto written_file_size = written_file.tellg();
    written_file.close();
    pdb_size_ = written_file_size;

    auto pdb_file_handle = CreateFileA(
        pdb_store_path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        NULL,
        nullptr);
    if (pdb_file_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    HANDLE process_handle = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION,
        false,
        GetCurrentProcessId()
    );
    if (!process_handle)
    {
        CloseHandle(pdb_file_handle);
        return false;
    }

    if (!SymInitialize(process_handle, pdb_store_path.c_str(), false))
    {
        CloseHandle(process_handle);
        CloseHandle(pdb_file_handle);
        return false;
    }

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_AUTO_PUBLICS | SYMOPT_DEBUG | SYMOPT_LOAD_ANYTHING);

    auto symbol_table = SymLoadModuleEx(
        process_handle, 
        nullptr,
        pdb_store_path.c_str(),
        nullptr,
        0x100000000ULL,
        static_cast<DWORD>(pdb_size_), 
        nullptr, 0);
    if (!symbol_table)
    {
        SymCleanup(process_handle);
        CloseHandle(process_handle);
        CloseHandle(pdb_file_handle);
        return false;
    }

    pdb_file_handle_ = pdb_file_handle;
    process_handle_ = process_handle;
    return true;
}

unsigned long long ez_pdb::get_rva(std::string sym_name)
{
    SYMBOL_INFO si = { 0 };
    si.SizeOfStruct = sizeof(SYMBOL_INFO);
    if (!SymFromName(this->process_handle_, sym_name.c_str(), &si))
    {
        return -1;
    }
    return si.Address - si.ModBase;
}

bool ez_pdb::create_directories(const std::string& path)
{

    size_t pos = 0;
    std::string dir;
    int ret = 0;

    while ((pos = path.find_first_of("/", pos)) != std::string::npos)
    {
        dir = path.substr(0, pos++);
        if (dir.size() == 0) continue;
        if (_mkdir(dir.c_str()) && errno != EEXIST) {
            std::cerr << "Failed to create directory: " << dir << std::endl;
            return false;
        }
    }
    return true;
}

bool ez_pdb::download_url_to_file(const std::string& url, const std::string& file_path)
{
    std::ifstream file(file_path);
    if (file.good())
    {
        std::cerr << "File already exists: " << file_path << std::endl;
        return true;
    }

    if (!create_directories(file_path)) {
        std::cerr << "Failed to create directories for: " << file_path << std::endl;
        return false;
    }

    HINTERNET internet_handle = InternetOpenA("ez_pdb_by_i1tao", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!internet_handle) {
        std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
        return false;
    }

    HINTERNET open_url_handle = InternetOpenUrlA(internet_handle, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!open_url_handle) {
        std::cerr << "InternetOpenUrl failed: " << GetLastError() << std::endl;
        InternetCloseHandle(internet_handle);
        return false;
    }

    std::ofstream output_file(file_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        InternetCloseHandle(open_url_handle);
        InternetCloseHandle(internet_handle);
        return false;
    }

    char buffer[4096];
    DWORD bytes_read;
    while (InternetReadFile(open_url_handle, buffer, sizeof(buffer), &bytes_read) && bytes_read != 0)
    {
        output_file.write(buffer, bytes_read);
    }

    output_file.close();
    InternetCloseHandle(open_url_handle);
    InternetCloseHandle(internet_handle);

    return true;
}
