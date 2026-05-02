#include "MetaCurveAction.h"

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "AnimationBlueprintLibrary.h"
#include "ContentBrowserMenuContexts.h"
#include "Curves/RichCurve.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IToolMenusModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FMetaCurveActionModule"

void FMetaCurveActionModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMetaCurveActionModule::RegisterMenus));
}

void FMetaCurveActionModule::ShutdownModule()
{
	for (int32 windowIndex = OpenDialogWindows.Num() - 1; windowIndex >= 0; --windowIndex)
	{
		if (const TSharedPtr<SWindow> window = OpenDialogWindows[windowIndex].Pin())
		{
			window->RequestDestroyWindow();
		}
	}
	OpenDialogWindows.Reset();

	if (IToolMenusModule::IsAvailable())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}
}

void FMetaCurveActionModule::RegisterMenus()
{
	FToolMenuOwnerScoped ownerScoped(this);

	if (UToolMenu* assetMenu = UToolMenus::Get()->ExtendMenu(TEXT("ContentBrowser.AssetContextMenu.AnimSequence")))
	{
		FToolMenuSection& section = assetMenu->FindOrAddSection(TEXT("GetAssetActions"));
		section.AddDynamicEntry(TEXT("MetaCurveAction.ApplyMetaDataCurve"), FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection) {
			const UContentBrowserAssetContextMenuContext* context = InSection.Context.FindContext<UContentBrowserAssetContextMenuContext>();
			if (!context)
			{
				return;
			}

			const TArray<FAssetData> selectedAssets = context->SelectedAssets;
			if (!CanApplyMetaDataCurve(selectedAssets))
			{
				return;
			}

			InSection.AddMenuEntry(
				TEXT("MetaCurveAction.ApplyMetaDataCurve"),
				LOCTEXT("ApplyMetaDataCurveLabel", "Apply Meta Data Curve"),
				LOCTEXT("ApplyMetaDataCurveTooltip", "Open dialog for applying meta data curve."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("ClassIcon.CurveBase")),
				FToolUIActionChoice(FExecuteAction::CreateLambda([this, selectedAssets]() {
					OpenApplyMetaDataCurveDialog(selectedAssets);
				})));
		}));
	}
}

