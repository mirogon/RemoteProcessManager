#include "process_manager.h"

namespace m1
{

	process_manager::process_manager()
	{
		CoInitialize(NULL);
	}

	process_manager::~process_manager()
	{
		CoUninitialize();
	}

	std::string process_manager::get_process_name_PSAPI(unsigned long process_id)
	{
		char process_name[MAX_PATH] = TEXT("<unknown>");

		// Get a void* to the process.
		void* p_process = OpenProcess(PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, process_id);

		// Get the process name.
		if (NULL != p_process)
		{
			HMODULE p_module;
			unsigned long bytes_needed;

			if (EnumProcessModules(p_process, &p_module, sizeof(p_module), &bytes_needed))
			{
				GetModuleBaseName(p_process, p_module, process_name, sizeof(process_name) / sizeof(char));
			}
		}

		// Release the void* from the process
		CloseHandle(p_process);
		
		std::string rs = process_name;
		return rs;
	}

	void process_manager::init_processes_PSAPI()
	{

		m_processes.clear();
		m_processes.shrink_to_fit();

		// Get the list of process identifiers
		unsigned long processes[1024], bytes_needed, num_processes;

		if (!EnumProcesses(processes, sizeof(processes), &bytes_needed))
		{
			//std::cout << "Processes could not be enumerated!" << std::endl;
			return;
		}

		//Calculate how many process identifiers were returned
		num_processes = bytes_needed / sizeof(unsigned long);

		for (int i = 0; i < num_processes; i++)
		{
			if (processes[i] != 0)
			{
				m_processes.push_back(m1::process());
				m_processes.back().process_id = processes[i];
				m_processes.back().name = get_process_name_PSAPI(processes[i]);
			}
		}
	}

	void process_manager::init_processes_THL()
	{
		m_processes.clear();
		m_processes.shrink_to_fit();

		HANDLE process_snap;
		PROCESSENTRY32 process_entry;
		process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (process_snap != INVALID_HANDLE_VALUE)
		{
			process_entry.dwSize = sizeof(PROCESSENTRY32);
			if (Process32First(process_snap, &process_entry))
			{
				m_processes.push_back(m1::process{ process_entry.szExeFile , process_entry.th32ProcessID });
				while (Process32Next(process_snap, &process_entry))
				{
					m_processes.push_back(m1::process{ process_entry.szExeFile , process_entry.th32ProcessID });
				}
				CloseHandle(process_snap);
			}
		}
		else
		{
			//std::cout << "Process snapshot could not be made successfully" << std::endl;
			return;
		}
	}

	void process_manager::kill_process(unsigned long pid)
	{
		HANDLE process_to_kill = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, pid);
		TerminateProcess(process_to_kill, 0);
	}

	std::vector<m1::process> process_manager::get_processes()
	{
		return m_processes;
	}
}