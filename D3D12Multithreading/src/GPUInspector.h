#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN            // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <stdexcept>

using namespace DirectX;

typedef struct FEATURE_DATA {
    std::string         feature_name;
    int                 feature_value = 0;
    bool                is_d3d12 = false;
    D3D12_FEATURE       d3d12_feature;
    D3D11_FEATURE       d3d11_feature;
    
    FEATURE_DATA(std::string& name, int value, D3D12_FEATURE dx12Feature)
    {
        feature_name = name;
        feature_value = value;
        d3d12_feature = dx12Feature;
        is_d3d12 = true;
    }

    FEATURE_DATA(std::string& name, int value, D3D11_FEATURE dx11Feature)
    {
        feature_name = name;
        feature_value = value;
        d3d11_feature = dx11Feature;
        is_d3d12 = false;
    }
} FEATURE_DATA;


typedef struct GPU_DEVICE_INFO
{
    int                                                         adapter_index;
    int                                                         device_id;
    std::wstring                                                adapter_name;
    long long                                                   adapter_vendor;
    long long                                                   adapter_video_memory;
    long long                                                   adapter_dedicated_memory;
    long long                                                   adapter_shared_memory;
    bool                                                        adapter_is_warp;
    IDXGIAdapter*                                               adapter = nullptr;
    ComPtr<ID3D12Device>                                        device_d3d12 = nullptr;
    D3D_FEATURE_LEVEL                                           device_d3d12_feature_level;
    ComPtr<ID3D11Device>                                        device_d3d11 = nullptr;
    D3D_FEATURE_LEVEL                                           device_d3d11_feature_level;
    std::list<FEATURE_DATA*>                                    features;
} GPU_DEVICE_INFO;


using Microsoft::WRL::ComPtr;

#define CREATE_FEATURE(device, name, value, feature) device->features.push_back(new FEATURE_DATA(std::string(name), value, feature));

