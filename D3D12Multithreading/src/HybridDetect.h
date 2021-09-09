#pragma once

#include <array>
#include <vector>
#include <intrin.h>
#include <string>
#include <windows.h>
#include <malloc.h>    
#include <stdio.h>
#include <bitset>
#include <map>
#include <assert.h>
#include <Powrprof.h>
#include <thread>

#pragma comment(lib, "Powrprof.lib")

#ifdef _WIN32
// Simple wrapper for __cpuid intrinsic, created for cross platform support e.g. WIN32
#define CPUID(registers, function) __cpuid((int*)registers, (int)function);
#define CPUIDEX(registers, function, extFunction) __cpuidex((int*)registers, (int)function, (int)extFunction);
#else 
// Linux Stuff
#define CPUID(registers, function) asm volatile ("cpuid" : "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]) : "a" (function), "c" (0));
#define CPUIDEX(registers, function, extFunction) asm volatile ("cpuid" : "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]) : "a" (function), "c" (extFunction));
#endif

// Enables/Disables Hybrid Detect
#define ENABLE_HYBRID_DETECT

// Tells the application to treat the target system as a heterogeneous software proxy.
#define ENABLE_SOFTWARE_PROXY	

// Enables/Disables Run On API
#define ENABLE_RUNON

// Simple conversion from an ordinal, n, to a set bit at position n
#define IndexToMask(n)  (1UL << n)

// Macros to store values for CPUID register ordinals
#define CPUID_EAX								0
#define CPUID_EBX								1
#define CPUID_ECX								2
#define CPUID_EDX								3

#define LEAF_CPUID_BASIC						0x00        // Basic CPUID Information
#define LEAF_THERMAL_POWER						0x06        // Thermal and Power Management Leaf
#define LEAF_EXTENDED_FEATURE_FLAGS				0x07        // Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
#define LEAF_EXTENDED_STATE						0x0D        // Processor Extended State Enumeration Main Leaf (EAX = 0DH, ECX = 0)
#define LEAF_FREQUENCY_INFORMATION				0x16        // Processor Frequency Information Leaf  function 0x16 only works on Skylake or newer.
#define LEAF_HYBRID_INFORMATION					0x1A        // Hybrid Information Sub - leaf(EAX = 1AH, ECX = 0)
#define LEAF_EXTENDED_INFORMATION_0				0x80000000  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_1				0x80000001  // Extended Function CPUID Information
#define LEAF_EXTENDED_BRAND_STRING_1			0x80000002  // Extended Function CPUID Information
#define LEAF_EXTENDED_BRAND_STRING_2			0x80000003  // Extended Function CPUID Information
#define LEAF_EXTENDED_BRAND_STRING_3			0x80000004  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_5				0x80000005  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_6				0x80000006  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_7				0x80000007  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_8				0x80000008  // Extended Function CPUID Information

enum CoreTypes
{
	ANY											= -1,
	NONE										= 0x00,
	RESERVED0									= 0x10,
	INTEL_ATOM									= 0x20,
	RESERVED1									= 0x30,
	INTEL_CORE									= 0x40,
	MAX											= 6
};

// Struct to store information for each Cache.
typedef struct _CACHE_INFO
{
	std::bitset<64>     processorMask			= 0;
	unsigned            level					= 0;
	unsigned            size					= 0;
	unsigned            lineSize				= 0;
	unsigned            type					= 0;
	unsigned            associativity			= 0;
} CACHE_INFO, *PCACHE_INFO;

// Struct to store Power information for a logical processor.
typedef struct _LOGICAL_PROCESSOR_POWER_INFORMATION {
	ULONG               number					= 0;
	ULONG               maxMhz					= 0;
	ULONG               currentMhz				= 0;
	ULONG               mhzLimit				= 0;
	ULONG               maxIdleState			= 0;
	ULONG               currentIdleState		= 0;
} LOGICAL_PROCESSOR_POWER_INFORMATION, *PLOGICAL_PROCESSOR_POWER_INFORMATION;

