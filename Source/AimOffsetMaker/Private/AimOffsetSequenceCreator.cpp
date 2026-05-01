#include "AimOffsetSequenceCreator.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AnimEnums.h"
#include "AnimPose.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationSettings.h"
#include "Animation/AnimTypes.h"
#include "Animation/Skeleton.h"
#include "ContentBrowserModule.h"
#include "Factories/AimOffsetBlendSpaceFactoryNew.h"
#include "Factories/AnimSequenceFactory.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IContentBrowserSingleton.h"
#include "Misc/AsyncTaskNotification.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FAimOffsetSequenceCreator"

namespace AimOffsetSequenceCreator
{
	static bool SetBlendParameter(UAimOffsetBlendSpace* InAimOffset, int32 InAxisIndex, const FBlendParameter& InValue)
	{
		if (!InAimOffset || InAxisIndex < 0)
		{
			return false;
		}

		const FStructProperty* property = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("BlendParameters"));
		if (!property || !property->Struct || property->Struct != FBlendParameter::StaticStruct() || InAxisIndex >= property->ArrayDim)
		{
			return false;
		}

		void* valuePtr = property->ContainerPtrToValuePtr<void>(InAimOffset);
		property->Struct->CopyScriptStruct(static_cast<uint8*>(valuePtr) + (property->GetElementSize() * InAxisIndex), &InValue);
		return true;
	}

	static bool SetInterpolationParameter(UAimOffsetBlendSpace* InAimOffset, int32 InAxisIndex, const FInterpolationParameter& InValue)
	{
		if (!InAimOffset || InAxisIndex < 0)
		{
			return false;
		}

		const FStructProperty* property = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("InterpolationParam"));
		if (!property || !property->Struct || property->Struct != FInterpolationParameter::StaticStruct() || InAxisIndex >= property->ArrayDim)
		{
			return false;
		}

		void* valuePtr = property->ContainerPtrToValuePtr<void>(InAimOffset);
		property->Struct->CopyScriptStruct(static_cast<uint8*>(valuePtr) + (property->GetElementSize() * InAxisIndex), &InValue);
		return true;
	}

	static bool SetTargetWeightInterpolationSpeed(UAimOffsetBlendSpace* InAimOffset, float InValue)
	{
		if (!InAimOffset)
		{
			return false;
		}

		const FFloatProperty* property = FindFProperty<FFloatProperty>(UBlendSpace::StaticClass(), TEXT("TargetWeightInterpolationSpeedPerSec"));
		if (!property)
		{
			return false;
		}

		property->SetPropertyValue_InContainer(InAimOffset, InValue);
		return true;
	}

	static USkeletalMesh* ResolvePreviewMesh(UAnimSequence* InSequence)
	{
		if (!InSequence)
		{
			return nullptr;
		}

		if (USkeletalMesh* previewMesh = InSequence->GetPreviewMesh())
		{
			return previewMesh;
		}

		USkeleton* skeleton = InSequence->GetSkeleton();
		if (!skeleton)
		{
			return nullptr;
		}

		if (USkeletalMesh* previewMesh = skeleton->GetAssetPreviewMesh(InSequence))
		{
			return previewMesh;
		}

		return skeleton->FindCompatibleMesh();
	}
}

const TArray<EAOM_Direction>& FAimOffsetSequenceCreator::GetOrderedDirections()
{
	static const TArray<EAOM_Direction> directions = {
		EAOM_Direction::CC,
		EAOM_Direction::CU,
		EAOM_Direction::CD,
		EAOM_Direction::UR,
		EAOM_Direction::URB,
		EAOM_Direction::UL,
		EAOM_Direction::ULB,
		EAOM_Direction::CR,
		EAOM_Direction::CRB,
		EAOM_Direction::CL,
		EAOM_Direction::CLB,
		EAOM_Direction::DR,
		EAOM_Direction::DRB,
		EAOM_Direction::DL,
		EAOM_Direction::DLB
	};

	return directions;
}

