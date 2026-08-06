// Experiment with the REAL Unreal Engine 5.8 TArray, compiled outside
// UnrealBuildTool. The headers come straight from the engine install
// (see UE_ROOT in CMakeLists.txt); ue_core_stub.cpp supplies the handful
// of Core-module symbols the container needs at link time.
#include "Containers/Array.h"

#include <cstdio>

struct FVecLike
{
	float X = 0;
	float Y = 0;
	float Z = 0;

	FVecLike() = default;
	FVecLike(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
};

int main()
{
	printf("=== Unreal Engine 5.8 TArray demo (real engine headers) ===\n\n");

	// --- Memory layout -----------------------------------------------------
	// TArray is { ElementType* Data; int32 ArrayNum; int32 ArrayMax; }
	printf("sizeof(TArray<int32>)  = %zu bytes (pointer + int32 Num + int32 Max)\n",
	       sizeof(TArray<int32>));
	printf("sizeof(TArray<double>) = %zu bytes (layout independent of element type)\n\n",
	       sizeof(TArray<double>));

	// --- Growth policy (slack) --------------------------------------------
	TArray<int32> Numbers;
	printf("Growth as elements are added (Num vs Max shows the slack policy):\n");
	int32 LastMax = -1;
	for (int32 i = 0; i < 100; ++i)
	{
		Numbers.Add(i);
		if (Numbers.Max() != LastMax)
		{
			LastMax = Numbers.Max();
			printf("  Num=%3d  Max=%3d  AllocatedSize=%4d bytes  Data=%p\n",
			       Numbers.Num(), Numbers.Max(),
			       static_cast<int32>(Numbers.GetAllocatedSize()),
			       static_cast<void*>(Numbers.GetData()));
		}
	}
	printf("\n");

	// --- Slack control -----------------------------------------------------
	TArray<int32> Reserved;
	Reserved.Reserve(1000);
	printf("After Reserve(1000):   Num=%d Max=%d\n", Reserved.Num(), Reserved.Max());
	Reserved.Add(1);
	Reserved.Shrink();
	printf("After Add + Shrink():  Num=%d Max=%d\n\n", Reserved.Num(), Reserved.Max());

	// --- RemoveAt vs RemoveAtSwap -----------------------------------------
	TArray<int32> A = {10, 20, 30, 40, 50};
	TArray<int32> B = A;  // deep copy, unlike raw pointers

	A.RemoveAt(1);      // ordered: shifts everything after index 1 (O(n))
	B.RemoveAtSwap(1);  // unordered: moves last element into the hole (O(1))

	printf("RemoveAt(1):     ");
	for (int32 V : A) { printf("%d ", V); }
	printf("   (order kept)\nRemoveAtSwap(1): ");
	for (int32 V : B) { printf("%d ", V); }
	printf("   (last element swapped in)\n\n");

	// --- Search / query API ------------------------------------------------
	printf("A.Contains(30)   = %s\n", A.Contains(30) ? "true" : "false");
	printf("A.Find(40)       = index %d\n", A.Find(40));
	printf("A.IndexOfByPredicate(>30) = index %d\n\n",
	       A.IndexOfByPredicate([](int32 V) { return V > 30; }));

	// --- Emplace constructs in place --------------------------------------
	TArray<FVecLike> Vectors;
	Vectors.Emplace(1.0f, 2.0f, 3.0f);
	Vectors.Emplace(4.0f, 5.0f, 6.0f);
	printf("Vectors.Num()=%d, Vectors[1] = (%.1f, %.1f, %.1f)\n",
	       Vectors.Num(), Vectors[1].X, Vectors[1].Y, Vectors[1].Z);

	// --- Bounds checking ---------------------------------------------------
	// Vectors[5] would trip UE's check() (RangeCheck) and abort — try it!
	printf("\nDone. TArray internals: Data=%p Num=%d Max=%d\n",
	       static_cast<void*>(Vectors.GetData()), Vectors.Num(), Vectors.Max());
	return 0;
}