GPU_DEVICE_INFO* GetDeviceInfo(IDXGIAdapter1* adapter, int adapterIndex)
{
    ComPtr<ID3D12Device>        d3d12_device;
    ComPtr<ID3D11Device>        d3d11_device;
    ComPtr<ID3D11DeviceContext> d3d11_device_context;  
    DWORD                       d3d11_device_flags = 0;
    DXGI_ADAPTER_DESC1          adapter_desc;
    GPU_DEVICE_INFO*            device_info = new GPU_DEVICE_INFO();
    D3D_FEATURE_LEVEL           device_feature_level = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL           device_feature_levels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

#if defined(_DEBUG)
    d3d11_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    adapter->GetDesc1(&adapter_desc);

    device_info->adapter = adapter;
    device_info->adapter_index = adapterIndex;
    device_info->adapter_name = adapter_desc.Description;
    device_info->device_id = adapter_desc.DeviceId;
    device_info->adapter_video_memory = adapter_desc.DedicatedVideoMemory;
    device_info->adapter_dedicated_memory = adapter_desc.DedicatedSystemMemory;
    device_info->adapter_shared_memory = adapter_desc.SharedSystemMemory;
    device_info->adapter_vendor = adapter_desc.VendorId;
    device_info->adapter_is_warp = adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;

    if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device)))) {
        D3D12_FEATURE_DATA_ARCHITECTURE                             feature_architecture;
        D3D12_FEATURE_DATA_ARCHITECTURE1                            feature_architecture1;
        D3D12_FEATURE_DATA_FORMAT_SUPPORT                           feature_format_support;
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS               feature_multi_sample;
        D3D12_FEATURE_DATA_FORMAT_INFO                              feature_format_info;
        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT              feature_virtual_address;
        D3D12_FEATURE_DATA_SHADER_MODEL                             feature_shader_model;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE                           feature_root_signature;
        D3D12_FEATURE_DATA_SHADER_CACHE                             feature_shader_cache;
        D3D12_FEATURE_DATA_COMMAND_QUEUE_PRIORITY                   feature_command_queue;
        D3D12_FEATURE_DATA_EXISTING_HEAPS                           feature_existing_heaps;
        D3D12_FEATURE_DATA_SERIALIZATION                            feature_serialization;
        D3D12_FEATURE_DATA_CROSS_NODE                               feature_cross_node;
        D3D12_FEATURE_DATA_DISPLAYABLE                              feature_displayable;
        D3D12_FEATURE_DATA_QUERY_META_COMMAND                       feature_meta_command;
        D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_SUPPORT       protected_session_support;
        D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_TYPE_COUNT    protected_session_count;
        D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_TYPES         protected_session_types;
        D3D12_FEATURE_DATA_D3D12_OPTIONS                            feature_options;
        D3D12_FEATURE_DATA_D3D12_OPTIONS1                           feature_options1;
        D3D12_FEATURE_DATA_D3D12_OPTIONS2                           feature_options2;
        D3D12_FEATURE_DATA_D3D12_OPTIONS3                           feature_options3;
        D3D12_FEATURE_DATA_D3D12_OPTIONS4                           feature_options4;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5                           feature_options5;
        D3D12_FEATURE_DATA_D3D12_OPTIONS6                           feature_options6;
        D3D12_FEATURE_DATA_D3D12_OPTIONS7                           feature_options7;
        D3D12_FEATURE_DATA_D3D12_OPTIONS8                           feature_options8;
        D3D12_FEATURE_DATA_D3D12_OPTIONS9                           feature_options9;
        D3D12_FEATURE_DATA_D3D12_OPTIONS10                          feature_options10;
        D3D12_FEATURE_DATA_D3D12_OPTIONS11                          feature_options11;
        D3D12_FEATURE_DATA_D3D12_OPTIONS12                          feature_options12;
        D3D12_FEATURE_DATA_D3D12_OPTIONS13                          feature_options13;
        D3D12_FEATURE_DATA_FEATURE_LEVELS                           feature_levels = {
            _countof(device_feature_levels), device_feature_levels, D3D_FEATURE_LEVEL_11_0
        };

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_levels, sizeof(feature_levels)))) {
            device_feature_level = feature_levels.MaxSupportedFeatureLevel;
            CREATE_FEATURE(device_info, "MaxSupportedFeatureLevel",                                             feature_levels.MaxSupportedFeatureLevel,                                    D3D12_FEATURE_FEATURE_LEVELS);
            CREATE_FEATURE(device_info, "NumFeatureLevels",                                                     feature_levels.NumFeatureLevels,                                            D3D12_FEATURE_FEATURE_LEVELS);
        }
            
        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &feature_architecture, sizeof(feature_architecture)))) {
            CREATE_FEATURE(device_info, "CacheCohercentUMA",                                                    feature_architecture.CacheCoherentUMA,                                      D3D12_FEATURE_ARCHITECTURE);
            CREATE_FEATURE(device_info, "NodeIndex",                                                            feature_architecture.NodeIndex,                                             D3D12_FEATURE_ARCHITECTURE);
            CREATE_FEATURE(device_info, "TileBasedRenderer",                                                    feature_architecture.TileBasedRenderer,                                     D3D12_FEATURE_ARCHITECTURE);
            CREATE_FEATURE(device_info, "UMA",                                                                  feature_architecture.UMA,                                                   D3D12_FEATURE_ARCHITECTURE);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &feature_architecture1, sizeof(feature_architecture1)))) {
            CREATE_FEATURE(device_info, "CacheCohercentUMA",                                                    feature_architecture1.CacheCoherentUMA,                                     D3D12_FEATURE_ARCHITECTURE1);
            CREATE_FEATURE(device_info, "IsolatedMMU",                                                          feature_architecture1.IsolatedMMU,                                          D3D12_FEATURE_ARCHITECTURE1);
            CREATE_FEATURE(device_info, "NodeIndex",                                                            feature_architecture1.NodeIndex,                                            D3D12_FEATURE_ARCHITECTURE1);
            CREATE_FEATURE(device_info, "TileBasedRenderer",                                                    feature_architecture1.TileBasedRenderer,                                    D3D12_FEATURE_ARCHITECTURE1);
            CREATE_FEATURE(device_info, "UMA",                                                                  feature_architecture1.UMA,                                                  D3D12_FEATURE_ARCHITECTURE1);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &feature_format_support, sizeof(feature_format_support)))) {
            CREATE_FEATURE(device_info, "Format",                                                               feature_format_support.Format,                                              D3D12_FEATURE_FORMAT_SUPPORT);
            CREATE_FEATURE(device_info, "Support1",                                                             feature_format_support.Support1,                                            D3D12_FEATURE_FORMAT_SUPPORT);
            CREATE_FEATURE(device_info, "Support2",                                                             feature_format_support.Support2,                                            D3D12_FEATURE_FORMAT_SUPPORT);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &feature_multi_sample, sizeof(feature_multi_sample)))) {
            CREATE_FEATURE(device_info, "Flags",                                                                feature_multi_sample.Flags,                                                 D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS);
            CREATE_FEATURE(device_info, "Format",                                                               feature_multi_sample.Format,                                                D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS);
            CREATE_FEATURE(device_info, "NumQualityLevels",                                                     feature_multi_sample.NumQualityLevels,                                      D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS);
            CREATE_FEATURE(device_info, "SampleCount",                                                          feature_multi_sample.SampleCount,                                           D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &feature_format_info, sizeof(feature_format_info)))) {
            CREATE_FEATURE(device_info, "Format",                                                               feature_format_info.Format,                                                 D3D12_FEATURE_FORMAT_INFO);
            CREATE_FEATURE(device_info, "PlaneCount",                                                           feature_format_info.PlaneCount,                                             D3D12_FEATURE_FORMAT_INFO);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &feature_virtual_address, sizeof(feature_virtual_address)))) {
            CREATE_FEATURE(device_info, "MaxGPUVirtualAddressBitsPerProcess",                                   feature_virtual_address.MaxGPUVirtualAddressBitsPerProcess,                 D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT);
            CREATE_FEATURE(device_info, "MaxGPUVirtualAddressBitsPerResource",                                  feature_virtual_address.MaxGPUVirtualAddressBitsPerResource,                D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_root_signature, sizeof(feature_root_signature)))) {
            CREATE_FEATURE(device_info, "HighestVersion",                                                       feature_root_signature.HighestVersion,                                      D3D12_FEATURE_ROOT_SIGNATURE);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &feature_shader_cache, sizeof(feature_shader_cache)))) {
            CREATE_FEATURE(device_info, "SupportFlags",                                                         feature_shader_cache.SupportFlags,                                          D3D12_FEATURE_SHADER_CACHE);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_COMMAND_QUEUE_PRIORITY, &feature_command_queue, sizeof(feature_command_queue)))) {
            CREATE_FEATURE(device_info, "CommandListType",                                                      feature_command_queue.CommandListType,                                      D3D12_FEATURE_COMMAND_QUEUE_PRIORITY);
            CREATE_FEATURE(device_info, "Priority",                                                             feature_command_queue.Priority,                                             D3D12_FEATURE_COMMAND_QUEUE_PRIORITY);
            CREATE_FEATURE(device_info, "PriorityForTypeIsSupported",                                           feature_command_queue.PriorityForTypeIsSupported,                           D3D12_FEATURE_COMMAND_QUEUE_PRIORITY);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_EXISTING_HEAPS, &feature_existing_heaps, sizeof(feature_existing_heaps)))) {
            CREATE_FEATURE(device_info, "Supported",                                                            feature_existing_heaps.Supported,                                           D3D12_FEATURE_EXISTING_HEAPS);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_SERIALIZATION, &feature_serialization, sizeof(feature_serialization)))) {
            CREATE_FEATURE(device_info, "HeapSerializationTier",                                                feature_serialization.HeapSerializationTier,                                D3D12_FEATURE_SERIALIZATION);
            CREATE_FEATURE(device_info, "NodeIndex",                                                            feature_serialization.NodeIndex,                                            D3D12_FEATURE_SERIALIZATION);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_CROSS_NODE, &feature_cross_node, sizeof(feature_cross_node)))) {
            CREATE_FEATURE(device_info, "AtomicShaderInstructions",                                             feature_cross_node.AtomicShaderInstructions,                                D3D12_FEATURE_CROSS_NODE);
            CREATE_FEATURE(device_info, "SharingTier",                                                          feature_cross_node.SharingTier,                                             D3D12_FEATURE_CROSS_NODE);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_DISPLAYABLE, &feature_displayable, sizeof(feature_displayable)))) {
            CREATE_FEATURE(device_info, "DisplayableTexture",                                                   feature_displayable.DisplayableTexture,                                     D3D12_FEATURE_DISPLAYABLE);
            CREATE_FEATURE(device_info, "SharedResourceCompatibilityTier",                                      feature_displayable.SharedResourceCompatibilityTier,                        D3D12_FEATURE_DISPLAYABLE);
        }
        
        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_QUERY_META_COMMAND, &feature_meta_command, sizeof(feature_meta_command)))) {
            CREATE_FEATURE(device_info, "CommandId.Data1",                                                      feature_meta_command.CommandId.Data1,                                       D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "CommandId.Data2",                                                      feature_meta_command.CommandId.Data2,                                       D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "CommandId.Data3",                                                      feature_meta_command.CommandId.Data3,                                       D3D12_FEATURE_QUERY_META_COMMAND);
            //CREATE_FEATURE(device_info, "CommandId.Data4", feature_meta_command.CommandId.Data4, D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "NodeMask",                                                             feature_meta_command.NodeMask,                                              D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "QueryInputDataSizeInBytes",                                            feature_meta_command.QueryInputDataSizeInBytes,                             D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "QueryOutputDataSizeInBytes",                                           feature_meta_command.QueryOutputDataSizeInBytes,                            D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "pQueryInputData (Addr)",                                               (int)feature_meta_command.pQueryInputData,                                  D3D12_FEATURE_QUERY_META_COMMAND);
            CREATE_FEATURE(device_info, "pQueryOutputData (Addr)",                                              (int)feature_meta_command.pQueryOutputData,                                 D3D12_FEATURE_QUERY_META_COMMAND);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT, &protected_session_support, sizeof(protected_session_support)))) {
            CREATE_FEATURE(device_info, "NodeIndex",                                                            protected_session_support.NodeIndex,                                        D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT);
            CREATE_FEATURE(device_info, "Support",                                                              protected_session_support.Support,                                          D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPE_COUNT, &protected_session_count, sizeof(protected_session_count)))) {
            CREATE_FEATURE(device_info, "Count",                                                                protected_session_count.Count,                                              D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPE_COUNT);
            CREATE_FEATURE(device_info, "NodeIndex",                                                            protected_session_count.NodeIndex,                                          D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPE_COUNT);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPES, &protected_session_types, sizeof(protected_session_types)))) {
            CREATE_FEATURE(device_info, "Count",                                                                protected_session_types.Count,                                              D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPES);
            CREATE_FEATURE(device_info, "NodeIndex",                                                            protected_session_types.NodeIndex,                                          D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPES);
            CREATE_FEATURE(device_info, "pTypes (Addr)",                                                        (int)protected_session_types.pTypes,                                        D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_TYPES);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &feature_options, sizeof(feature_options)))) {
            CREATE_FEATURE(device_info, "ConservativeRasterizationTier",                                        feature_options.ConservativeRasterizationTier,                              D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "CrossAdapterRowMajorTextureSupported",                                 feature_options.CrossAdapterRowMajorTextureSupported,                       D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "CrossNodeSharingTier",                                                 feature_options.CrossNodeSharingTier,                                       D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "DoublePrecisionFloatShaderOps",                                        feature_options.DoublePrecisionFloatShaderOps,                              D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "MaxGPUVirtualAddressBitsPerResource",                                  feature_options.MaxGPUVirtualAddressBitsPerResource,                        D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "MinPrecisionSupport",                                                  feature_options.MinPrecisionSupport,                                        D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "OutputMergerLogicOp",                                                  feature_options.OutputMergerLogicOp,                                        D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "PSSpecifiedStencilRefSupported",                                       feature_options.PSSpecifiedStencilRefSupported,                             D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "ResourceBindingTier",                                                  feature_options.ResourceBindingTier,                                        D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "ResourceHeapTier",                                                     feature_options.ResourceHeapTier,                                           D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "ROVsSupported",                                                        feature_options.ROVsSupported,                                              D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "StandardSwizzle64KBSupported",                                         feature_options.StandardSwizzle64KBSupported,                               D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "TiledResourcesTier",                                                   feature_options.TiledResourcesTier,                                         D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "TypedUAVLoadAdditionalFormats",                                        feature_options.TypedUAVLoadAdditionalFormats,                              D3D12_FEATURE_D3D12_OPTIONS);
            CREATE_FEATURE(device_info, "VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation", feature_options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation, D3D12_FEATURE_D3D12_OPTIONS);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &feature_options1, sizeof(feature_options1)))) {
            CREATE_FEATURE(device_info, "ExpandedComputeResourceStates",                                        feature_options1.ExpandedComputeResourceStates,                             D3D12_FEATURE_D3D12_OPTIONS1);
            CREATE_FEATURE(device_info, "Int64ShaderOps",                                                       feature_options1.Int64ShaderOps,                                            D3D12_FEATURE_D3D12_OPTIONS1);
            CREATE_FEATURE(device_info, "TotalLaneCount",                                                       feature_options1.TotalLaneCount,                                            D3D12_FEATURE_D3D12_OPTIONS1);
            CREATE_FEATURE(device_info, "WaveLaneCountMax",                                                     feature_options1.WaveLaneCountMax,                                          D3D12_FEATURE_D3D12_OPTIONS1);
            CREATE_FEATURE(device_info, "WaveLaneCountMin",                                                     feature_options1.WaveLaneCountMin,                                          D3D12_FEATURE_D3D12_OPTIONS1);
            CREATE_FEATURE(device_info, "WaveOps",                                                              feature_options1.WaveOps,                                                   D3D12_FEATURE_D3D12_OPTIONS1);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &feature_options2, sizeof(feature_options2)))) {
            CREATE_FEATURE(device_info, "DepthBoundsTestSupported",                                             feature_options2.DepthBoundsTestSupported,                                  D3D12_FEATURE_D3D12_OPTIONS2);
            CREATE_FEATURE(device_info, "ProgrammableSamplePositionsTier",                                      feature_options2.ProgrammableSamplePositionsTier,                           D3D12_FEATURE_D3D12_OPTIONS2);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &feature_options3, sizeof(feature_options3)))) {
            CREATE_FEATURE(device_info, "BarycentricsSupported",                                                feature_options3.BarycentricsSupported,                                     D3D12_FEATURE_D3D12_OPTIONS3);
            CREATE_FEATURE(device_info, "CastingFullyTypedFormatSupported",                                     feature_options3.CastingFullyTypedFormatSupported,                          D3D12_FEATURE_D3D12_OPTIONS3);
            CREATE_FEATURE(device_info, "CopyQueueTimestampQueriesSupported",                                   feature_options3.CopyQueueTimestampQueriesSupported,                        D3D12_FEATURE_D3D12_OPTIONS3);
            CREATE_FEATURE(device_info, "ViewInstancingTier",                                                   feature_options3.ViewInstancingTier,                                        D3D12_FEATURE_D3D12_OPTIONS3);
            CREATE_FEATURE(device_info, "WriteBufferImmediateSupportFlags",                                     feature_options3.WriteBufferImmediateSupportFlags,                          D3D12_FEATURE_D3D12_OPTIONS3);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &feature_options4, sizeof(feature_options4)))) {
            CREATE_FEATURE(device_info, "MSAA64KBAlignedTextureSupported",                                      feature_options4.MSAA64KBAlignedTextureSupported,                           D3D12_FEATURE_D3D12_OPTIONS4);
            CREATE_FEATURE(device_info, "Native16BitShaderOpsSupported",                                        feature_options4.Native16BitShaderOpsSupported,                             D3D12_FEATURE_D3D12_OPTIONS4);
            CREATE_FEATURE(device_info, "SharedResourceCompatibilityTier",                                      feature_options4.SharedResourceCompatibilityTier,                           D3D12_FEATURE_D3D12_OPTIONS4);
        }   

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_options5, sizeof(feature_options5)))) {
            CREATE_FEATURE(device_info, "RaytracingTier",                                                       feature_options5.RaytracingTier,                                            D3D12_FEATURE_D3D12_OPTIONS5);
            CREATE_FEATURE(device_info, "RenderPassesTier",                                                     feature_options5.RenderPassesTier,                                          D3D12_FEATURE_D3D12_OPTIONS5);
            CREATE_FEATURE(device_info, "SRVOnlyTiledResourceTier3",                                            feature_options5.SRVOnlyTiledResourceTier3,                                 D3D12_FEATURE_D3D12_OPTIONS5);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &feature_options6, sizeof(feature_options6)))) {
            CREATE_FEATURE(device_info, "AdditionalShadingRatesSupported",                                      feature_options6.AdditionalShadingRatesSupported,                           D3D12_FEATURE_D3D12_OPTIONS6);
            CREATE_FEATURE(device_info, "BackgroundProcessingSupported",                                        feature_options6.BackgroundProcessingSupported,                             D3D12_FEATURE_D3D12_OPTIONS6);
            CREATE_FEATURE(device_info, "PerPrimitiveShadingRateSupportedWithViewportIndexing",                 feature_options6.PerPrimitiveShadingRateSupportedWithViewportIndexing,      D3D12_FEATURE_D3D12_OPTIONS6);
            CREATE_FEATURE(device_info, "ShadingRateImageTileSize",                                             feature_options6.ShadingRateImageTileSize,                                  D3D12_FEATURE_D3D12_OPTIONS6);
            CREATE_FEATURE(device_info, "VariableShadingRateTier",                                              feature_options6.VariableShadingRateTier,                                   D3D12_FEATURE_D3D12_OPTIONS6);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &feature_options7, sizeof(feature_options7)))) {
            CREATE_FEATURE(device_info, "MeshShaderTier",                                                       feature_options7.MeshShaderTier,                                            D3D12_FEATURE_D3D12_OPTIONS7);
            CREATE_FEATURE(device_info, "SamplerFeedbackTier",                                                  feature_options7.SamplerFeedbackTier,                                       D3D12_FEATURE_D3D12_OPTIONS7);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &feature_options8, sizeof(feature_options8)))) {
            CREATE_FEATURE(device_info, "UnalignedBlockTexturesSupported",                                      feature_options8.UnalignedBlockTexturesSupported,                           D3D12_FEATURE_D3D12_OPTIONS8);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &feature_options9, sizeof(feature_options9)))) {
            CREATE_FEATURE(device_info, "AtomicInt64OnGroupSharedSupported",                                    feature_options9.AtomicInt64OnGroupSharedSupported,                         D3D12_FEATURE_D3D12_OPTIONS9);
            CREATE_FEATURE(device_info, "AtomicInt64OnTypedResourceSupported",                                  feature_options9.AtomicInt64OnTypedResourceSupported,                       D3D12_FEATURE_D3D12_OPTIONS9);
            CREATE_FEATURE(device_info, "DerivativesInMeshAndAmplificationShadersSupported",                    feature_options9.DerivativesInMeshAndAmplificationShadersSupported,         D3D12_FEATURE_D3D12_OPTIONS9);
            CREATE_FEATURE(device_info, "MeshShaderPipelineStatsSupported",                                     feature_options9.MeshShaderPipelineStatsSupported,                          D3D12_FEATURE_D3D12_OPTIONS9);
            CREATE_FEATURE(device_info, "MeshShaderSupportsFullRangeRenderTargetArrayIndex",                    feature_options9.MeshShaderSupportsFullRangeRenderTargetArrayIndex,         D3D12_FEATURE_D3D12_OPTIONS9);
            CREATE_FEATURE(device_info, "WaveMMATier",                                                          feature_options9.WaveMMATier,                                               D3D12_FEATURE_D3D12_OPTIONS9);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &feature_options10, sizeof(feature_options10)))) {
            CREATE_FEATURE(device_info, "MeshShaderPerPrimitiveShadingRateSupported",                           feature_options10.MeshShaderPerPrimitiveShadingRateSupported,               D3D12_FEATURE_D3D12_OPTIONS10);
            CREATE_FEATURE(device_info, "VariableRateShadingSumCombinerSupported",                              feature_options10.VariableRateShadingSumCombinerSupported,                  D3D12_FEATURE_D3D12_OPTIONS10);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &feature_options11, sizeof(feature_options11)))) {
            CREATE_FEATURE(device_info, "AtomicInt64OnDescriptorHeapResourceSupported",                         feature_options11.AtomicInt64OnDescriptorHeapResourceSupported,             D3D12_FEATURE_D3D12_OPTIONS11);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &feature_options12, sizeof(feature_options12)))) {
            CREATE_FEATURE(device_info, "EnhancedBarriersSupported",                                            feature_options12.EnhancedBarriersSupported,                                D3D12_FEATURE_D3D12_OPTIONS12);
            CREATE_FEATURE(device_info, "MSPrimitivesPipelineStatisticIncludesCulledPrimitives",                feature_options12.MSPrimitivesPipelineStatisticIncludesCulledPrimitives,    D3D12_FEATURE_D3D12_OPTIONS12);
            CREATE_FEATURE(device_info, "RelaxedFormatCastingSupported",                                        feature_options12.RelaxedFormatCastingSupported,                            D3D12_FEATURE_D3D12_OPTIONS12);
        }

        if (SUCCEEDED(d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &feature_options13, sizeof(feature_options13)))) {
            CREATE_FEATURE(device_info, "AlphaBlendFactorSupported",                                            feature_options13.AlphaBlendFactorSupported,                                D3D12_FEATURE_D3D12_OPTIONS13);
            CREATE_FEATURE(device_info, "InvertedViewportDepthFlipsZSupported",                                 feature_options13.InvertedViewportDepthFlipsZSupported,                     D3D12_FEATURE_D3D12_OPTIONS13);
            CREATE_FEATURE(device_info, "InvertedViewportHeightFlipsYSupported",                                feature_options13.InvertedViewportHeightFlipsYSupported,                    D3D12_FEATURE_D3D12_OPTIONS13);
            CREATE_FEATURE(device_info, "TextureCopyBetweenDimensionsSupported",                                feature_options13.TextureCopyBetweenDimensionsSupported,                    D3D12_FEATURE_D3D12_OPTIONS13);
            CREATE_FEATURE(device_info, "UnrestrictedBufferTextureCopyPitchSupported",                          feature_options13.UnrestrictedBufferTextureCopyPitchSupported,              D3D12_FEATURE_D3D12_OPTIONS13);
            CREATE_FEATURE(device_info, "AlphaBlendFactorSuppUnrestrictedVertexElementAlignmentSupportedorted", feature_options13.UnrestrictedVertexElementAlignmentSupported,              D3D12_FEATURE_D3D12_OPTIONS13);
        }

        device_info->device_d3d12 = d3d12_device.Get();
        device_info->device_d3d12_feature_level = device_feature_level;
    }

    int try_count = 0;
    bool create_d3d11_result = false;

    do {
        create_d3d11_result = SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
            d3d11_device_flags, &device_feature_levels[try_count], _countof(device_feature_levels) - try_count,
            D3D11_SDK_VERSION, &d3d11_device, &device_feature_level, &d3d11_device_context));
        
        if(create_d3d11_result) {
            D3D11_FEATURE_DATA_THREADING                        feature_threading;
            D3D11_FEATURE_DATA_DOUBLES                          feature_doubles;
            D3D11_FEATURE_DATA_FORMAT_SUPPORT                   feature_format_support;
            D3D11_FEATURE_DATA_FORMAT_SUPPORT2                  feature_format_support2;
            D3D11_FEATURE_DATA_D3D9_OPTIONS                     feature_d3d9_options;
            D3D11_FEATURE_DATA_D3D9_OPTIONS1                    feature_d3d9_options1;
            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS         feature_d3d10_options;
            D3D11_FEATURE_DATA_D3D11_OPTIONS                    feature_d3d11_options;
            D3D11_FEATURE_DATA_D3D11_OPTIONS1                   feature_d3d11_options1;
            D3D11_FEATURE_DATA_D3D11_OPTIONS2                   feature_d3d11_options2;
            D3D11_FEATURE_DATA_D3D11_OPTIONS3                   feature_d3d11_options3;
            D3D11_FEATURE_DATA_D3D11_OPTIONS4                   feature_d3d11_options4;
            D3D11_FEATURE_DATA_D3D11_OPTIONS5                   feature_d3d11_options5;
            D3D11_FEATURE_DATA_ARCHITECTURE_INFO                feature_arch_info;
            D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT     feature_shader_min;
            D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT              feature_d3d9_shadow;
            D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT   feature_d3d9_instancing;
            D3D11_FEATURE_DATA_MARKER_SUPPORT                   feature_marker_support;
            D3D11_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT      feature_virtual_address;
            D3D11_FEATURE_DATA_SHADER_CACHE                     feature_shader_cache;
            D3D11_FEATURE_DATA_DISPLAYABLE                      feature_displayable;

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &feature_threading, sizeof(feature_threading)))) {
                CREATE_FEATURE(device_info, "DriverCommandLists",                       feature_threading.DriverCommandLists,                           D3D11_FEATURE_THREADING);
                CREATE_FEATURE(device_info, "DriverConcurrentCreates",                  feature_threading.DriverConcurrentCreates,                      D3D11_FEATURE_THREADING);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &feature_doubles, sizeof(feature_doubles)))) {
                CREATE_FEATURE(device_info, "DoublePrecisionFloatShaderOps",            feature_doubles.DoublePrecisionFloatShaderOps,                  D3D11_FEATURE_DOUBLES);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT, &feature_format_support, sizeof(feature_format_support)))) {
                CREATE_FEATURE(device_info, "InFormat",                                 feature_format_support.InFormat,                                D3D11_FEATURE_FORMAT_SUPPORT);
                CREATE_FEATURE(device_info, "OutFormatSupport",                         feature_format_support.OutFormatSupport,                        D3D11_FEATURE_FORMAT_SUPPORT);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &feature_format_support2, sizeof(feature_format_support2)))) {
                CREATE_FEATURE(device_info, "InFormat",                                 feature_format_support2.InFormat,                               D3D11_FEATURE_FORMAT_SUPPORT2);
                CREATE_FEATURE(device_info, "OutFormatSupport2",                        feature_format_support2.OutFormatSupport2,                      D3D11_FEATURE_FORMAT_SUPPORT2);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &feature_d3d10_options, sizeof(feature_d3d10_options)))) {
                CREATE_FEATURE(device_info, "ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x", feature_d3d10_options.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x, D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &feature_d3d11_options, sizeof(feature_d3d11_options)))) {
                CREATE_FEATURE(device_info, "ClearView",                                feature_d3d11_options.ClearView,                                D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "ConstantBufferOffsetting",                 feature_d3d11_options.ConstantBufferOffsetting,                 D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "ConstantBufferPartialUpdate",              feature_d3d11_options.ConstantBufferPartialUpdate,              D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "CopyWithOverlap",                          feature_d3d11_options.CopyWithOverlap,                          D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "DiscardAPIsSeenByDriver",                  feature_d3d11_options.DiscardAPIsSeenByDriver,                  D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "ExtendedDoublesShaderInstructions",        feature_d3d11_options.ExtendedDoublesShaderInstructions,        D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "ExtendedResourceSharing",                  feature_d3d11_options.ExtendedResourceSharing,                  D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "FlagsForUpdateAndCopySeenByDriver",        feature_d3d11_options.FlagsForUpdateAndCopySeenByDriver,        D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "MapNoOverwriteOnDynamicBufferSRV",         feature_d3d11_options.MapNoOverwriteOnDynamicBufferSRV,         D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "MapNoOverwriteOnDynamicConstantBuffer",    feature_d3d11_options.MapNoOverwriteOnDynamicConstantBuffer,    D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "MultisampleRTVWithForcedSampleCountOne",   feature_d3d11_options.MultisampleRTVWithForcedSampleCountOne,   D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "OutputMergerLogicOp",                      feature_d3d11_options.OutputMergerLogicOp,                      D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "SAD4ShaderInstructions",                   feature_d3d11_options.SAD4ShaderInstructions,                   D3D11_FEATURE_D3D11_OPTIONS);
                CREATE_FEATURE(device_info, "UAVOnlyRenderingForcedSampleCount",        feature_d3d11_options.UAVOnlyRenderingForcedSampleCount,        D3D11_FEATURE_D3D11_OPTIONS);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &feature_d3d11_options1, sizeof(feature_d3d11_options1)))) {
                CREATE_FEATURE(device_info, "ClearViewAlsoSupportsDepthOnlyFormats",    feature_d3d11_options1.ClearViewAlsoSupportsDepthOnlyFormats,   D3D11_FEATURE_D3D11_OPTIONS1);
                CREATE_FEATURE(device_info, "MapOnDefaultBuffers",                      feature_d3d11_options1.MapOnDefaultBuffers,                     D3D11_FEATURE_D3D11_OPTIONS1);
                CREATE_FEATURE(device_info, "MinMaxFiltering",                          feature_d3d11_options1.MinMaxFiltering,                         D3D11_FEATURE_D3D11_OPTIONS1);
                CREATE_FEATURE(device_info, "TiledResourcesTier",                       feature_d3d11_options1.TiledResourcesTier,                      D3D11_FEATURE_D3D11_OPTIONS1);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &feature_d3d11_options2, sizeof(feature_d3d11_options2)))) {
                CREATE_FEATURE(device_info, "ConservativeRasterizationTier",            feature_d3d11_options2.ConservativeRasterizationTier,           D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "MapOnDefaultTextures",                     feature_d3d11_options2.MapOnDefaultTextures,                    D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "PSSpecifiedStencilRefSupported",           feature_d3d11_options2.PSSpecifiedStencilRefSupported,          D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "ROVsSupported",                            feature_d3d11_options2.ROVsSupported,                           D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "StandardSwizzle",                          feature_d3d11_options2.StandardSwizzle,                         D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "TiledResourcesTier",                       feature_d3d11_options2.TiledResourcesTier,                      D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "TypedUAVLoadAdditionalFormats",            feature_d3d11_options2.TypedUAVLoadAdditionalFormats,           D3D11_FEATURE_D3D11_OPTIONS2);
                CREATE_FEATURE(device_info, "UnifiedMemoryArchitecture",                feature_d3d11_options2.UnifiedMemoryArchitecture,               D3D11_FEATURE_D3D11_OPTIONS2);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &feature_d3d11_options3, sizeof(feature_d3d11_options3)))) {
                CREATE_FEATURE(device_info, "VPAndRTArrayIndexFromAnyShaderFeedingRasterizer", feature_d3d11_options3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer, D3D11_FEATURE_D3D11_OPTIONS3);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &feature_d3d11_options4, sizeof(feature_d3d11_options4)))) {
                CREATE_FEATURE(device_info, "ExtendedNV12SharedTextureSupported",       feature_d3d11_options4.ExtendedNV12SharedTextureSupported,      D3D11_FEATURE_D3D11_OPTIONS4);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &feature_d3d11_options5, sizeof(feature_d3d11_options5)))) {
                CREATE_FEATURE(device_info, "SharedResourceTier",                       feature_d3d11_options5.SharedResourceTier,                      D3D11_FEATURE_D3D11_OPTIONS5);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_ARCHITECTURE_INFO, &feature_arch_info, sizeof(feature_arch_info)))) {
                CREATE_FEATURE(device_info, "TileBasedDeferredRenderer",                feature_arch_info.TileBasedDeferredRenderer,                    D3D11_FEATURE_ARCHITECTURE_INFO);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D9_OPTIONS, &feature_d3d9_options, sizeof(feature_d3d9_options)))) {
                CREATE_FEATURE(device_info, "FullNonPow2TextureSupport",                feature_d3d9_options.FullNonPow2TextureSupport,                 D3D11_FEATURE_D3D9_OPTIONS);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &feature_shader_min, sizeof(feature_shader_min)))) {
                CREATE_FEATURE(device_info, "AllOtherShaderStagesMinPrecision",         feature_shader_min.AllOtherShaderStagesMinPrecision,            D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT);
                CREATE_FEATURE(device_info, "PixelShaderMinPrecision",                  feature_shader_min.PixelShaderMinPrecision,                     D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D9_SHADOW_SUPPORT, &feature_d3d9_shadow, sizeof(feature_d3d9_shadow)))) {
                CREATE_FEATURE(device_info, "SupportsDepthAsTextureWithLessEqualComparisonFilter", feature_d3d9_shadow.SupportsDepthAsTextureWithLessEqualComparisonFilter, D3D11_FEATURE_D3D9_SHADOW_SUPPORT);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT, &feature_d3d9_instancing, sizeof(feature_d3d9_instancing)))) {
                CREATE_FEATURE(device_info, "SimpleInstancingSupported",                feature_d3d9_instancing.SimpleInstancingSupported,              D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_MARKER_SUPPORT, &feature_marker_support, sizeof(feature_marker_support)))) {
                CREATE_FEATURE(device_info, "Profile",                                  feature_marker_support.Profile,                                 D3D11_FEATURE_MARKER_SUPPORT);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_D3D9_OPTIONS1, &feature_d3d9_options1, sizeof(feature_d3d9_options1)))) {
                CREATE_FEATURE(device_info, "DepthAsTextureWithLessEqualComparisonFilterSupported", feature_d3d9_options1.DepthAsTextureWithLessEqualComparisonFilterSupported, D3D11_FEATURE_D3D9_OPTIONS1);
                CREATE_FEATURE(device_info, "FullNonPow2TextureSupported",              feature_d3d9_options1.FullNonPow2TextureSupported,              D3D11_FEATURE_D3D9_OPTIONS1);
                CREATE_FEATURE(device_info, "SimpleInstancingSupported",                feature_d3d9_options1.SimpleInstancingSupported,                D3D11_FEATURE_D3D9_OPTIONS1);
                CREATE_FEATURE(device_info, "TextureCubeFaceRenderTargetWithNonCubeDepthStencilSupported", feature_d3d9_options1.TextureCubeFaceRenderTargetWithNonCubeDepthStencilSupported, D3D11_FEATURE_D3D9_OPTIONS1);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &feature_virtual_address, sizeof(feature_virtual_address)))) {
                CREATE_FEATURE(device_info, "MaxGPUVirtualAddressBitsPerProcess",       feature_virtual_address.MaxGPUVirtualAddressBitsPerProcess,     D3D11_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT);
                CREATE_FEATURE(device_info, "MaxGPUVirtualAddressBitsPerResource",      feature_virtual_address.MaxGPUVirtualAddressBitsPerResource,    D3D11_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_SHADER_CACHE, &feature_shader_cache, sizeof(feature_shader_cache)))) {
                CREATE_FEATURE(device_info, "SupportFlags",                             feature_shader_cache.SupportFlags,                              D3D11_FEATURE_SHADER_CACHE);
            }

            if (SUCCEEDED(d3d11_device->CheckFeatureSupport(D3D11_FEATURE_DISPLAYABLE, &feature_displayable, sizeof(feature_displayable)))) {
                CREATE_FEATURE(device_info, "DisplayableTexture",                       feature_displayable.DisplayableTexture,                         D3D11_FEATURE_DISPLAYABLE);
                CREATE_FEATURE(device_info, "SharedResourceTier",                       feature_displayable.SharedResourceTier,                         D3D11_FEATURE_DISPLAYABLE);
            }

            device_feature_level = device_feature_levels[try_count];
            device_info->device_d3d11 = d3d11_device.Get();
            device_info->device_d3d11_feature_level = device_feature_level;
            break;
        }        
    } while (!create_d3d11_result);

    return device_info;
}

