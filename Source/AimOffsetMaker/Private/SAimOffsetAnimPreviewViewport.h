#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class FEditorViewportClient;
class UAnimSequence;
class UDebugSkelMeshComponent;

class SAimOffsetAnimPreviewViewport final : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAimOffsetAnimPreviewViewport)
		: _PreviewAnimation(nullptr)
	{
	}

		SLATE_ARGUMENT(UAnimSequence*, PreviewAnimation)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetPreviewAnimation(UAnimSequence* InPreviewAnimation);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

protected:
	// SEditorViewport
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void RefreshPreview();

private:
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FEditorViewportClient> ViewportClient;
	TObjectPtr<UDebugSkelMeshComponent> PreviewComponent;
	TWeakObjectPtr<UAnimSequence> PreviewAnimation;
	TWeakObjectPtr<UAnimSequence> CurrentAnimation;
};