FString FAimOffsetSequenceCreator::GetDirectionName(EAOM_Direction InDirection)
{
	if (const UEnum* directionEnum = StaticEnum<EAOM_Direction>())
	{
		return directionEnum->GetNameStringByValue(static_cast<int64>(InDirection));
	}

	return TEXT("Invalid");
}

void FAimOffsetSequenceCreator::ValidateSourceSequence(const FAimOffsetSequenceCreateParams& InParams, FAimOffsetSequenceCreateResult& OutResult) const
{
	UAnimSequence* sourceSequence = InParams.SourceSequence.Get();
	if (!sourceSequence)
	{
		OutResult.Errors.Add(TEXT("Original anim sequence is not selected."));
		return;
	}

	const int32 maxFrameIndex = sourceSequence->GetNumberOfSampledKeys() - 1;
	if (maxFrameIndex < 0)
	{
		OutResult.Errors.Add(TEXT("Original anim sequence has no valid sampled frames."));
	}

	if (!AimOffsetSequenceCreator::ResolvePreviewMesh(sourceSequence))
	{
		OutResult.Errors.Add(TEXT("Original anim sequence has no valid preview mesh for pose extraction."));
	}

	if (!sourceSequence->GetSkeleton())
	{
		OutResult.Errors.Add(TEXT("Original anim sequence has no valid skeleton."));
	}
}

void FAimOffsetSequenceCreator::ValidateOutputSettings(const FAimOffsetSequenceCreateParams& InParams, FAimOffsetSequenceCreateResult& OutResult) const
{
	if (InParams.OutputBaseName.IsEmpty())
	{
		OutResult.Errors.Add(TEXT("Base name is not set."));
	}

	if (InParams.bCreateAimOffset && InParams.AimOffsetAssetName.IsEmpty())
	{
		OutResult.Errors.Add(TEXT("Aim Offset Asset Name is not set."));
	}

	if (InParams.bUseAdditiveSettings && InParams.bUseAdditiveAnimation && !InParams.AdditiveBasePoseAnimation.IsValid())
	{
		OutResult.Errors.Add(TEXT("Additive Settings uses external animation, but Base Pose Animation is not set."));
	}

	if (InParams.OutputFolder.IsEmpty())
	{
		OutResult.Errors.Add(TEXT("Folder is not set."));
	}
	else if (!FPackageName::IsValidLongPackageName(InParams.OutputFolder))
	{
		OutResult.Errors.Add(FString::Printf(TEXT("Folder '%s' is not a valid project path."), *InParams.OutputFolder));
	}
	else
	{
		FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& assetRegistry = assetRegistryModule.Get();
		assetRegistry.ScanPathsSynchronous({InParams.OutputFolder}, false);
		if (!assetRegistry.PathExists(InParams.OutputFolder))
		{
			OutResult.Errors.Add(FString::Printf(TEXT("Folder '%s' does not exist in the project."), *InParams.OutputFolder));
		}
	}
}

void FAimOffsetSequenceCreator::ValidateFrameSettings(const FAimOffsetSequenceCreateParams& InParams, FAimOffsetSequenceCreateResult& OutResult) const
{
	const UAnimSequence* sourceSequence = InParams.SourceSequence.Get();
	const int32 maxFrameIndex = sourceSequence ? sourceSequence->GetNumberOfSampledKeys() - 1 : INDEX_NONE;
	TSet<int32> usedFrames;

	for (const EAOM_Direction direction : GetOrderedDirections())
	{
		const int32* frameValue = InParams.DirectionFrames.Find(direction);
		const FString directionName = GetDirectionName(direction);

		if (!frameValue)
		{
			OutResult.Errors.Add(FString::Printf(TEXT("Frame is not set for %s."), *directionName));
			continue;
		}

		if (*frameValue < 0 || (maxFrameIndex != INDEX_NONE && *frameValue > maxFrameIndex))
		{
			OutResult.Errors.Add(FString::Printf(TEXT("Frame %d for %s is outside the animation range [0..%d]."), *frameValue, *directionName, maxFrameIndex));
		}

		if (usedFrames.Contains(*frameValue))
		{
			OutResult.Errors.Add(FString::Printf(TEXT("Frame %d is duplicated. Each direction must use a unique frame."), *frameValue));
		}
		else
		{
			usedFrames.Add(*frameValue);
		}
	}
}