// Struct to store Power information for a logical processor.
typedef struct _LOGICAL_PROCESSOR_INFO
{
	CoreTypes          coreType					= CoreTypes::NONE;
	std::bitset<64>    processorMask			= 0;
	unsigned           processorNumber			= 0;
	unsigned           baseFrequency			= 0;
	unsigned           maximumFrequency			= 0;
	unsigned           busFrequency				= 0;
	unsigned           SSE						: 1;
	unsigned           AVX						: 1;
	unsigned           AVX2						: 1;
	unsigned           AVX512					: 1;
	unsigned           AVX512F					: 1;
	unsigned           AVX512DQ					: 1;
	unsigned           AVX512PF					: 1;
	unsigned           AVX512ER					: 1;
	unsigned           AVX512CD					: 1;
	unsigned           AVX512BW					: 1;
	unsigned           AVX512VL					: 1;
	unsigned           AVX512_IFMA				: 1;
	unsigned           AVX512_VBMI				: 1;
	unsigned           AVX512_VBMI2				: 1;
	unsigned           AVX512_VNNI				: 1;
	unsigned           AVX512_BITALG			: 1;
	unsigned           AVX512_VPOPCNTDQ			: 1;
	unsigned           AVX512_4VNNIW			: 1;
	unsigned           AVX512_4FMAPS			: 1;
	unsigned           AVX512_VP2INTERSECT		: 1;
	unsigned           SGX						: 1;
	unsigned           SHA						: 1;
	unsigned           currentFrequency			= 0;
} LOGICAL_PROCESSOR_INFO, *PLOGICAL_PROCESSOR_INFO;

// Struct to store Processor information
typedef struct _PROCESSOR_INFO
{
	char                vendorID[13];
	char                brandString[64];
	unsigned            numNUMANodes			= 0;
	unsigned            numProcessorPackages	= 0;
	unsigned            numPhysicalCores		= 0;
	unsigned            numLogicalCores			= 0;
	unsigned            numL1Caches				= 0;
	unsigned            numL2Caches				= 0;
	unsigned            numL3Caches				= 0;
	unsigned            hybrid					: 1;
	unsigned            turboBoost				: 1;
	unsigned	        turboBoost3_0			: 1;
	std::vector<CACHE_INFO>					caches;
	std::map<short, ULONG64>				coreMasks;
	std::vector<LOGICAL_PROCESSOR_INFO>		logicalCores;
#ifdef  _DEBUG
	std::vector<std::array<int, 4>>			leafs;
	std::vector<std::array<int, 4>>			extendedLeafs;
#endif
	const bool IsIntel()    const { return !strcmp("GenuineIntel", vendorID); }
	const bool IsAMD()      const { return !strcmp("AuthenticAMD", vendorID); }
} PROCESSOR_INFO, *PPROCESSOR_INFO;

// Type to String Conversion Helper Function
inline const char* CoreTypeString(CoreTypes type)
{
	switch (type)
	{
	case CoreTypes::NONE:
		return "None";
	case CoreTypes::RESERVED0:
		return "Reserved_0";
	case CoreTypes::INTEL_ATOM:
		return "Atom";
	case CoreTypes::RESERVED1:
		return "Reserved_1";
	case CoreTypes::INTEL_CORE:
		return "Core";
	}
	return "Any";
}

// Cache to String Conversion Helper Function
inline const char* CacheTypeString(int type)
{
	switch (type)
	{
	case CacheUnified:
		return "Unified";
	case CacheInstruction:
		return "Instruction";
	case CacheData:
		return "Data";
	case CacheTrace:
		return "Trace";
	}
	return "Any";
}

// Helper function to Call the CPUID intrinsic
inline bool CallCPUID(unsigned function, std::array<int, 4>& registers, unsigned extFunction = 0, int CPUIDFunctionMax = LEAF_EXTENDED_INFORMATION_8)
{
	if (function > CPUIDFunctionMax) return false;
	CPUIDEX(registers.data(), function, extFunction);
	return true;
}

