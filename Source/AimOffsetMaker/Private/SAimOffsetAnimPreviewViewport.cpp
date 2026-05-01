#include "SAimOffsetAnimPreviewViewport.h"

#include "AdvancedPreviewScene.h"
#include "AnimPreviewInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/Skeleton.h"
#include "EditorViewportClient.h"

namespace
{
	class FAimOffsetAnimPreviewViewportClient final : public FEditorViewportClient
	{
	public:
		explicit FAimOffsetAnimPreviewViewportClient(FAdvancedPreviewScene& InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget = nullptr)
			: FEditorViewportClient(nullptr, &InPreviewScene, InEditorViewportWidget)
		{
			SetViewMode(VMI_Lit);
			EngineShowFlags.SetCompositeEditorPrimitives(true);
			EngineShowFlags.DisableAdvancedFeatures();
			OverrideNearClipPlane(1.0f);
			bSetListenerPosition = false;
		}

		virtual void Tick(float DeltaSeconds) override
		{
			FEditorViewportClient::Tick(DeltaSeconds);

			if (PreviewScene)
			{
				PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
			}
		}

		virtual FSceneInterface* GetScene() const override
		{
			return PreviewScene ? PreviewScene->GetScene() : nullptr;
		}
	};
}

void SAimOffsetAnimPreviewViewport::Construct(const FArguments& InArgs)
{
	PreviewAnimation = InArgs._PreviewAnimation;
	PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
	PreviewComponent = NewObject<UDebugSkelMeshComponent>();
	PreviewComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	PreviewScene->AddComponent(PreviewComponent, FTransform::Identity);

	SEditorViewport::Construct(SEditorViewport::FArguments());
	RefreshPreview();
}

void SAimOffsetAnimPreviewViewport::SetPreviewAnimation(UAnimSequence* InPreviewAnimation)
{
	PreviewAnimation = InPreviewAnimation;
	RefreshPreview();
}

void SAimOffsetAnimPreviewViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PreviewComponent);
}

FString SAimOffsetAnimPreviewViewport::GetReferencerName() const
{
	return TEXT("SAimOffsetAnimPreviewViewport");
}

TSharedRef<FEditorViewportClient> SAimOffsetAnimPreviewViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShared<FAimOffsetAnimPreviewViewportClient>(*PreviewScene.Get(), SharedThis(this));
	ViewportClient->ViewportType = LVT_Perspective;
	ViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
	ViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);
	ViewportClient->SetRealtime(true);
	return ViewportClient.ToSharedRef();
}

void SAimOffsetAnimPreviewViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	RefreshPreview();
}

void SAimOffsetAnimPreviewViewport::RefreshPreview()
{
	if (!PreviewComponent)
	{
		return;
	}

	UAnimSequence* previewAnimation = PreviewAnimation.Get();
	if (!previewAnimation)
	{
		CurrentAnimation = nullptr;
		return;
	}

	USkeleton* skeleton = previewAnimation->GetSkeleton();
	USkeletalMesh* previewMesh = skeleton ? skeleton->GetAssetPreviewMesh(previewAnimation) : nullptr;
	if (!previewMesh)
	{
		return;
	}

	if (CurrentAnimation.Get() != previewAnimation || PreviewComponent->GetSkeletalMeshAsset() != previewMesh || !PreviewComponent->IsPreviewOn())
	{
		PreviewComponent->SetSkeletalMesh(previewMesh);
		PreviewComponent->EnablePreview(true, previewAnimation);
		if (PreviewComponent->PreviewInstance)
		{
			PreviewComponent->PreviewInstance->SetLooping(true);
			PreviewComponent->PreviewInstance->SetPosition(0.0f, false);
		}

		PreviewComponent->SetPlayRate(1.0f);
		CurrentAnimation = previewAnimation;

		if (ViewportClient.IsValid())
		{
			const float radius = previewMesh->GetImportedBounds().SphereRadius;
			const FVector dir = ViewportClient->GetViewLocation().GetSafeNormal();
			ViewportClient->SetViewLocation(dir * FMath::Max(radius * 1.5f, 75.0f));
		}
	}
}
