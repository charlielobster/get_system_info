#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <d3d12.h>
#include <TCHAR.h>
#include <pdh.h>

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
static HANDLE self;

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

void initSystemInfoForProcessCPUTimes() 
{
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;
	GetSystemInfo(&sysInfo);
	numProcessors = sysInfo.dwNumberOfProcessors;
	GetSystemTimeAsFileTime(&ftime);
	memcpy(&lastCPU, &ftime, sizeof(FILETIME));
	self = GetCurrentProcess();
	GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
	memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}

double getSystemInfoForProcessCPUTimesCurrentValue() 
{
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;
	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));
	GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - lastSysCPU.QuadPart) +
		(user.QuadPart - lastUserCPU.QuadPart);
	percent /= (now.QuadPart - lastCPU.QuadPart);
	percent /= numProcessors;
	lastCPU = now;
	lastUserCPU = user;
	lastSysCPU = sys;
	return percent * 100;
}

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

void ProcessInfo(DWORD processID)
{
	HANDLE hProcess;
	PROCESS_MEMORY_COUNTERS pmc;

	// Print the process identifier.
	printf("\tProcess ID: %u\n", processID);

	// Print information about the process.
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (NULL == hProcess) {
		printf("\tFailed to open handle to %u\n", processID);
		return;
	}

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
		printf("\tPagefileUsage:             \t0x%08X\t(%d)\n", pmc.PagefileUsage, pmc.PagefileUsage);
		printf("\tPeakPagefileUsage:         \t0x%08X\t(%d)\n",
			pmc.PeakPagefileUsage, pmc.PeakPagefileUsage);
	} else {
		printf("\tFailed to get process memory info for %u\n", processID);
	}

	CloseHandle(hProcess);
}

int main(void)
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;

	// Get the list of process identifiers
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return 1;
	}

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the info for each process
	for (i = 0; i < cProcesses; i++)
	{
		printf("\n\tGetting info for the %dth process\n", i);
		double processTime = getSystemInfoForProcessCPUTimesCurrentValue()
		printf("\tCPU Time: %d\n", processTime );
		ProcessInfo(aProcesses[i]);
	}

	return 0;
}