// Calls CPUID & GetLogicalProcessors to fill in PROCESSOR_INFO Caches & Cores
inline bool GetLogicalProcessors(PROCESSOR_INFO& procInfo)
{
#ifdef ENABLE_HYBRID_DETECT
	// Based On: https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation?redirectedfrom=MSDN
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfoStart = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION currProcInfo = NULL;
	DWORD length = 0;
	PCACHE_DESCRIPTOR Cache;
	CACHE_INFO cacheInfo;

	while (true)
	{
		if (!GetLogicalProcessorInformation(procInfoStart, &length))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				//TODO: need hooks for malloc/free
				if (procInfoStart)
					free(procInfoStart);

				procInfoStart = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(length);

				if (procInfoStart == NULL)
					return false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			break;
		}
	}

	currProcInfo = procInfoStart;

	for (DWORD offset = 0; offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= length; offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION), currProcInfo++)
	{
		switch (currProcInfo->Relationship)
		{
		case RelationNumaNode:
			// Non-NUMA systems report a single record of this type.
			procInfo.numNUMANodes++;
			break;
		case RelationProcessorCore:
			// A hyperthreaded core supplies more than one logical processor.
			procInfo.numPhysicalCores++;
			procInfo.numLogicalCores += std::bitset<64>(currProcInfo->ProcessorMask).count();
			break;
		case RelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			Cache = &currProcInfo->Cache;

			if (Cache->Level == 1) procInfo.numL1Caches++;
			if (Cache->Level == 2) procInfo.numL2Caches++;
			if (Cache->Level == 3) procInfo.numL3Caches++;

			cacheInfo.processorMask = currProcInfo->ProcessorMask;
			cacheInfo.level = Cache->Level;
			cacheInfo.size = Cache->Size;
			cacheInfo.lineSize = Cache->LineSize;
			cacheInfo.associativity = Cache->Associativity;
			cacheInfo.type = Cache->Type;

			procInfo.caches.push_back(cacheInfo);
			break;
		case RelationProcessorPackage:
			// Logical processors share a physical package.
			procInfo.numProcessorPackages++;
			break;
		default:
			break;
		}
	}

	//TODO: need hooks for malloc/free
	free(procInfoStart);
	return true;
#else
	return false;
#endif
}

inline void UpdateProcessorInfo(PROCESSOR_INFO& procInfo)
{
	for (int i = 0; i < procInfo.logicalCores.size(); i++)
	{
		// Query Current Frequency for each Logical Processor using CallNTPowerInformation
		// https://docs.microsoft.com/en-us/windows/win32/api/powerbase/nf-powerbase-callntpowerinformation
		std::vector<LOGICAL_PROCESSOR_POWER_INFORMATION> pwrInfo(procInfo.numLogicalCores);
		DWORD size = sizeof(LOGICAL_PROCESSOR_POWER_INFORMATION) * procInfo.numLogicalCores;
		CallNtPowerInformation(ProcessorInformation, nullptr, 0, &pwrInfo[0], size);

		procInfo.logicalCores[i].currentFrequency = pwrInfo[i].currentMhz;
	}
}