FAimOffsetSequenceCreateResult FAimOffsetSequenceCreator::Validate(const FAimOffsetSequenceCreateParams& InParams) const
{
	FAimOffsetSequenceCreateResult result;
	result.bHasValidationErrors = true;
	ValidateSourceSequence(InParams, result);
	ValidateOutputSettings(InParams, result);
	ValidateFrameSettings(InParams, result);
	return result;
}

bool FAimOffsetSequenceCreator::BuildAnimationSequence(
	const FAimOffsetSequenceCreateParams& InParams,
	UAnimSequence* InSourceSequence,
	USkeletalMesh* InPreviewMesh,
	USkeleton* InSkeleton,
	const FReferenceSkeleton& InReferenceSkeleton,
	EAOM_Direction InDirection,
	int32 InFrameIndex,
	UAnimSequence*& OutCreatedSequence) const
{
	OutCreatedSequence = nullptr;

	FAnimPoseEvaluationOptions evaluationOptions;
	evaluationOptions.EvaluationType = EAnimDataEvalType::Raw;
	evaluationOptions.bShouldRetarget = false;
	evaluationOptions.bExtractRootMotion = false;
	evaluationOptions.bIncorporateRootMotionIntoPose = true;
	evaluationOptions.OptionalSkeletalMesh = InPreviewMesh;

	FAnimPose sampledPose;
	UAnimPoseExtensions::GetAnimPoseAtFrame(InSourceSequence, InFrameIndex, evaluationOptions, sampledPose);
	if (!UAnimPoseExtensions::IsValid(sampledPose))
	{
		return false;
	}

	UAnimSequenceFactory* factory = NewObject<UAnimSequenceFactory>();
	factory->TargetSkeleton = InSkeleton;
	factory->PreviewSkeletalMesh = InPreviewMesh;

	const FString assetName = FString::Printf(TEXT("%s_%s"), *InParams.OutputBaseName, *GetDirectionName(InDirection));
	UObject* createdObject = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateAsset(
		assetName,
		InParams.OutputFolder,
		UAnimSequence::StaticClass(),
		factory,
		TEXT("AimOffsetMaker"));

	UAnimSequence* createdSequence = Cast<UAnimSequence>(createdObject);
	if (!createdSequence)
	{
		return false;
	}

	createdSequence->SetPreviewMesh(InPreviewMesh);

	IAnimationDataController& controller = createdSequence->GetController();
	IAnimationDataController::FScopedBracket scopedBracket(
		controller,
		FText::Format(LOCTEXT("CreateAssetBracket", "Create {0} from sampled pose"), FText::FromString(assetName)),
		false);

	controller.ResetModel(false);
	controller.SetFrameRate(UAnimationSettings::Get()->GetDefaultFrameRate(), false);
	controller.SetNumberOfFrames(FFrameNumber(1), false);

	bool bPoseWriteSucceeded = true;
	for (int32 boneIndex = 0; boneIndex < InReferenceSkeleton.GetRawBoneNum(); ++boneIndex)
	{
		const FName boneName = InReferenceSkeleton.GetBoneName(boneIndex);
		const FTransform& boneTransform = UAnimPoseExtensions::GetBonePose(sampledPose, boneName, EAnimPoseSpaces::Local);
		bPoseWriteSucceeded &= controller.AddBoneCurve(boneName, false);
		bPoseWriteSucceeded &= controller.SetBoneTrackKeys(
			boneName,
			{boneTransform.GetTranslation(), boneTransform.GetTranslation()},
			{boneTransform.GetRotation(), boneTransform.GetRotation()},
			{boneTransform.GetScale3D(), boneTransform.GetScale3D()},
			false);
	}

	if (!bPoseWriteSucceeded)
	{
		createdSequence->ClearFlags(RF_Standalone | RF_Public);
		createdSequence->MarkAsGarbage();
		return false;
	}

	controller.NotifyPopulated();
	createdSequence->MarkPackageDirty();
	createdSequence->PostEditChange();
	OutCreatedSequence = createdSequence;
	return true;
}

