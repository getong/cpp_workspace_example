// Minimal stand-ins for the CORE_API symbols that UE's Core module normally
// provides, just enough to link TArray outside UnrealBuildTool.
#include "CoreTypes.h"
#include "HAL/MemoryBase.h"
#include "HAL/UnrealMemory.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Exec.h"
#include "Misc/OutputDevice.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

class FMallocAnsiLite final : public FMalloc
{
public:
	void* Malloc(SIZE_T Count, uint32 Alignment) override
	{
		void* Ptr = nullptr;
		posix_memalign(&Ptr, Alignment < 16 ? 16 : Alignment, Count ? Count : 1);
		return Ptr;
	}
	void* Realloc(void* Original, SIZE_T Count, uint32 Alignment) override
	{
		if (Count == 0) { ::free(Original); return nullptr; }
		return ::realloc(Original, Count);
	}
	void Free(void* Original) override { ::free(Original); }
};

namespace UE::Private { FMalloc* GMalloc = nullptr; }

void FMemory::GCreateMalloc()
{
	static FMallocAnsiLite Instance;
	UE::Private::GMalloc = &Instance;
}

SIZE_T FMemory::QuantizeSize(SIZE_T Count, uint32 /*Alignment*/) { return Count; }

void* FMalloc::TryMalloc(SIZE_T Count, uint32 Alignment) { return Malloc(Count, Alignment); }
void* FMalloc::TryRealloc(void* Original, SIZE_T Count, uint32 Alignment) { return Realloc(Original, Count, Alignment); }
void* FMalloc::MallocZeroed(SIZE_T Count, uint32 Alignment)
{
	void* Ptr = Malloc(Count, Alignment);
	if (Ptr) { memset(Ptr, 0, Count); }
	return Ptr;
}
void* FMalloc::TryMallocZeroed(SIZE_T Count, uint32 Alignment) { return MallocZeroed(Count, Alignment); }
void FMalloc::GetAllocatorStats(FGenericMemoryStats&) {}
void FMalloc::InitializeStatsMetadata() {}
void FMalloc::UpdateStats() {}

FExec::~FExec() {}

void* FUseSystemMallocForNew::operator new(size_t Size) { return ::malloc(Size); }
void FUseSystemMallocForNew::operator delete(void* Ptr) { ::free(Ptr); }
void* FUseSystemMallocForNew::operator new[](size_t Size) { return ::malloc(Size); }
void FUseSystemMallocForNew::operator delete[](void* Ptr) { ::free(Ptr); }

void FOutputDevice::LogfImpl(const TCHAR*, ...) {}

bool FDebug::CheckVerifyFailedImpl2(const ANSICHAR* Expr, const ANSICHAR* File, int32 Line, const TCHAR*, ...)
{
	fprintf(stderr, "UE check() failed: %s at %s:%d\n", Expr, File, Line);
	abort();
}

namespace UE::Core::Private
{
	[[noreturn]] void OnInvalidArrayNum(unsigned long long NewNum)
	{
		fprintf(stderr, "Invalid TArray Num: %llu\n", NewNum);
		abort();
	}
	[[noreturn]] void OnInvalidSizedHeapAllocatorNum(int32 IndexSize, int64 NewNum, SIZE_T)
	{
		fprintf(stderr, "Invalid allocator Num (IndexSize=%d): %lld\n", IndexSize, NewNum);
		abort();
	}
}

// UE compiles these into Core via explicit instantiation to reduce code bloat;
// outside UBT we have to instantiate them ourselves.
template void TSizedHeapAllocator<32, FMemory>::ForAnyElementType::ResizeAllocation(int32, int32, SIZE_T);
template void TSizedHeapAllocator<32, FMemory>::ForAnyElementType::ResizeAllocation(int32, int32, SIZE_T, uint32);

bool FExec::Exec(UWorld*, const TCHAR*, FOutputDevice&) { return false; }

void* FMemory::Malloc(SIZE_T Count, uint32 Alignment)
{
	if (!UE::Private::GMalloc) { GCreateMalloc(); }
	return UE::Private::GMalloc->Malloc(Count, Alignment);
}
void* FMemory::Realloc(void* Original, SIZE_T Count, uint32 Alignment)
{
	if (!UE::Private::GMalloc) { GCreateMalloc(); }
	return UE::Private::GMalloc->Realloc(Original, Count, Alignment);
}
void FMemory::Free(void* Original)
{
	if (!UE::Private::GMalloc) { GCreateMalloc(); }
	UE::Private::GMalloc->Free(Original);
}

namespace UE::Assert::Private
{
	bool CheckEnsureFailed(bool bAlways, std::atomic<uint8>& bExecuted, const ANSICHAR*, int32, const ANSICHAR*)
	{
		return bAlways || bExecuted.exchange(1) == 0;
	}
	bool CheckEnsureFailed(bool bAlways, const std::atomic<uint8>& bExecuted)
	{
		return bAlways || bExecuted.load() == 0;
	}
}

namespace UE::Assert::Private
{
	bool EnsureFailed(std::atomic<uint8>&, const FStaticEnsureRecord* Ensure, ...)
	{
		fprintf(stderr, "UE ensure() failed: %s\n", Ensure ? Ensure->Expression : "<unknown>");
		return false;
	}
}
