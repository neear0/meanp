#pragma once

#include <Windows.h>
#include <winternl.h>

auto constexpr dll_notification_reason_loaded = 1;
auto constexpr dll_notification_reason_unloaded = 2;

typedef struct dll_loaded_notification_data_t
{
    ULONG flags;
    PCUNICODE_STRING full_dll_name;
    PCUNICODE_STRING base_dll_name;
    PVOID dll_base;
    ULONG size_of_image;
} dll_loaded_notification_data, * dll_loaded_notification_data_ptr;

typedef struct dll_unloaded_notification_data_t
{
    ULONG flags;
    PCUNICODE_STRING full_dll_name;
    PCUNICODE_STRING base_dll_name;
    PVOID dll_base;
    ULONG size_of_image;
} dll_unloaded_notification_data, * dll_unloaded_notification_data_ptr;

typedef union dll_notification_data_u
{
    dll_loaded_notification_data loaded;
    dll_unloaded_notification_data unloaded;
} dll_notification_data, * dll_notification_data_ptr;

typedef const dll_notification_data_u* type_dll_notification_data;

typedef void(__stdcall* dll_notification_function)
(
    ULONG notification_reason,
    type_dll_notification_data notification_data,
    PVOID context
    );

long __stdcall register_dll_notification
(
	ULONG flags,
	dll_notification_function notification_function,
	PVOID context,
	PVOID* cookie
);

long __stdcall unregister_dll_notification(
	PVOID cookie
);
