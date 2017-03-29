#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <d3d12.h>
#include <tchar.h>
#include <pdh.h>

static SYSTEM_INFO sysInfo;
static FILETIME fileTime;
static MEMORYSTATUSEX globalMemoryInfo;
static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;
static ULARGE_INTEGER now;

/*
void initPdhStuff() 
{
	PdhOpenQuery(NULL, NULL, &cpuQuery);
	PdhAddCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
	PdhCollectQueryData(cpuQuery);
}

double getPdhStuffCurrentValue() 
{
	PDH_FMT_COUNTERVALUE counterVal;
	PdhCollectQueryData(cpuQuery);
	PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
	return counterVal.doubleValue;
}
*/

void GetAndPrintSystemTimeAsFileTime()
{
	GetSystemTimeAsFileTime(&fileTime);
	printf(
		"\n\
		\tSystem Time as File Time:\n \
		\tLow: %ld\n \
		\tHigh: %ld\n \
		\n", 
		fileTime.dwLowDateTime, 
		fileTime.dwHighDateTime);
}

void ProcessTimes(HANDLE hProcess)
{
	FILETIME fsys, fuser;
	ULARGE_INTEGER sys, user;
	GetSystemTimeAsFileTime(&fileTime);
	GetProcessTimes(hProcess, &fileTime, &fileTime, &fsys, &fuser);
	memcpy(&now, &fileTime, sizeof(FILETIME));
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
}

void ProcessInfo(DWORD processID)
{
	HANDLE hProcess;
	PROCESS_MEMORY_COUNTERS pmc;

	// Print the process identifier.
	printf("\t\t\tProcess ID: %u\n", processID);

	// Print information about the process.
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (NULL == hProcess) {
		printf("\t\t\tFailed to open handle to %u\n", processID);
		return;
	}

	ProcessTimes(hProcess);

	TCHAR path[MAX_PATH];
	if (GetModuleFileNameEx(hProcess, 0, path, MAX_PATH))
	{
		printf("\tProcess Path: %s\n", path);
		// At this point, buffer contains the full path to the executable
	}

	if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
	{
		printf("\tPageFaultCount:            \t0x%08X\t(%d)\n", pmc.PageFaultCount, pmc.PageFaultCount);
		printf("\tPeakWorkingSetSize:        \t0x%08X\t(%d)\n", pmc.PeakWorkingSetSize, pmc.PeakWorkingSetSize);
		printf("\tWorkingSetSize:            \t0x%08X\t(%d)\n", pmc.WorkingSetSize, pmc.WorkingSetSize);
		printf("\tQuotaPeakPagedPoolUsage:   \t0x%08X\t(%d)\n",
			pmc.QuotaPeakPagedPoolUsage, pmc.QuotaPeakPagedPoolUsage);
		printf("\tQuotaPagedPoolUsage:       \t0x%08X\t(%d)\n",
			pmc.QuotaPagedPoolUsage, pmc.QuotaPagedPoolUsage);
		printf("\tQuotaPeakNonPagedPoolUsage:\t0x%08X\t(%d)\n",
			pmc.QuotaPeakNonPagedPoolUsage, pmc.QuotaPeakNonPagedPoolUsage);
		printf("\tQuotaNonPagedPoolUsage:    \t0x%08X\t(%d)\n",
			pmc.QuotaNonPagedPoolUsage, pmc.QuotaNonPagedPoolUsage);
		printf("\tPagefileUsage:             \t0x%08X\t(%d)\n", 
			pmc.PagefileUsage, pmc.PagefileUsage);
		printf("\tPeakPagefileUsage:         \t0x%08X\t(%d)\n",
			pmc.PeakPagefileUsage, pmc.PeakPagefileUsage);
	} else {
		printf("\tFailed to get process memory info for %u\n", processID);
	}

	CloseHandle(hProcess);
}

void GetAndPrintSystemInfo() 
{
	GetSystemInfo(&sysInfo);
	printf(
		"\n \
		\tSystem Info:\n \
		\tOEM ID: %ld\n \
		\tProcessor Architecture: %d\n \
		\tPage Size: %ld\n \
		\tMinimum Application Address: 0x%016llx\n \
		\tMaximum Application Address: 0x%016llx\n \
		\tActive Processor Mask: %ld\n \
		\tNumber of Processors: %ld\n \
		\tAllocation Granularity: %ld\n \
		\tProcessor Type: %ld\n \
		\tProcessor Level: %d\n \
		\tProcessor Revision: %d\n \
		\n",
		sysInfo.dwOemId,
		sysInfo.wProcessorArchitecture,
		sysInfo.dwPageSize,
		sysInfo.lpMinimumApplicationAddress,
		sysInfo.lpMaximumApplicationAddress,
		sysInfo.dwActiveProcessorMask,
		sysInfo.dwNumberOfProcessors,
		sysInfo.dwAllocationGranularity,
		sysInfo.dwProcessorType,
		sysInfo.wProcessorLevel,
		sysInfo.wProcessorRevision
	);
}

void GetAndPrintGlobalMemoryInfo()
{
	globalMemoryInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&globalMemoryInfo);
	printf(
		"\n \
		\tGlobal Memory Info:\n \
		\tLength: %ld\n \
		\tMemory Load: %ld\n \
		\tTotal Physical: %llu\n \
		\tAvailable Physical: %llu\n \
		\tTotal Page File: %llu\n \
		\tAvailable Page File: %llu\n \
		\tTotal Virtual: %llu\n \
		\tAvailable Virtual: %llu\n \
		\tAvailable Extended Virtual: %llu\n \
		\n",
		globalMemoryInfo.dwLength,
		globalMemoryInfo.dwMemoryLoad,
		globalMemoryInfo.ullTotalPhys,
		globalMemoryInfo.ullAvailPhys,
		globalMemoryInfo.ullTotalPageFile,
		globalMemoryInfo.ullAvailPageFile,
		globalMemoryInfo.ullTotalVirtual,
		globalMemoryInfo.ullAvailVirtual,
		globalMemoryInfo.ullAvailExtendedVirtual
	);
}

int main(void)
{	
	// Get the System Time as File Time
	GetAndPrintSystemTimeAsFileTime();

	// Print System Info	
	GetAndPrintSystemInfo();

	// Print Global Memory Info
	GetAndPrintGlobalMemoryInfo();

	//	initPdhStuff();

	printf("\n\t\t\tCollecting Process Info\n\n");

	DWORD aProcesses[1024], cbNeeded, cProcesses;

	// Get the list of process identifiers
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
		return 1; // error result
	}

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	for (int i = 0; i < cProcesses; i++) {
		printf("\n\t\t\tInfo for the %dth process\n", i);		
		ProcessInfo(aProcesses[i]);
	}

	return 0;
}