void FMetaCurveActionModule::OpenApplyMetaDataCurveDialog(const TArray<FAssetData>& InSelectedAssets)
{
	TSharedPtr<SEditableTextBox> nameTextBox;
	TSharedRef<SWindow> dialogWindow = SNew(SWindow)
		.Title(LOCTEXT("ApplyMetaDataCurveDialogTitle", "Apply Meta Data Curve"))
		.ClientSize(FVector2D(360.0f, 140.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize)
		.IsTopmostWindow(false);

	dialogWindow->SetContent(
		SNew(SBox)
		.Padding(16.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EnterNameLabel", "Enter name"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SAssignNew(nameTextBox, SEditableTextBox)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ApplyMetaDataCurveApply", "Apply"))
					.OnClicked_Lambda([this, dialogWindow, nameTextBox, InSelectedAssets]() {
						const FString enteredName = nameTextBox.IsValid() ? nameTextBox->GetText().ToString().TrimStartAndEnd() : FString();
						if (enteredName.IsEmpty())
						{
							FNotificationInfo info(LOCTEXT("ApplyMetaDataCurveEmptyName", "Enter curve name."));
							info.ExpireDuration = 4.0f;
							if (const TSharedPtr<SNotificationItem> notification = FSlateNotificationManager::Get().AddNotification(info))
							{
								notification->SetCompletionState(SNotificationItem::CS_Fail);
							}
							return FReply::Handled();
						}

						ApplyMetaDataCurve(InSelectedAssets, enteredName);
						dialogWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ApplyMetaDataCurveCancel", "Cancel"))
					.OnClicked_Lambda([dialogWindow]() {
						dialogWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		]);

	dialogWindow->GetOnWindowClosedEvent().AddRaw(this, &FMetaCurveActionModule::OnDialogWindowClosed);
	OpenDialogWindows.Add(dialogWindow);
	FSlateApplication::Get().AddWindow(dialogWindow);
	if (nameTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(nameTextBox);
	}
}

void FMetaCurveActionModule::ApplyMetaDataCurve(const TArray<FAssetData>& InSelectedAssets, const FString& InCurveName) const
{
	const FName curveName(*InCurveName);
	int32 addedCount = 0;
	int32 updatedCount = 0;
	int32 failedCount = 0;

	FNotificationInfo progressInfo(FText::Format(LOCTEXT("ApplyMetaDataCurveProgressStart", "Applying metadata curve '{0}'..."), FText::FromName(curveName)));
	progressInfo.bFireAndForget = false;
	progressInfo.bUseLargeFont = false;
	progressInfo.bUseSuccessFailIcons = false;
	progressInfo.bUseThrobber = true;
	progressInfo.FadeOutDuration = 0.2f;
	progressInfo.ExpireDuration = 4.0f;
	const TSharedPtr<SNotificationItem> progressNotification = FSlateNotificationManager::Get().AddNotification(progressInfo);
	if (progressNotification.IsValid())
	{
		progressNotification->SetCompletionState(SNotificationItem::CS_Pending);
	}

	const FScopedTransaction transaction(LOCTEXT("ApplyMetaDataCurveTransaction", "Apply Meta Data Curve"));

	for (int32 assetIndex = 0; assetIndex < InSelectedAssets.Num(); ++assetIndex)
	{
		const FAssetData& assetData = InSelectedAssets[assetIndex];
		if (progressNotification.IsValid())
		{
			progressNotification->SetText(FText::Format(
				LOCTEXT("ApplyMetaDataCurveProgressStep", "Applying '{0}' to {1} ({2}/{3})"),
				FText::FromName(curveName),
				FText::FromName(assetData.AssetName),
				FText::AsNumber(assetIndex + 1),
				FText::AsNumber(InSelectedAssets.Num())));
		}

		UAnimSequence* animSequence = Cast<UAnimSequence>(assetData.GetAsset());
		if (!animSequence)
		{
			++failedCount;
			continue;
		}

		const bool bCurveExists = UAnimationBlueprintLibrary::DoesCurveExist(animSequence, curveName, ERawCurveTrackTypes::RCT_Float);
		if (!bCurveExists)
		{
			UAnimationBlueprintLibrary::AddCurve(animSequence, curveName, ERawCurveTrackTypes::RCT_Float, true);
		}

		animSequence->Modify();
		IAnimationDataController& controller = animSequence->GetController();
		const FAnimationCurveIdentifier curveId(curveName, ERawCurveTrackTypes::RCT_Float);
		controller.SetCurveFlag(curveId, AACF_Metadata, true);

		TArray<FRichCurveKey> curveKeys;
		curveKeys.Add(FRichCurveKey(0.0f, 1.0f));
		controller.SetCurveKeys(curveId, curveKeys);

		if (UAnimationBlueprintLibrary::DoesCurveExist(animSequence, curveName, ERawCurveTrackTypes::RCT_Float))
		{
			animSequence->MarkPackageDirty();
			animSequence->PostEditChange();
			if (bCurveExists)
			{
				++updatedCount;
			}
			else
			{
				++addedCount;
			}
			continue;
		}

		++failedCount;
	}

	const bool bSuccess = failedCount == 0;
	const FText resultText = FText::Format(
		LOCTEXT("ApplyMetaDataCurveResult", "Metadata curve '{0}' processed. Added: {1}. Updated: {2}. Failed: {3}."),
		FText::FromName(curveName),
		FText::AsNumber(addedCount),
		FText::AsNumber(updatedCount),
		FText::AsNumber(failedCount));

	if (progressNotification.IsValid())
	{
		progressNotification->SetText(resultText);
		progressNotification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		progressNotification->ExpireAndFadeout();
	}
}

void FMetaCurveActionModule::OnDialogWindowClosed(const TSharedRef<SWindow>& InWindow) const
{
	OpenDialogWindows.RemoveAll([&InWindow](const TWeakPtr<SWindow>& InTrackedWindow) {
		return !InTrackedWindow.IsValid() || InTrackedWindow.Pin() == InWindow;
	});
}

bool FMetaCurveActionModule::CanApplyMetaDataCurve(const TArray<FAssetData>& InSelectedAssets) const
{
	if (InSelectedAssets.IsEmpty())
	{
		return false;
	}

	for (const FAssetData& assetData : InSelectedAssets)
	{
		if (assetData.AssetClassPath != UAnimSequence::StaticClass()->GetClassPathName())
		{
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetaCurveActionModule, MetaCurveAction)
