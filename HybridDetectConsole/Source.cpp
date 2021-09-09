///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a   copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is    furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of 
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A    PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HybridDetect.h"
#include <windows.h>
#include <stdio.h>

int __cdecl main(int argc, char** argv)
{
    // Example Representation of a Physical Processor Mask from GLPI
    // 0000000000000000000000000000000000000000000000000000000000000001
    unsigned long physical_core_mask = 0x00000001; 

    // Example Representation of a Logical Processor Mask from GLPI
    // 0000000000000000000000000000000000000000000000000000000000000010
    unsigned long logical_core_mask = 0x00000002; 
    
    // Example Representation of a Shared Cache Mask from GLPI
    // 0000000000000000000000000000000000000000000000000000000000000011
    unsigned long l1_cache_mask = 0x00000003;  

    // Check to See if Physical Core Mask & Cache Mask are Related
    if (physical_core_mask & l1_cache_mask)
    {
        // Physical Core Uses L1 Cache
    }
    
    // Check to See if Physical & Logical Cores Share a Cache
    if ((physical_core_mask & l1_cache_mask) 
     && (logical_core_mask & l1_cache_mask))
    {
        // Physical Core and Logical Core share L1 Cache
    }

    printf("\nGroups\n");
    for (EnumLogicalProcessorInformation enumInfo(RelationGroup);
        auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
        printf("Active Group Count:      %d\n", pinfo->Group.ActiveGroupCount);
        printf("Maximum Group Count:     %d\n", pinfo->Group.MaximumGroupCount);
        printf("Active Processor Count:  %d\n", pinfo->Group.GroupInfo->ActiveProcessorCount);
        printf("Maximum Processor Count: %d\n", pinfo->Group.GroupInfo->MaximumProcessorCount);
        printf("Active Processor Mask:   ");
        PrintMask(pinfo->Group.GroupInfo->ActiveProcessorMask);
        printf("\n");
    }
    printf("\nNuma Nodes\n");
    for (EnumLogicalProcessorInformation enumInfo(RelationNumaNode);
        auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
        printf("Group                    %d\n", pinfo->NumaNode.GroupMask.Group);
        printf("Node Number:             %d\n", pinfo->NumaNode.NodeNumber);
        printf("NUMA Mask:               ");
        PrintMask(pinfo->NumaNode.GroupMask.Mask);
        printf("\n");
    }
    printf("\nProcessor Packages\n");
    for (EnumLogicalProcessorInformation enumInfo(RelationProcessorPackage);
        auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
        for (UINT GroupIndex = 0; GroupIndex < pinfo->Processor.GroupCount; GroupIndex++) {
            printf("Group                    %d\n", pinfo->Processor.GroupMask[GroupIndex].Group);
            printf("Group Count:             %d\n", pinfo->Processor.GroupCount);
            printf("Node Number:             %d\n", (int)pinfo->NumaNode.NodeNumber);
            printf("Efficiency Class:        %d\n", (int)pinfo->Processor.EfficiencyClass);
            printf("Package Mask:            ");
            PrintMask(pinfo->Processor.GroupMask[GroupIndex].Mask);
            printf("\n");
        }
    }
    printf("\nProcessors\n"); int i = 0;
    for (EnumLogicalProcessorInformation enumInfo(RelationProcessorCore);
        auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
        for (UINT GroupIndex = 0; GroupIndex < pinfo->Processor.GroupCount; GroupIndex++) {
            printf("Group                    %d\n", pinfo->Processor.GroupMask[GroupIndex].Group);
            printf("Group Count:             %d\n", pinfo->Processor.GroupCount);
            printf("Node Number:             %d\n", (int)pinfo->NumaNode.NodeNumber);
            printf("Processor:               %d\n", i++);
            printf("Efficiency Class:        %d\n", (int)pinfo->Processor.EfficiencyClass);
            printf("Hyperthreaded:           %s\n", pinfo->Processor.Flags ? "Yes" : "No");
            printf("Processor Mask:          ");
            PrintMask(pinfo->Processor.GroupMask[GroupIndex].Mask);
            printf("\n\n");
        }
    }
    printf("\nCaches\n"); i = 0;
    for (EnumLogicalProcessorInformation enumInfo(RelationCache);
        auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
        //for (UINT GroupIndex = 0; GroupIndex < pinfo->Processor.GroupCount; GroupIndex++) {
            printf("Group                    %d\n", pinfo->Cache.GroupMask.Group);
            printf("Cache:                   %d\n", i++);
            printf("Cache Level:             %d\n", (int)pinfo->Cache.Level);
            printf("Cache Type:              %s\n", CacheTypeString(pinfo->Cache.Type));
            printf("Line Size:               %d\n", (int)pinfo->Cache.LineSize);
            printf("Cache Size:              %d\n", (int)pinfo->Cache.CacheSize);
            printf("Associativity:           %d\n", (int)pinfo->Cache.Associativity);
            printf("Cache Mask:              ");
            PrintMask(pinfo->Cache.GroupMask.Mask);
            printf("\n\n");
        //}
    }

    return 0;
}