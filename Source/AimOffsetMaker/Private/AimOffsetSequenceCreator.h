#pragma once

#include "CoreMinimal.h"

#include "AOM_Direction.h"

class UAnimSequence;
class UObject;
class USkeletalMesh;
class USkeleton;
class UAimOffsetBlendSpace;
struct FReferenceSkeleton;
class FAsyncTaskNotification;

struct FAimOffsetSequenceCreateParams
{
	TWeakObjectPtr<UAnimSequence> SourceSequence;
	FString OutputFolder;
	FString OutputBaseName;
	TMap<EAOM_Direction, int32> DirectionFrames;
	bool bCompressAnimations = false;
	bool bCreateAimOffset = true;
	FString AimOffsetAssetName;
	bool bSetupAimOffset = true;
	bool bUseAdditiveSettings = false;
	bool bUseAdditiveAnimation = false;
	TWeakObjectPtr<UAnimSequence> AdditiveBasePoseAnimation;
};

struct FAimOffsetSequenceCreateResult
{
	bool bSuccess = false;
	bool bHasValidationErrors = false;
	int32 CreatedCount = 0;
	TArray<FString> Errors;
	TArray<FString> FailedAssets;
	TArray<UObject*> CreatedAssets;
};

class FAimOffsetSequenceCreator final
{
public:
	FAimOffsetSequenceCreateResult Execute(const FAimOffsetSequenceCreateParams& InParams) const;

	static const TArray<EAOM_Direction>& GetOrderedDirections();
	static FString GetDirectionName(EAOM_Direction InDirection);

private:
	void ValidateSourceSequence(const FAimOffsetSequenceCreateParams& InParams, FAimOffsetSequenceCreateResult& OutResult) const;
	void ValidateOutputSettings(const FAimOffsetSequenceCreateParams& InParams, FAimOffsetSequenceCreateResult& OutResult) const;
	void ValidateFrameSettings(const FAimOffsetSequenceCreateParams& InParams, FAimOffsetSequenceCreateResult& OutResult) const;
	FAimOffsetSequenceCreateResult Validate(const FAimOffsetSequenceCreateParams& InParams) const;
	bool BuildAnimationSequence(
		const FAimOffsetSequenceCreateParams& InParams,
		UAnimSequence* InSourceSequence,
		USkeletalMesh* InPreviewMesh,
		USkeleton* InSkeleton,
		const FReferenceSkeleton& InReferenceSkeleton,
		EAOM_Direction InDirection,
		int32 InFrameIndex,
		UAnimSequence*& OutCreatedSequence) const;
	void ApplyAdditiveSettings(
		const FAimOffsetSequenceCreateParams& InParams,
		const TMap<EAOM_Direction, UAnimSequence*>& InCreatedSequencesByDirection) const;
	void ApplyCompressionSettings(
		const FAimOffsetSequenceCreateParams& InParams,
		const TMap<EAOM_Direction, UAnimSequence*>& InCreatedSequencesByDirection) const;
	UAimOffsetBlendSpace* CreateAimOffsetAsset(
		const FAimOffsetSequenceCreateParams& InParams,
		USkeleton* InSkeleton,
		USkeletalMesh* InPreviewMesh) const;
	void SetupAimOffsetAsset(
		const FAimOffsetSequenceCreateParams& InParams,
		UAimOffsetBlendSpace* InAimOffset,
		const TMap<EAOM_Direction, UAnimSequence*>& InCreatedSequencesByDirection) const;
	FVector GetAimOffsetSamplePosition(EAOM_Direction InDirection) const;
	void FinalizeNotification(const FAimOffsetSequenceCreateResult& InResult) const;
};
