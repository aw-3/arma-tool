#pragma once

#include "../blackbone/src/BlackBone/Process/Process.h"
#include "../blackbone/src/BlackBone/Process/RPC/RemoteFunction.hpp"
#include "../blackbone/src/BlackBone/Misc/Utils.h"

int Read(blackbone::Process &process, blackbone::ptr_t addr, blackbone::ptr_t size, PVOID buff);
int Write(blackbone::Process &process, blackbone::ptr_t addr, blackbone::ptr_t size, PVOID buff);
blackbone::ptr_t DoAllocate(blackbone::Process &process, blackbone::ptr_t size, DWORD protection);
