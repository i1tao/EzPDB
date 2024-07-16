# EzPDB

A simple C++ library for getting the symbol's rva from the PDB file. Applicable to C++ 14 - C++20 standards.

# Usage

add ez_pdb.h and ez_pdb.cpp to your project. 

```cpp
auto pdb = ez_pdb(std::string(std::getenv("systemroot"))+ "\\System32\\ntoskrnl.exe");
if (pdb.init())
{
    auto rva_ntclose = pdb.get_rva("PspCidTable");
    printf("nt!PspCidTable = %llx\n", rva_ntclose);
}
```

---