void FAimOffsetSequenceCreator::ApplyAdditiveSettings(
	const FAimOffsetSequenceCreateParams& InParams,
	const TMap<EAOM_Direction, UAnimSequence*>& InCreatedSequencesByDirection) const
{
	if (!InParams.bUseAdditiveSettings)
	{
		return;
	}

	UAnimSequence* centerSequence = InCreatedSequencesByDirection.FindRef(EAOM_Direction::CC);
	UAnimSequence* basePoseAnimation = InParams.bUseAdditiveAnimation ? InParams.AdditiveBasePoseAnimation.Get() : centerSequence;

	for (const auto& it : InCreatedSequencesByDirection)
	{
		if (!it.Value)
		{
			continue;
		}

		it.Value->Modify();
		it.Value->AdditiveAnimType = AAT_RotationOffsetMeshSpace;
		it.Value->RefPoseType = ABPT_AnimFrame;
		it.Value->RefPoseSeq = basePoseAnimation;
		it.Value->RefFrameIndex = 0;
		it.Value->CacheDerivedDataForCurrentPlatform();
		it.Value->MarkPackageDirty();
		it.Value->PostEditChange();
	}
}

void FAimOffsetSequenceCreator::ApplyCompressionSettings(
	const FAimOffsetSequenceCreateParams& InParams,
	const TMap<EAOM_Direction, UAnimSequence*>& InCreatedSequencesByDirection) const
{
	if (!InParams.bCompressAnimations)
	{
		return;
	}

	for (const auto& it : InCreatedSequencesByDirection)
	{
		if (!it.Value)
		{
			continue;
		}

		it.Value->WaitOnExistingCompression(true);
		it.Value->CacheDerivedDataForCurrentPlatform();
		it.Value->MarkPackageDirty();
	}
}

UAimOffsetBlendSpace* FAimOffsetSequenceCreator::CreateAimOffsetAsset(
	const FAimOffsetSequenceCreateParams& InParams,
	USkeleton* InSkeleton,
	USkeletalMesh* InPreviewMesh) const
{
	if (!InParams.bCreateAimOffset || !InSkeleton)
	{
		return nullptr;
	}

	UAimOffsetBlendSpaceFactoryNew* factory = NewObject<UAimOffsetBlendSpaceFactoryNew>();
	factory->TargetSkeleton = InSkeleton;
	factory->PreviewSkeletalMesh = InPreviewMesh;

	UObject* createdObject = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateAsset(
		InParams.AimOffsetAssetName,
		InParams.OutputFolder,
		UAimOffsetBlendSpace::StaticClass(),
		factory,
		TEXT("AimOffsetMaker"));

	return Cast<UAimOffsetBlendSpace>(createdObject);
}

FVector FAimOffsetSequenceCreator::GetAimOffsetSamplePosition(EAOM_Direction InDirection) const
{
	switch (InDirection)
	{
	case EAOM_Direction::CC: return FVector(0.0f, 0.0f, 0.0f);
	case EAOM_Direction::CU: return FVector(0.0f, 90.0f, 0.0f);
	case EAOM_Direction::CD: return FVector(0.0f, -90.0f, 0.0f);
	case EAOM_Direction::UR: return FVector(90.0f, 90.0f, 0.0f);
	case EAOM_Direction::URB: return FVector(180.0f, 90.0f, 0.0f);
	case EAOM_Direction::UL: return FVector(-90.0f, 90.0f, 0.0f);
	case EAOM_Direction::ULB: return FVector(-180.0f, 90.0f, 0.0f);
	case EAOM_Direction::CR: return FVector(90.0f, 0.0f, 0.0f);
	case EAOM_Direction::CRB: return FVector(180.0f, 0.0f, 0.0f);
	case EAOM_Direction::CL: return FVector(-90.0f, 0.0f, 0.0f);
	case EAOM_Direction::CLB: return FVector(-180.0f, 0.0f, 0.0f);
	case EAOM_Direction::DR: return FVector(90.0f, -90.0f, 0.0f);
	case EAOM_Direction::DRB: return FVector(180.0f, -90.0f, 0.0f);
	case EAOM_Direction::DL: return FVector(-90.0f, -90.0f, 0.0f);
	case EAOM_Direction::DLB: return FVector(-180.0f, -90.0f, 0.0f);
	default: return FVector::ZeroVector;
	}
}