// Calls CPUID & GetLogicalProcessors & CallNTPowerInformation to fill in PROCESSOR_INFO
inline void GetProcessorInfo(PROCESSOR_INFO& procInfo)
{
#ifdef ENABLE_HYBRID_DETECT
	// https://software.intel.com/content/www/us/en/develop/download/intel-architecture-instruction-set-extensions-programming-reference.html

		// Store registers from CPUID
	std::array<int, 4>  cpuInfo;

	// Maximum leaf ordinal Reported by CPUID
	unsigned            CPUIDFunctionMax;

	DWORD_PTR           affinityMask;
	DWORD_PTR           processAffinityMask;
	DWORD_PTR           sysAffinityMask;
	std::bitset<32>     bits;

	procInfo.coreMasks.emplace(CoreTypes::NONE, 0);
	procInfo.coreMasks.emplace(CoreTypes::ANY, 0);
	procInfo.coreMasks.emplace(CoreTypes::INTEL_CORE, 0);
	procInfo.coreMasks.emplace(CoreTypes::INTEL_ATOM, 0);
	procInfo.coreMasks.emplace(CoreTypes::RESERVED0, 0);
	procInfo.coreMasks.emplace(CoreTypes::RESERVED1, 0);

	//Basic CPUID Information
	CallCPUID(LEAF_CPUID_BASIC, cpuInfo);

	// Store highest function number.
	CPUIDFunctionMax = cpuInfo[CPUID_EAX];

	// Read Vendor ID from CPUID
	memcpy(procInfo.vendorID + 0, &cpuInfo[CPUID_EBX], sizeof(cpuInfo[CPUID_EBX]));
	memcpy(procInfo.vendorID + 4, &cpuInfo[CPUID_EDX], sizeof(cpuInfo[CPUID_EDX]));
	memcpy(procInfo.vendorID + 8, &cpuInfo[CPUID_ECX], sizeof(cpuInfo[CPUID_ECX]));
	procInfo.vendorID[12] = '\0';

	// Read Brand String from Extended CPUID information
	CallCPUID(LEAF_EXTENDED_BRAND_STRING_1, cpuInfo);
	memcpy(procInfo.brandString + 00, cpuInfo.data(), sizeof(cpuInfo));
	CallCPUID(LEAF_EXTENDED_BRAND_STRING_2, cpuInfo);
	memcpy(procInfo.brandString + 16, cpuInfo.data(), sizeof(cpuInfo));
	CallCPUID(LEAF_EXTENDED_BRAND_STRING_3, cpuInfo);
	memcpy(procInfo.brandString + 32, cpuInfo.data(), sizeof(cpuInfo));

	// Structured Extended Feature Flags Enumeration Leaf 
	// (Output depends on ECX input value)
	CallCPUID(LEAF_EXTENDED_FEATURE_FLAGS, cpuInfo);
	{
		bits = cpuInfo[CPUID_EDX];
#ifndef ENABLE_SOFTWARE_PROXY
		procInfo.hybrid = bits[15];
#else
		procInfo.hybrid = true;
#endif
	}

	// Thermal and Power Management Leaf
	CallCPUID(LEAF_THERMAL_POWER, cpuInfo);
	{
		bits = cpuInfo[CPUID_EAX];
		procInfo.turboBoost = bits[1];
		procInfo.turboBoost3_0 = bits[14];
	}

	// What does the process & system allow
	GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &sysAffinityMask);

	// Fill in Logical Processors, short circuit if error.
	if (GetLogicalProcessors(procInfo))
	{
		// Query Current Frequency for each Logical Processor using CallNTPowerInformation
		// https://docs.microsoft.com/en-us/windows/win32/api/powerbase/nf-powerbase-callntpowerinformation
		std::vector<LOGICAL_PROCESSOR_POWER_INFORMATION> pwrInfo(procInfo.numLogicalCores);
		DWORD size = sizeof(LOGICAL_PROCESSOR_POWER_INFORMATION) * procInfo.numLogicalCores;
		CallNtPowerInformation(ProcessorInformation, nullptr, 0, &pwrInfo[0], size);

		// Enumerate each logical core.
		for (unsigned core = 0; core < procInfo.numLogicalCores; core++)
		{
			// Logical Processor Info struct for storage.
			LOGICAL_PROCESSOR_INFO                  logicalCore;

			// Get the ID of the current processor;
			int                                     prevProcessor = GetCurrentProcessorNumber();

			// Convert the oridinal position to an affinity mask.
			affinityMask = IndexToMask(core);

			// Thread Affinity Mask is enough to switch to current thread immediatlely. 
			SetThreadAffinityMask(GetCurrentThread(), processAffinityMask & affinityMask);
			//SetThreadIdealProcessor(GetCurrentThread(), core); // Requires Spin Wait below and doesn't pin to ideal core. 

			logicalCore.processorMask = std::bitset<64>(affinityMask);

			// Processor Extended State Enumeration Main Leaf (EAX = 0DH, ECX = 0)
			CallCPUID(LEAF_EXTENDED_STATE, cpuInfo);
			{
				bits = cpuInfo[CPUID_EAX];
				logicalCore.SSE = bits[1];
				logicalCore.AVX = bits[2];
				logicalCore.AVX512 = bits[5];
			}

			// Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
			CallCPUID(LEAF_EXTENDED_FEATURE_FLAGS, cpuInfo);
			{
				bits = cpuInfo[CPUID_EBX];

				logicalCore.AVX2 = bits[5];
				logicalCore.AVX512F = bits[16];
				logicalCore.AVX512DQ = bits[17];
				logicalCore.AVX512_IFMA = bits[21];
				logicalCore.AVX512PF = bits[26];
				logicalCore.AVX512ER = bits[27];
				logicalCore.AVX512CD = bits[28];
				logicalCore.SHA = bits[29];
				logicalCore.SGX = bits[2];
				logicalCore.AVX512BW = bits[30];
				logicalCore.AVX512VL = bits[31];

				bits = cpuInfo[CPUID_ECX];
				logicalCore.AVX512_VBMI = bits[1];
				logicalCore.AVX512_VBMI2 = bits[6];
				logicalCore.AVX512_VNNI = bits[11];
				logicalCore.AVX512_BITALG = bits[12];
				logicalCore.AVX512_VPOPCNTDQ = bits[14];

				bits = cpuInfo[CPUID_EDX];
				logicalCore.AVX512_4VNNIW = bits[2];
				logicalCore.AVX512_4FMAPS = bits[3];
				logicalCore.AVX512_VP2INTERSECT = bits[8];
			}



			// Processor Frequency Information Leaf  function 0x16 only works on Sky-lake or newer.
			CallCPUID(LEAF_FREQUENCY_INFORMATION, cpuInfo);
			{
				logicalCore.baseFrequency = cpuInfo[CPUID_EAX];
				logicalCore.maximumFrequency = cpuInfo[CPUID_EBX];
				logicalCore.busFrequency = cpuInfo[CPUID_ECX];

				// Fall-back to ProcessorPowerInfo for older CPUs
				if (logicalCore.baseFrequency == 0)
				{
					logicalCore.baseFrequency = pwrInfo[core].mhzLimit;
					logicalCore.maximumFrequency = pwrInfo[core].mhzLimit;
				}

				logicalCore.currentFrequency = pwrInfo[core].currentMhz;
			}

			// Hybrid Information Sub - leaf(EAX = 1AH, ECX = 0)
			CallCPUID(LEAF_HYBRID_INFORMATION, cpuInfo);
			{
#ifndef ENABLE_SOFTWARE_PROXY
				std::bitset<8> coreTypeBits;        // Bits 31 - 24: Core type
				bits = cpuInfo[CPUID_EAX];
				coreTypeBits[0] = bits[24];
				coreTypeBits[1] = bits[25];
				coreTypeBits[2] = bits[26];
				coreTypeBits[3] = bits[27];
				coreTypeBits[4] = bits[28];
				coreTypeBits[5] = bits[29];
				coreTypeBits[6] = bits[30];
				coreTypeBits[7] = bits[31];
				logicalCore.coreType = (CoreTypes)coreTypeBits.to_ulong();
#else
				// Split homogeneous cores into logical hetergeneous regions
				logicalCore.coreType =
					(core < (procInfo.numLogicalCores / 2) ? CoreTypes::INTEL_CORE : CoreTypes::INTEL_ATOM);
#endif
			}

			// Heterogeneous processor constellation
			procInfo.coreMasks[CoreTypes::ANY] |= affinityMask;

			// Homgeneous processsor constellations
			procInfo.coreMasks[logicalCore.coreType] |= affinityMask;

			procInfo.logicalCores.push_back(logicalCore);
			//pwrInfo.clear();
		}

		// Reset Process Affinity Mask
		SetThreadAffinityMask(GetCurrentThread(), processAffinityMask);
	}

