#pragma once

#include "types.hpp"

static const constexpr dword BUFFER_SIZE      = 4096u;

static const constexpr dword SYNC_MAGIC       = 0x434E5953ull;
static const constexpr dword SACK_MAGIC       = 0x4B434153ull;

static const constexpr dword CMD_SOFT_INFO    = 0x44494653ull;
static const constexpr dword CMD_ERASE_SECTOR = 0x52454553ull;
static const constexpr dword CMD_ERASE_CHIP   = 0x52454843ull;
static const constexpr dword CMD_READ_BYTES   = 0x44414552ull;
static const constexpr dword CMD_WRITE_BYTES  = 0x54495257ull;
