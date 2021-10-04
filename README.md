# Hybrid Detect

Hybrid Detect demonstrates CPU topology detection using multiple intrinsic and OS level APIs. First, we demonstrate usage of CPUID intrinsic to detect information leafs including the new Hybrid leaf offered for the latest Intel processors. Additionally, we use GetLogicalProcessorInformation() and GetLogicalProcessorInformationEX() to demonstrate full topology enumeration including Logical Core & Cache Relationships along with Affinity Masking. Finally we show how to use GetSystemCPUSetInformation() to get valid CPU Identifiers for use with SetThreadSelectedCPUSets() as well as how to read the Efficiency Class and other flags such as the Parked flag for each P-Core & E-Core.

In addition to topology detection several sample functions are demonstrated which control affinitization strategies for threads; these include weak affinity functions such as SetThreadIdealProcessor, SetThreadPriority, and SetThreadInformation, as well as strong affinity functions like SetThreadSelectedCPUSets and SetThreadAffinityMask.

HybridDetect.h is the primary source module for all Hybrid Detect functionality and requires no additional dependencies for integration into your project. 

# Projects in Solution

## Hybrid Detect Console

Simple console based unit test for HybridDetect.h

### HybridDetect.h Pre-Compiler Macros

	// Enables/Disables Hybrid Detect
	#define ENABLE_HYBRID_DETECT

	// Tells the application to treat the target system as a heterogeneous software proxy.
	//#define ENABLE_SOFTWARE_PROXY	

	// Enables/Disables Run On API
	#define ENABLE_RUNON

	// Enables/Disables ThreadPriority Based on Core-Type
	//#define ENABLE_RUNON_PRIORITY

	// Enables/Disables SetThreadInformation Memory Priority Based on Core-Type
	#define ENABLE_RUNON_MEMORY_PRIORITY

	// Enables/Disables SetThreadInformation Execution Speed based on Core-Type
	#define ENABLE_RUNON_EXECUTION_SPEED

	// Enables CPU-Sets and Disables ThreadAffinityMasks
	#define ENABLE_CPU_SETS

## D3D12Multithreading

Demonstrates logical processor and cache topology enumeration using HybridDetect.h in a DirectX 12 rendering environment.

## asteroids_d3d12

Demonstrates a variety of task scheduling scenarios with a simple task schedule using pre-compiler flags. Demonstrates split topology threadpools, as well as homogeneous/heterogeneous threadpool adaption. Rendering is done via the critical P-Cores and asteroid simulation is performed using E-Cores. Render/Update tasks can be composed into multiple task-dependency relationships, including SingleThreaded, NoDependency, OneToOne, Batched, and Asymmetric.

### asteroids_d3d12 Command Line Arguments

'-scheduler [0-4]' controls how Render/Update task dependecies are composed

	-scheduler 0 (Single Threaded)
	-scheduler 1 (No Dependency)
	-scheduler 2 (OneToOne)
	-scheduler 3 (Batched)
	-scheduler 4 (Asymetric)

### asteroids_d3d12 Logical Threadpool Pre-Compiler

For Default Split-Topology threadpool, use the following pre-compiler flags: 

	#define RESERVE_ANY     0 // Hybrid Only, 1 reserves 2 'Any' threads affinitized to P-Cores & E-Cores
	#define CORE_ONLY       0 // Hybrid Only, Run all Tasks in 'Core' threads.

For P-Core Only threadpool, use the following pre-compiler flags: 

	#define RESERVE_ANY     0 // Hybrid Only, 1 reserves 2 'Any' threads affinitized to P-Cores & E-Cores
	#define CORE_ONLY       1 // Hybrid Only, Run all Tasks in 'Core' threads.

To demonstrate an alternative threadpool topology that reserves two threads that execute on P-Cores and E-Cores: 

	#define RESERVE_ANY     1 // Hybrid Only, 1 reserves 2 'Any' threads affinitized to P-Cores & E-Cores
	#define CORE_ONLY       0 // Hybrid Only, Run all Tasks in 'Core' threads.


