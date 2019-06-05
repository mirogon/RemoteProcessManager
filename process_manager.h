#pragma once
//Core
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

//Windows
#include <windows.h>
#include <TCHAR.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <TlHelp32.h>

namespace m1
{
	struct process
	{
		std::string name;
		unsigned long process_id;
	};

	class process_manager
	{
		//Methods
	public:

		process_manager();
		~process_manager();

		//Get the process information from the PSAPI
		void init_processes_PSAPI();
		//Get the process information from the Tool Help Library
		void init_processes_THL();

		void kill_process(unsigned long pid);

		std::vector<m1::process> get_processes();

	private:

		std::string get_process_name_PSAPI(unsigned long process_id);

		//Member
	private:

		std::vector<process> m_processes;
	};
}