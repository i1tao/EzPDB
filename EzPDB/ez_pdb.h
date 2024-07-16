#pragma once

#include <string>
#include <vector>
#include <Windows.h>


class ez_pdb
{
public:
    ez_pdb(std::string target, std::string server = "http://msdl.microsoft.com/download/symbols", std::string store_path = "./symbol")
        :target_path_(std::move(target)), symbol_server_(std::move(server)), pdb_path_(std::move(store_path))
    {}

    bool init();
    unsigned long long get_rva(std::string sym_name);
private:
    bool create_directories(const std::string& path);
    bool download_url_to_file(const std::string& url, const std::string& file_path);

    std::string target_path_;
    std::string symbol_server_;
    std::string pdb_path_;
    std::uint64_t pdb_size_;

    HANDLE pdb_file_handle_;
    HANDLE process_handle_;
};