void FAimOffsetSequenceCreator::SetupAimOffsetAsset(
	const FAimOffsetSequenceCreateParams& InParams,
	UAimOffsetBlendSpace* InAimOffset,
	const TMap<EAOM_Direction, UAnimSequence*>& InCreatedSequencesByDirection) const
{
	if (!InAimOffset)
	{
		return;
	}

	InAimOffset->Modify();

	if (InParams.bSetupAimOffset)
	{
		FBlendParameter horizontalAxis;
		horizontalAxis.DisplayName = TEXT("Yaw");
		horizontalAxis.Min = -180.0f;
		horizontalAxis.Max = 180.0f;
		horizontalAxis.GridNum = 8;
		horizontalAxis.bSnapToGrid = true;

		FBlendParameter verticalAxis;
		verticalAxis.DisplayName = TEXT("Pitch");
		verticalAxis.Min = -90.0f;
		verticalAxis.Max = 90.0f;
		verticalAxis.GridNum = 2;
		verticalAxis.bSnapToGrid = true;

		FInterpolationParameter horizontalInterpolation;
		horizontalInterpolation.InterpolationTime = 0.1f;

		FInterpolationParameter verticalInterpolation;
		verticalInterpolation.InterpolationTime = 0.1f;

		AimOffsetSequenceCreator::SetBlendParameter(InAimOffset, 0, horizontalAxis);
		AimOffsetSequenceCreator::SetBlendParameter(InAimOffset, 1, verticalAxis);
		AimOffsetSequenceCreator::SetInterpolationParameter(InAimOffset, 0, horizontalInterpolation);
		AimOffsetSequenceCreator::SetInterpolationParameter(InAimOffset, 1, verticalInterpolation);
		AimOffsetSequenceCreator::SetTargetWeightInterpolationSpeed(InAimOffset, 2.0f);

		for (const auto& it : InCreatedSequencesByDirection)
		{
			if (!it.Value)
			{
				continue;
			}

			const int32 sampleIndex = InAimOffset->AddSample(it.Value, GetAimOffsetSamplePosition(it.Key));
			if (sampleIndex >= 0)
			{
				InAimOffset->ReplaceSampleAnimation(sampleIndex, it.Value);
			}
		}

		InAimOffset->ValidateSampleData();
		InAimOffset->ResampleData();
	}

	InAimOffset->MarkPackageDirty();
	InAimOffset->PostEditChange();
}