#ifdef _DEBUG
	// Read Child Leafs 
	for (unsigned i = 0; i <= 30; ++i)
	{
		CallCPUID(i, cpuInfo);
		procInfo.leafs.push_back(cpuInfo);
	}

	// Extended CPUID information
	for (unsigned i = LEAF_EXTENDED_INFORMATION_0; i <= LEAF_EXTENDED_INFORMATION_8; ++i)
	{
		CallCPUID(i, cpuInfo);
		procInfo.extendedLeafs.push_back(cpuInfo);
	}
#endif
#endif
}

// Run A Current Thread On A Custom Logical Processor Constellation
inline bool RunOnMask(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const ULONG64 mask, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	DWORD_PTR  processAffinityMask;
	DWORD_PTR  systemAffinityMask;

	// Get the system and process affinity mask
	GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

	ULONG64 threadAffinityMask = mask;

	assert((systemAffinityMask & threadAffinityMask));
	assert((processAffinityMask & threadAffinityMask));

	// Is the thread-mask allowed in this system/process?
	if ((systemAffinityMask & threadAffinityMask) && (processAffinityMask & threadAffinityMask))
	{
		// Run On Thread In Process
		SetThreadAffinityMask(threadHandle, processAffinityMask & threadAffinityMask);
		return true;
	}

	assert(systemAffinityMask & fallbackMask);
	assert(processAffinityMask & fallbackMask);

	// Is fall-back thread-mask allowed in this system/process?
	if ((systemAffinityMask & fallbackMask) && (processAffinityMask & fallbackMask))
	{
		SetThreadAffinityMask(threadHandle, processAffinityMask & fallbackMask);
	}
	else
	{
		// Fallback to process affinity mask!
		SetThreadAffinityMask(threadHandle, processAffinityMask);
	}
#endif
	return false;
}

