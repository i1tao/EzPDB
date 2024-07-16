#pragma warning(disable : 4996)
#include <iostream>
#include "ez_pdb.h"


int main()
{
    auto pdb = ez_pdb(std::string(std::getenv("systemroot"))+ "\\System32\\ntoskrnl.exe");
	if (pdb.init())
	{
		auto rva_ntclose = pdb.get_rva("PspCidTable");
		printf("nt!PspCidTable = %llx\n", rva_ntclose);
	}
}

