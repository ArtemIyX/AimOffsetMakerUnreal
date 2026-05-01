#include "AimOffsetMaker.h"

#include "Animation/AnimSequence.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "IToolMenusModule.h"
#include "SAimOffsetMakerDialog.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Toolkits/AssetEditorToolkitMenuContext.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "FAimOffsetMakerModule"

void FAimOffsetMakerModule::StartupModule()
{
	RegisterStyle();

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAimOffsetMakerModule::RegisterMenus));
}

void FAimOffsetMakerModule::ShutdownModule()
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

	UnregisterStyle();
}

void FAimOffsetMakerModule::RegisterStyle()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShared<FSlateStyleSet>(TEXT("AimOffsetMakerStyle"));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("AimOffsetMaker"))->GetBaseDir() / TEXT("Resources"));
	StyleSet->Set(TEXT("AimOffsetMaker.OpenDialog"), new FSlateImageBrush(StyleSet->RootToContentDir(ButtonIconName, TEXT(".png")), FVector2D(16.0f, 16.0f)));
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FAimOffsetMakerModule::UnregisterStyle()
{
	if (!StyleSet.IsValid())
	{
		return;
	}

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	StyleSet.Reset();
}

void FAimOffsetMakerModule::RegisterMenus()
{
	FToolMenuOwnerScoped ownerScoped(this);

	if (UToolMenu* windowMenu = UToolMenus::Get()->ExtendMenu(TEXT("AssetEditor.AnimationEditor.MainMenu.Window")))
	{
		FToolMenuSection& section = windowMenu->FindOrAddSection(TEXT("AssetEditor"));
		section.AddDynamicEntry(TEXT("AimOffsetMaker.DynamicWindowEntry"), FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection) {
			if (!IsAnimSequenceEditorContext(InSection.Context))
			{
				return;
			}

			InSection.AddMenuEntry(
				TEXT("AimOffsetMaker.OpenDialog"),
				LOCTEXT("Label", "Anim Offset Maker"),
				LOCTEXT("Tooltip", "Open Anim Offset Maker dialog."),
				FSlateIcon(StyleSet->GetStyleSetName(), TEXT("AimOffsetMaker.OpenDialog")),
				FToolUIActionChoice(FExecuteAction::CreateLambda([this, InContext = InSection.Context]()
				{
					PendingDialogSequence = GetCurrentAnimSequenceFromContext(InContext);
					OpenAimOffsetDialog();
				})));
		}));
	}

	if (UToolMenu* toolbarMenu = UToolMenus::Get()->ExtendMenu(TEXT("AssetEditor.AnimationEditor.ToolBar")))
	{
		FToolMenuSection& section = toolbarMenu->FindOrAddSection(TEXT("Asset"));
		section.AddDynamicEntry(TEXT("AimOffsetMaker.DynamicToolbarEntry"), FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection) {
			if (!IsAnimSequenceEditorContext(InSection.Context))
			{
				return;
			}

			FToolMenuEntry entry = FToolMenuEntry::InitToolBarButton(
				TEXT("AimOffsetMaker.OpenDialog.Toolbar"),
				FUIAction(FExecuteAction::CreateLambda([this, InContext = InSection.Context]()
				{
					PendingDialogSequence = GetCurrentAnimSequenceFromContext(InContext);
					OpenAimOffsetDialog();
				})),
				LOCTEXT("ToolbarLabel", "Anim Offset Maker"),
				LOCTEXT("ToolbarTooltip", "Open Anim Offset Maker dialog."),
				FSlateIcon(StyleSet->GetStyleSetName(), TEXT("AimOffsetMaker.OpenDialog")));
			entry.StyleNameOverride = TEXT("CalloutToolbar");
			InSection.AddEntry(entry);
		}));
	}
}

void FAimOffsetMakerModule::OpenAimOffsetDialog() const
{
	TSharedRef<SWindow> dialogWindow = SNew(SWindow)
		.Title(LOCTEXT("DialogTitle", "Anim Offset Maker"))
		.ClientSize(FVector2D(720.0f, 620.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.IsTopmostWindow(false);

	dialogWindow->SetContent(
		SNew(SAimOffsetMakerDialog)
		.InitialSequence(PendingDialogSequence.Get())
		.ParentWindow(dialogWindow));

	dialogWindow->GetOnWindowClosedEvent().AddRaw(this, &FAimOffsetMakerModule::OnAimOffsetDialogClosed);
	OpenDialogWindows.Add(dialogWindow);
	FSlateApplication::Get().AddWindow(dialogWindow);
}

void FAimOffsetMakerModule::OnAimOffsetDialogClosed(const TSharedRef<SWindow>& InWindow) const
{
	OpenDialogWindows.RemoveAll([&InWindow](const TWeakPtr<SWindow>& InTrackedWindow)
	{
		return !InTrackedWindow.IsValid() || InTrackedWindow.Pin() == InWindow;
	});
}

UAnimSequence* FAimOffsetMakerModule::GetCurrentAnimSequenceFromContext(const FToolMenuContext& InContext) const
{
	const UAssetEditorToolkitMenuContext* toolkitContext = InContext.FindContext<UAssetEditorToolkitMenuContext>();
	if (!toolkitContext || !toolkitContext->Toolkit.IsValid())
	{
		return nullptr;
	}

	const TArray<UObject*>* editingObjects = toolkitContext->Toolkit.Pin()->GetObjectsCurrentlyBeingEdited();
	if (!editingObjects)
	{
		return nullptr;
	}

	for (UObject* object : *editingObjects)
	{
		if (UAnimSequence* animSequence = Cast<UAnimSequence>(object))
		{
			return animSequence;
		}
	}

	return nullptr;
}

bool FAimOffsetMakerModule::IsAnimSequenceEditorContext(const FToolMenuContext& InContext) const
{
	return GetCurrentAnimSequenceFromContext(InContext) != nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAimOffsetMakerModule, AimOffsetMaker)