_Use_decl_annotations_
std::list<GPU_DEVICE_INFO*> QueryAdapters(bool requestHighPerformanceAdapter)
{
    UINT factory_flags = 0;
    UINT adapter_index = 0;
    bool supports_gpu_pref = false;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory4> factory4;
    ComPtr<IDXGIFactory6> factory6;
    ComPtr<ID3D12Debug> debugController;
    std::list<GPU_DEVICE_INFO*> devices;

#if defined(_DEBUG)
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));
    supports_gpu_pref = SUCCEEDED(factory4->QueryInterface(IID_PPV_ARGS(&factory6)));

    for (;;++adapter_index) {
        HRESULT enumAdapterResult;

        if (supports_gpu_pref) {
            if (requestHighPerformanceAdapter) {
                enumAdapterResult = factory6->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
            }
            else {
                enumAdapterResult = factory6->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&adapter));
            }
        }
        else {
            enumAdapterResult = factory4->EnumAdapters1(adapter_index, &adapter);
        }

        if (enumAdapterResult == DXGI_ERROR_NOT_FOUND)
            break;

        devices.push_back(GetDeviceInfo(adapter.Get(), adapter_index));
    }

    return devices;
}

std::string GPUFeatureLevelToString(D3D_FEATURE_LEVEL featureLevel) {
    switch (featureLevel) {
    case D3D_FEATURE_LEVEL_12_1:
        return std::string("D3D_FEATURE_LEVEL_12_1");
    case D3D_FEATURE_LEVEL_12_0:
        return std::string("D3D_FEATURE_LEVEL_12_0");
    case D3D_FEATURE_LEVEL_11_1:
        return std::string("D3D_FEATURE_LEVEL_11_1");
    case D3D_FEATURE_LEVEL_11_0:
        return std::string("D3D_FEATURE_LEVEL_11_0");
    case D3D_FEATURE_LEVEL_10_1:
        return std::string("D3D_FEATURE_LEVEL_10_1");
    case D3D_FEATURE_LEVEL_10_0:
        return std::string("D3D_FEATURE_LEVEL_10_0");
    case D3D_FEATURE_LEVEL_9_3:
        return std::string("D3D_FEATURE_LEVEL_9_3");
    case D3D_FEATURE_LEVEL_9_2:
        return std::string("D3D_FEATURE_LEVEL_9_2");
    case D3D_FEATURE_LEVEL_9_1:
        return std::string("D3D_FEATURE_LEVEL_9_1");
    }

    return std::string("");
}

