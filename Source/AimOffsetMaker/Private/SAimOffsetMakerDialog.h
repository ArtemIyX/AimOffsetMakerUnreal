#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "AOM_Direction.h"

class SWindow;
class SAimOffsetAnimPreviewViewport;
class UAnimSequence;

class SAimOffsetMakerDialog final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAimOffsetMakerDialog)
		: _InitialSequence(nullptr)
	{
	}

		SLATE_ARGUMENT(UAnimSequence*, InitialSequence)
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FDirectionGroup
	{
		FText Title;
		FLinearColor HeaderColor;
		FLinearColor RowColorA;
		FLinearColor RowColorB;
		TArray<EAOM_Direction> Directions;
	};

	void UpdateOutputDefaults(UAnimSequence* InAnimSequence);
	FReply OnCreateClicked();
	FReply OnCancelClicked() const;
	TSharedRef<SWidget> BuildFramesCategory();
	TSharedRef<SWidget> BuildOutputCategory();
	TSharedRef<SWidget> BuildAdditiveCategory();
	TSharedRef<SWidget> BuildDirectionGroup(const FDirectionGroup& InGroup, const UEnum* InDirectionEnum);
	FText GetDirectionTooltip(EAOM_Direction InDirection) const;

private:
	TWeakPtr<SWindow> ParentWindow;
	TWeakObjectPtr<UAnimSequence> SelectedSequence;
	FString OutputFolder;
	FString OutputBaseName;
	bool bCreateAimOffset = true;
	FString AimOffsetAssetName;
	bool bSetupAimOffset = true;
	TMap<int64, int32> DirectionFrames;
	bool bCompressAnimations = false;
	bool bUseAdditiveSettings = false;
	bool bUseAdditiveAnimation = false;
	TWeakObjectPtr<UAnimSequence> AdditiveBasePoseAnimation;
	TSharedPtr<SAimOffsetAnimPreviewViewport> AdditivePreviewViewport;
};
