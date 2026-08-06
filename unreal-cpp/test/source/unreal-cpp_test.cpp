// Tests against the real Unreal Engine TArray (see UE_ROOT in the top-level
// CMakeLists.txt), linked via the ue_core_minimal stub library.
#include "Containers/Array.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TArray starts empty and grows on Add", "[tarray]")
{
	TArray<int32> Numbers;
	REQUIRE(Numbers.Num() == 0);
	REQUIRE(Numbers.Max() == 0);

	Numbers.Add(1);
	Numbers.Add(2);
	Numbers.Add(3);

	REQUIRE(Numbers.Num() == 3);
	REQUIRE(Numbers.Max() >= 3);
	REQUIRE(Numbers[0] == 1);
	REQUIRE(Numbers[2] == 3);
}

TEST_CASE("TArray is 16 bytes regardless of element type", "[tarray]")
{
	STATIC_REQUIRE(sizeof(TArray<int32>) == 16);
	STATIC_REQUIRE(sizeof(TArray<double>) == 16);
}

TEST_CASE("Reserve sets capacity without changing Num", "[tarray]")
{
	TArray<int32> Numbers;
	Numbers.Reserve(100);
	REQUIRE(Numbers.Num() == 0);
	REQUIRE(Numbers.Max() == 100);

	Numbers.Add(7);
	Numbers.Shrink();
	REQUIRE(Numbers.Num() == 1);
	REQUIRE(Numbers.Max() == 1);
}

TEST_CASE("RemoveAt keeps order, RemoveAtSwap does not", "[tarray]")
{
	const TArray<int32> Source = {10, 20, 30, 40, 50};

	TArray<int32> Ordered = Source;
	Ordered.RemoveAt(1);
	REQUIRE(Ordered == TArray<int32>{10, 30, 40, 50});

	TArray<int32> Swapped = Source;
	Swapped.RemoveAtSwap(1);
	REQUIRE(Swapped == TArray<int32>{10, 50, 30, 40});
}

TEST_CASE("Find and Contains locate elements", "[tarray]")
{
	const TArray<int32> Numbers = {5, 10, 15};

	REQUIRE(Numbers.Contains(10));
	REQUIRE_FALSE(Numbers.Contains(11));
	REQUIRE(Numbers.Find(15) == 2);
	REQUIRE(Numbers.Find(999) == INDEX_NONE);
	REQUIRE(Numbers.IndexOfByPredicate([](int32 V) { return V > 5; }) == 1);
}

TEST_CASE("Copies are deep", "[tarray]")
{
	TArray<int32> Original = {1, 2, 3};
	TArray<int32> Copy = Original;

	Copy[0] = 99;
	REQUIRE(Original[0] == 1);
	REQUIRE(Original.GetData() != Copy.GetData());
}