void GetGPUInfo()
{
    AllocConsole();

    FILE* fp = nullptr;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    auto l = QueryAdapters(true);

    for (auto gpu : l) {
        wprintf(L"(%d) %s [%d]\n", gpu->adapter_index,  gpu->adapter_name.c_str(), gpu->device_id);
        wprintf(L"Video Mem: %d\nDedicated Mem: %d\nShared Mem: %d\n", gpu->adapter_video_memory, gpu->adapter_dedicated_memory, gpu->adapter_shared_memory);
        wprintf(L"D3D12 Feature Level: %s\n", GPUFeatureLevelToString(gpu->device_d3d12_feature_level).c_str());
        wprintf(L"D3D11 Feature Level: %s\n", GPUFeatureLevelToString(gpu->device_d3d11_feature_level).c_str());
        wprintf(L"Is Warp Device: %s\n", gpu->adapter_is_warp ? L"True" : L"False");
        for (auto feature : gpu->features)
        {
            printf("%-75s: %8d (%s)\n", feature->feature_name.c_str(), feature->feature_value, feature->is_d3d12 ? "D3D12" : "D3D11");
        }
        wprintf(L"\n");
    }
}

std::string DisplayMem(long long bytes)
{
    std::string display;
    int GB = static_cast<int>(bytes / pow(1024,3));
    if (GB < 1)
        return std::to_string(static_cast<int>(bytes / pow(1024, 2))) + " MB";
    return std::to_string(GB) + " GB";
}