void FAimOffsetSequenceCreator::FinalizeNotification(const FAimOffsetSequenceCreateResult& InResult) const
{
	FNotificationInfo resultInfo(
		InResult.bSuccess
			? FText::Format(LOCTEXT("CreateSuccessMessage", "Created {0} aim offset animations."), FText::AsNumber(InResult.CreatedCount))
			: FText::Format(LOCTEXT("CreateFailureMessage", "Created {0} animations. Failed: {1}"), FText::AsNumber(InResult.CreatedCount), FText::FromString(FString::Join(InResult.FailedAssets, TEXT(", ")))));
	resultInfo.ExpireDuration = 6.0f;
	resultInfo.bUseLargeFont = false;
	if (TSharedPtr<SNotificationItem> notificationItem = FSlateNotificationManager::Get().AddNotification(resultInfo))
	{
		notificationItem->SetCompletionState(InResult.bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}
}

FAimOffsetSequenceCreateResult FAimOffsetSequenceCreator::Execute(const FAimOffsetSequenceCreateParams& InParams) const
{
	FAimOffsetSequenceCreateResult result = Validate(InParams);
	if (result.Errors.Num() > 0)
	{
		return result;
	}

	result.bHasValidationErrors = false;

	UAnimSequence* sourceSequence = InParams.SourceSequence.Get();
	USkeletalMesh* previewMesh = AimOffsetSequenceCreator::ResolvePreviewMesh(sourceSequence);
	USkeleton* skeleton = sourceSequence ? sourceSequence->GetSkeleton() : nullptr;
	if (!sourceSequence || !previewMesh || !skeleton)
	{
		result.Errors.Add(TEXT("Source animation is missing skeleton or preview mesh."));
		result.bHasValidationErrors = true;
		return result;
	}

	FAsyncTaskNotificationConfig notificationConfig;
	notificationConfig.TitleText = LOCTEXT("CreateProgressTitle", "Creating aim offset animations");
	notificationConfig.ProgressText = LOCTEXT("CreateProgressStart", "Preparing...");
	notificationConfig.bKeepOpenOnFailure = true;
	notificationConfig.Icon = FAppStyle::GetBrush("Persona.AssetActions.CreateAnimAsset");
	FAsyncTaskNotification progressNotification(notificationConfig);

	const FReferenceSkeleton& referenceSkeleton = previewMesh->GetRefSkeleton();
	const TArray<EAOM_Direction>& orderedDirections = GetOrderedDirections();
	TMap<EAOM_Direction, UAnimSequence*> createdSequencesByDirection;

	for (int32 index = 0; index < orderedDirections.Num(); ++index)
	{
		const EAOM_Direction direction = orderedDirections[index];
		const int32 frameIndex = InParams.DirectionFrames.FindChecked(direction);
		const FString directionName = GetDirectionName(direction);
		const FString assetName = FString::Printf(TEXT("%s_%s"), *InParams.OutputBaseName, *directionName);

		progressNotification.SetProgressText(FText::Format(
			LOCTEXT("CreateProgressItem", "Creating {0} ({1}/{2})"),
			FText::FromString(assetName),
			FText::AsNumber(index + 1),
			FText::AsNumber(orderedDirections.Num())));

		UAnimSequence* createdSequence = nullptr;
		if (!BuildAnimationSequence(InParams, sourceSequence, previewMesh, skeleton, referenceSkeleton, direction, frameIndex, createdSequence))
		{
			result.FailedAssets.Add(assetName);
			continue;
		}

		result.CreatedAssets.Add(createdSequence);
		createdSequencesByDirection.Add(direction, createdSequence);
		++result.CreatedCount;
	}

	ApplyAdditiveSettings(InParams, createdSequencesByDirection);
	ApplyCompressionSettings(InParams, createdSequencesByDirection);

	if (UAimOffsetBlendSpace* aimOffset = CreateAimOffsetAsset(InParams, skeleton, previewMesh))
	{
		SetupAimOffsetAsset(InParams, aimOffset, createdSequencesByDirection);
		result.CreatedAssets.Add(aimOffset);
	}

	result.bSuccess = result.FailedAssets.Num() == 0 && result.CreatedCount == orderedDirections.Num();

	progressNotification.SetComplete(
		result.bSuccess
			? LOCTEXT("CreateProgressSuccessTitle", "Aim offset animations created")
			: LOCTEXT("CreateProgressFailedTitle", "Aim offset animation creation finished with errors"),
		result.bSuccess
			? FText::Format(LOCTEXT("CreateProgressSuccessText", "Created {0} assets."), FText::AsNumber(result.CreatedCount))
			: FText::Format(LOCTEXT("CreateProgressFailedText", "Created {0} assets. Failed: {1}."), FText::AsNumber(result.CreatedCount), FText::AsNumber(result.FailedAssets.Num())),
		result.bSuccess);

	FinalizeNotification(result);

	if (result.CreatedAssets.Num() > 0)
	{
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().SyncBrowserToAssets(result.CreatedAssets);
	}

	return result;
}

#undef LOCTEXT_NAMESPACE
