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
		"\tSystem As File Time:\t\tLow: %lu\t\t\tHigh: %lu\n", 
		fileTime.dwLowDateTime, 
		fileTime.dwHighDateTime);
}

void ProcessTimes(HANDLE hProcess)
{
	FILETIME creationTime, exitTime, kernelTime, userTime;
	GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime);
	printf(
		"\t\t\tCreation Time\t\tLow: %lu\t\t\tHigh: %lu\n \
		\tExit Time\t\tLow: %lu\t\t\tHigh: %lu\n \
		\tKernel Time\t\tLow: %lu\t\t\tHigh: %lu\n \
		\tUser Time\t\tLow: %lu\t\t\tHigh: %lu\n \
		\n",
		creationTime.dwLowDateTime, 
		creationTime.dwHighDateTime,
		exitTime.dwLowDateTime,
		exitTime.dwHighDateTime,
		kernelTime.dwLowDateTime,
		kernelTime.dwHighDateTime,
		userTime.dwLowDateTime,
		userTime.dwHighDateTime);
}

void ProcessInfo(DWORD processID)
{
	HANDLE hProcess;
	PROCESS_MEMORY_COUNTERS pmc;

	// Print the process identifier.
	printf("\t\t\tProcess ID: %lu\n", processID);

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess == NULL) {
		printf("\t\t\tFailed to open handle to %lu\n", processID);
		return;
	}

	TCHAR path[MAX_PATH];
	if (GetModuleFileNameEx(hProcess, 0, path, MAX_PATH)) {
		printf("\t\t\tPath: %s\n", path);
	}

	ProcessTimes(hProcess);

	if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {

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
	printf("\n \
		\tSystem Info:\n \
		\tOEM ID: %lu\n \
		\tProcessor Architecture: %u\n \
		\tPage Size: %lu\n \
		\tMinimum Application Address: 0x%016llx\n \
		\tMaximum Application Address: 0x%016llx\n \
		\tActive Processor Mask: %lu\n \
		\tNumber of Processors: %lu\n \
		\tAllocation Granularity: %lu\n \
		\tProcessor Type: %lu\n \
		\tProcessor Level: %u\n \
		\tProcessor Revision: %u\n \
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
	printf("\n \
		\tGlobal Memory Info:\n \
		\tLength: %lu\n \
		\tMemory Load: %lu\n \
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

void GetAndPrintProcessInfo()
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;

	printf("\n\t\t\tProcess Info:\n");

	// Get the list of process identifiers
	if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {

		// Calculate how many process identifiers were actually returned.
		cProcesses = cbNeeded / sizeof(DWORD);
		printf("\t\t\tReturned %lu processes in array of size %lu\n", cProcesses, cbNeeded);
		for (unsigned int i = 0; i < cProcesses; i++) {
			printf("\n\t\t\tInfo for the %dth process\n", i);
			ProcessInfo(aProcesses[i]);
		}
	}
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
	GetAndPrintProcessInfo();

	return 0;
}