// Run The Current Thread On A Custom Logical Processor Constellation
inline bool RunOnMask(PROCESSOR_INFO& procInfo, const ULONG64 mask, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnMask(procInfo, threadHandle, mask, fallbackMask);
#else
	return false;
#endif
}

// Run A Thread On the Atom or Core Logical Processor Constellation
inline bool RunOn(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const CoreTypes type, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	assert(procInfo.coreMasks.size());

	if (procInfo.coreMasks.size())
	{
		ULONG64 threadMask = procInfo.coreMasks[type];
		return RunOnMask(procInfo, threadHandle, threadMask, fallbackMask);
	}

	return RunOnMask(procInfo, threadHandle, fallbackMask);
#else
	return false;
#endif
}

// Run The Current Thread On Atom or Core Logical Processor Constellation
inline bool RunOn(PROCESSOR_INFO& procInfo, const CoreTypes type, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOn(procInfo, threadHandle, type, fallbackMask);
#else
	return false;
#endif
}

// Run A Thread On Any Logical Processor
inline bool RunOnAny(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	return RunOn(procInfo, threadHandle, CoreTypes::ANY, fallbackMask);
#else
	return false;
#endif 
}

// Run The Current Thread On Any Logical Processor
inline bool RunOnAny(PROCESSOR_INFO& procInfo, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnAny(procInfo, threadHandle, fallbackMask);
#else
	return false;
#endif
}

// Run A Thread On One Logical Processor
inline bool RunOnOne(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const short coreID, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	bool succeeded = false;

#ifdef _DEBUG
	int startedOn = GetCurrentProcessorNumber();
#endif
	if (coreID < procInfo.numLogicalCores)
	{
		bool succeeded = RunOnMask(procInfo, threadHandle, IndexToMask(coreID), fallbackMask);

#ifdef _DEBUG
		// Check to see if the core is pinned.
		// Surrender time-slice until the current thread is scheduled on a logical processor matching the affinity mask.
		int iterations = 0;
		int runningOn = -1;
		do {
			Sleep(0);
			runningOn = GetCurrentProcessorNumber();
			iterations++;
		} while (runningOn != coreID);
		assert(runningOn == coreID);
		int finishedOn = GetCurrentProcessorNumber();

		//std::cout << "Started On " << startedOn << ", Running On " << coreID << ", Finished On " << finishedOn << "; Ran in " << iterations << " iteration(s)." << std::endl;
		assert(runningOn == finishedOn);
#endif       

		return succeeded;
	}

	return RunOnMask(procInfo, threadHandle, IndexToMask(coreID), fallbackMask);
#else
	return false;
#endif
}

// Run The Current Thread On One Logical Processor
inline bool RunOnOne(PROCESSOR_INFO& procInfo, const short coreID, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnOne(procInfo, threadHandle, coreID, fallbackMask);
#else
	return false;
#endif
}