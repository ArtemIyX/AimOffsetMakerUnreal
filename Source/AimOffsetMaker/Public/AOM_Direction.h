// Developed by Aberration Team (by Wellsaik)

#pragma once

#include "CoreMinimal.h"

#include "AOM_Direction.generated.h"

UENUM(BlueprintType)
enum class EAOM_Direction : uint8
{
	CC = 0,
	CU = 1,
	UR = 2,
	URB = 3,
	UL = 4,
	ULB = 5,
	CR = 6,
	CRB = 7,
	CL = 8,
	CLB = 9,
	CD = 10,
	DR = 11,
	DRB = 12,
	DL = 13,
	DLB = 14
};
