#include "SAimOffsetMakerDialog.h"

#include "SAimOffsetAnimPreviewViewport.h"
#include "AimOffsetSequenceCreator.h"
#include "Animation/AnimSequence.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Misc/Optional.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SAimOffsetMakerDialog"

void SAimOffsetMakerDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	SelectedSequence = InArgs._InitialSequence;
	AdditiveBasePoseAnimation = InArgs._InitialSequence;
	bCompressAnimations = true;
	bUseAdditiveSettings = true;
	bUseAdditiveAnimation = true;
	UpdateOutputDefaults(SelectedSequence.Get());

	ChildSlot
	[
		SNew(SBox)
		.ToolTipText(LOCTEXT("DialogRootTooltip", "Configure source animation, output, additive options, and frame mapping for generated aim offset sequences."))
		.Padding(16.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				.ToolTipText(LOCTEXT("DialogScrollTooltip", "Scroll to review all configuration categories in this window."))
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.ToolTipText(LOCTEXT("DialogTextTooltip", "Create one-frame aim offset animation assets from selected frames of a source animation sequence."))
						.Text(LOCTEXT("DialogText", "Anim Offset Maker"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UAnimSequence::StaticClass())
						.ToolTipText(LOCTEXT("SourceSequenceTooltip", "Source animation sequence used to sample poses for every aim offset direction."))
						.ObjectPath_Lambda([this]() -> FString
						{
							return SelectedSequence.IsValid() ? SelectedSequence->GetPathName() : FString();
						})
						.OnObjectChanged_Lambda([this](const FAssetData& InAssetData)
						{
							UAnimSequence* animSequence = Cast<UAnimSequence>(InAssetData.GetAsset());
							SelectedSequence = animSequence;
							UpdateOutputDefaults(animSequence);
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						BuildOutputCategory()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						BuildAdditiveCategory()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						BuildFramesCategory()
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("CreateTooltip", "Validate all settings and create one-frame animation assets in the selected folder."))
					.Text(LOCTEXT("Create", "Create"))
					.OnClicked(this, &SAimOffsetMakerDialog::OnCreateClicked)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("CancelTooltip", "Close the window without creating any assets."))
					.Text(LOCTEXT("Cancel", "Cancel"))
					.OnClicked(this, &SAimOffsetMakerDialog::OnCancelClicked)
				]
			]
		]
	];
}

void SAimOffsetMakerDialog::UpdateOutputDefaults(UAnimSequence* InAnimSequence)
{
	if (!InAnimSequence)
	{
		OutputFolder.Reset();
		OutputBaseName.Reset();
		AimOffsetAssetName.Reset();
		return;
	}

	OutputFolder = FPackageName::GetLongPackagePath(InAnimSequence->GetOutermost()->GetName());
	OutputBaseName = InAnimSequence->GetName();
	AimOffsetAssetName = FString::Printf(TEXT("%s_AnimOffset"), *InAnimSequence->GetName());
}

FReply SAimOffsetMakerDialog::OnCreateClicked()
{
	if (bUseAdditiveSettings && bUseAdditiveAnimation && !AdditiveBasePoseAnimation.IsValid())
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("AdditiveMissingAnimationWarning", "Additive Settings is enabled and Use Animation is turned on, but Base Pose Animation is not set."),
			LOCTEXT("AdditiveMissingAnimationWarningTitle", "Additive Warning"));
		return FReply::Handled();
	}

	FAimOffsetSequenceCreateParams params;
	params.SourceSequence = SelectedSequence.Get();
	params.OutputFolder = OutputFolder.TrimStartAndEnd();
	params.OutputBaseName = OutputBaseName.TrimStartAndEnd();
	params.bCompressAnimations = bCompressAnimations;
	params.bCreateAimOffset = bCreateAimOffset;
	params.AimOffsetAssetName = AimOffsetAssetName.TrimStartAndEnd();
	params.bSetupAimOffset = bSetupAimOffset;
	params.bUseAdditiveSettings = bUseAdditiveSettings;
	params.bUseAdditiveAnimation = bUseAdditiveAnimation;
	params.AdditiveBasePoseAnimation = AdditiveBasePoseAnimation.Get();

	for (const auto& it : DirectionFrames)
	{
		params.DirectionFrames.Add(static_cast<EAOM_Direction>(it.Key), it.Value);
	}

	const FAimOffsetSequenceCreator creator;
	const FAimOffsetSequenceCreateResult result = creator.Execute(params);
	if (result.bHasValidationErrors)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(FString::Join(result.Errors, TEXT("\n"))),
			LOCTEXT("ValidationFailedTitle", "Create Failed"));
		return FReply::Handled();
	}

	if (result.bSuccess)
	{
		if (const TSharedPtr<SWindow> parentWindow = ParentWindow.Pin())
		{
			parentWindow->RequestDestroyWindow();
		}
	}

	return FReply::Handled();
}

FReply SAimOffsetMakerDialog::OnCancelClicked() const
{
	if (const TSharedPtr<SWindow> parentWindow = ParentWindow.Pin())
	{
		parentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

TSharedRef<SWidget> SAimOffsetMakerDialog::BuildAdditiveCategory()
{
	return SNew(SExpandableArea)
		.ToolTipText(LOCTEXT("AdditiveCategoryTooltip", "Optional additive preview settings. This does not affect created assets yet, but lets you inspect a base pose animation."))
		.AreaTitle(LOCTEXT("AdditiveCategoryTitle", "Additive Settings"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 8.0f, 8.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("AdditiveEnabledLabelTooltip", "Enable the optional additive settings preview section."))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Text(LOCTEXT("AdditiveEnabledLabel", "Enable"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.ToolTipText(LOCTEXT("AdditiveEnabledTooltip", "Turn on additive settings. When enabled, a base pose animation can be selected and previewed in 3D."))
					.IsChecked_Lambda([this]()
					{
						return bUseAdditiveSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](const ECheckBoxState InState)
					{
						bUseAdditiveSettings = InState == ECheckBoxState::Checked;
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]()
				{
					return bUseAdditiveSettings ? EVisibility::Visible : EVisibility::Collapsed;
				})
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("UseAnimationLabelTooltip", "Use an external animation sequence as the additive base pose source."))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Text(LOCTEXT("UseAnimationLabel", "Use Animation"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.ToolTipText(LOCTEXT("UseAnimationTooltip", "If enabled, the selected Base Pose Animation will be assigned to created additive sequences. If disabled, the created CC animation will be used."))
					.IsChecked_Lambda([this]()
					{
						return bUseAdditiveAnimation ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](const ECheckBoxState InState)
					{
						bUseAdditiveAnimation = InState == ECheckBoxState::Checked;
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]()
				{
					return bUseAdditiveSettings && bUseAdditiveAnimation ? EVisibility::Visible : EVisibility::Collapsed;
				})
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("BasePoseAnimationLabelTooltip", "Animation used as the optional additive base pose preview."))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Text(LOCTEXT("BasePoseAnimationLabel", "Base Pose Animation"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SObjectPropertyEntryBox)
					.ToolTipText(LOCTEXT("BasePoseAnimationTooltip", "Select an animation sequence to preview as additive base pose reference."))
					.AllowedClass(UAnimSequence::StaticClass())
					.ObjectPath_Lambda([this]() -> FString
					{
						return AdditiveBasePoseAnimation.IsValid() ? AdditiveBasePoseAnimation->GetPathName() : FString();
					})
					.OnObjectChanged_Lambda([this](const FAssetData& InAssetData)
					{
						UAnimSequence* animSequence = Cast<UAnimSequence>(InAssetData.GetAsset());
						AdditiveBasePoseAnimation = animSequence;
						if (AdditivePreviewViewport.IsValid())
						{
							AdditivePreviewViewport->SetPreviewAnimation(animSequence);
						}
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 6.0f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([this]()
				{
					return bUseAdditiveSettings && bUseAdditiveAnimation && !AdditiveBasePoseAnimation.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f)))
				.ToolTipText(LOCTEXT("AdditiveWarningTooltip", "You enabled additive settings, but no base pose animation is selected yet."))
				.Text(LOCTEXT("AdditiveWarningText", "Warning: Additive Settings uses external animation, but Base Pose Animation is not set."))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 8.0f)
			[
				SAssignNew(AdditivePreviewViewport, SAimOffsetAnimPreviewViewport)
				.PreviewAnimation(AdditiveBasePoseAnimation.Get())
				.Visibility_Lambda([this]()
				{
					return bUseAdditiveSettings && bUseAdditiveAnimation && AdditiveBasePoseAnimation.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				.ToolTipText(LOCTEXT("AdditivePreviewTooltip", "3D preview of the selected base pose animation, similar to the animation sequence additive settings preview."))
			]
		];
}

FText SAimOffsetMakerDialog::GetDirectionTooltip(EAOM_Direction InDirection) const
{
	switch (InDirection)
	{
	case EAOM_Direction::CC:
		return LOCTEXT("DirTooltip_CC", "Center center. Neutral forward aim pose.");
	case EAOM_Direction::CU:
		return LOCTEXT("DirTooltip_CU", "Center up. Straight up from the center.");
	case EAOM_Direction::CD:
		return LOCTEXT("DirTooltip_CD", "Center down. Straight down from the center.");
	case EAOM_Direction::UR:
		return LOCTEXT("DirTooltip_UR", "Upper right. Up and right diagonal aim.");
	case EAOM_Direction::URB:
		return LOCTEXT("DirTooltip_URB", "Upper right back. Stronger upper-right corner pose.");
	case EAOM_Direction::UL:
		return LOCTEXT("DirTooltip_UL", "Upper left. Up and left diagonal aim.");
	case EAOM_Direction::ULB:
		return LOCTEXT("DirTooltip_ULB", "Upper left back. Stronger upper-left corner pose.");
	case EAOM_Direction::CR:
		return LOCTEXT("DirTooltip_CR", "Center right. Straight right from the center.");
	case EAOM_Direction::CRB:
		return LOCTEXT("DirTooltip_CRB", "Center right back. Stronger right-side corner pose.");
	case EAOM_Direction::CL:
		return LOCTEXT("DirTooltip_CL", "Center left. Straight left from the center.");
	case EAOM_Direction::CLB:
		return LOCTEXT("DirTooltip_CLB", "Center left back. Stronger left-side corner pose.");
	case EAOM_Direction::DR:
		return LOCTEXT("DirTooltip_DR", "Down right. Down and right diagonal aim.");
	case EAOM_Direction::DRB:
		return LOCTEXT("DirTooltip_DRB", "Down right back. Stronger lower-right corner pose.");
	case EAOM_Direction::DL:
		return LOCTEXT("DirTooltip_DL", "Down left. Down and left diagonal aim.");
	case EAOM_Direction::DLB:
		return LOCTEXT("DirTooltip_DLB", "Down left back. Stronger lower-left corner pose.");
	default:
		return LOCTEXT("DirTooltip_Default", "Aim offset direction frame.");
	}
}

TSharedRef<SWidget> SAimOffsetMakerDialog::BuildFramesCategory()
{
	TSharedRef<SVerticalBox> framesCategoryBox = SNew(SVerticalBox);
	const UEnum* directionEnum = StaticEnum<EAOM_Direction>();

	if (directionEnum)
	{
		const TArray<FDirectionGroup> directionGroups = {
			{
				LOCTEXT("FramesGroupCenter", "Center"),
				FLinearColor(0.29f, 0.45f, 0.67f, 1.0f),
				FLinearColor(0.12f, 0.16f, 0.22f, 0.92f),
				FLinearColor(0.15f, 0.20f, 0.28f, 0.92f),
				{EAOM_Direction::CC, EAOM_Direction::CU, EAOM_Direction::CD}
			},
			{
				LOCTEXT("FramesGroupUpper", "Upper"),
				FLinearColor(0.39f, 0.55f, 0.34f, 1.0f),
				FLinearColor(0.13f, 0.19f, 0.15f, 0.92f),
				FLinearColor(0.17f, 0.24f, 0.19f, 0.92f),
				{EAOM_Direction::UR, EAOM_Direction::URB, EAOM_Direction::UL, EAOM_Direction::ULB}
			},
			{
				LOCTEXT("FramesGroupSides", "Sides"),
				FLinearColor(0.67f, 0.49f, 0.24f, 1.0f),
				FLinearColor(0.22f, 0.17f, 0.11f, 0.92f),
				FLinearColor(0.28f, 0.22f, 0.14f, 0.92f),
				{EAOM_Direction::CR, EAOM_Direction::CRB, EAOM_Direction::CL, EAOM_Direction::CLB}
			},
			{
				LOCTEXT("FramesGroupLower", "Lower"),
				FLinearColor(0.62f, 0.31f, 0.31f, 1.0f),
				FLinearColor(0.21f, 0.11f, 0.11f, 0.92f),
				FLinearColor(0.27f, 0.14f, 0.14f, 0.92f),
				{EAOM_Direction::DR, EAOM_Direction::DRB, EAOM_Direction::DL, EAOM_Direction::DLB}
			}
		};

		for (const FDirectionGroup& group : directionGroups)
		{
			framesCategoryBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				BuildDirectionGroup(group, directionEnum)
			];
		}
	}

	return SNew(SExpandableArea)
		.ToolTipText(LOCTEXT("FramesCategoryTooltip", "Pick the source animation frame for each aim offset direction. Every frame should be unique and inside the source animation range."))
		.AreaTitle(LOCTEXT("FramesCategoryTitle", "Frames"))
		.InitiallyCollapsed(true)
		.BodyContent()
		[
			SNew(SBox)
			.ToolTipText(LOCTEXT("FramesBodyTooltip", "All direction frame assignments. Scroll to inspect and edit every aim offset pose frame."))
			.Padding(FMargin(8.0f, 8.0f, 4.0f, 4.0f))
			[
				SNew(SScrollBox)
				.ToolTipText(LOCTEXT("FramesScrollTooltip", "Scroll through all direction frame rows."))
				+ SScrollBox::Slot()
				[
					framesCategoryBox
				]
			]
		];
}

TSharedRef<SWidget> SAimOffsetMakerDialog::BuildOutputCategory()
{
	FPathPickerConfig outputPathPickerConfig;
	outputPathPickerConfig.DefaultPath = OutputFolder;
	outputPathPickerConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& InPath)
	{
		OutputFolder = InPath;
	});

	TSharedRef<SWidget> outputPathPicker =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().CreatePathPicker(outputPathPickerConfig);

	return SNew(SExpandableArea)
		.ToolTipText(LOCTEXT("OutputCategoryTooltip", "Choose where created animation assets will be saved and which base name they should use."))
		.AreaTitle(LOCTEXT("OutputCategoryTitle", "Output"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 8.0f, 8.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("OutputFolderLabelTooltip", "Content Browser folder where created assets will be placed."))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Text(LOCTEXT("OutputFolderLabel", "Folder"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SEditableTextBox)
					.ToolTipText(LOCTEXT("OutputFolderTooltip", "Project content path for created assets, for example /Game/AimOffsets/Rifle."))
					.Text_Lambda([this]()
					{
						return FText::FromString(OutputFolder);
					})
					.OnTextChanged_Lambda([this](const FText& InText)
					{
						OutputFolder = InText.ToString();
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 6.0f)
			[
				SNew(SBox)
				.ToolTipText(LOCTEXT("OutputFolderPickerTooltip", "Pick an existing Content Browser folder for the created assets."))
				.HeightOverride(220.0f)
				[
					outputPathPicker
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("CompressAnimationsLabelTooltip", "Enable post-create compression for generated animation assets."))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Text(LOCTEXT("CompressAnimationsLabel", "Compress Animations"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.ToolTipText(LOCTEXT("CompressAnimationsTooltip", "If enabled, generated animation sequences will be compressed after creation using the current platform settings."))
					.IsChecked_Lambda([this]()
					{
						return bCompressAnimations ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](const ECheckBoxState InState)
					{
						bCompressAnimations = InState == ECheckBoxState::Checked;
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("OutputBaseNameLabelTooltip", "Base name used before appending the direction suffix."))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Text(LOCTEXT("OutputBaseNameLabel", "Base Name"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SEditableTextBox)
					.ToolTipText(LOCTEXT("OutputBaseNameTooltip", "Created assets will be named BaseName_Direction, for example RifleAim_UR."))
					.Text_Lambda([this]()
					{
						return FText::FromString(OutputBaseName);
					})
					.OnTextChanged_Lambda([this](const FText& InText)
					{
						OutputBaseName = InText.ToString();
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 0.0f, 8.0f, 8.0f)
			[
				SNew(SExpandableArea)
				.ToolTipText(LOCTEXT("AimOffsetCategoryTooltip", "Settings for creating and configuring the final Aim Offset asset."))
				.InitiallyCollapsed(false)
				.HeaderContent()
				[
					SNew(STextBlock)
					.ToolTipText(LOCTEXT("AimOffsetCategoryHeaderTooltip", "Expand to edit Aim Offset asset creation settings."))
					.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
					.Text(LOCTEXT("AimOffsetCategoryLabel", "Anim Offset"))
				]
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(8.0f, 8.0f, 8.0f, 8.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(STextBlock)
							.ToolTipText(LOCTEXT("CreateAimOffsetLabelTooltip", "Create an Aim Offset asset after generating the animation sequences."))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
							.Text(LOCTEXT("CreateAimOffsetLabel", "Create Anim Offset"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SCheckBox)
							.ToolTipText(LOCTEXT("CreateAimOffsetTooltip", "If enabled, an Aim Offset Blend Space asset will be created in the output folder."))
							.IsChecked_Lambda([this]()
							{
								return bCreateAimOffset ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
							})
							.OnCheckStateChanged_Lambda([this](const ECheckBoxState InState)
							{
								bCreateAimOffset = InState == ECheckBoxState::Checked;
							})
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(8.0f, 0.0f, 8.0f, 8.0f)
					[
						SNew(SHorizontalBox)
						.Visibility_Lambda([this]()
						{
							return bCreateAimOffset ? EVisibility::Visible : EVisibility::Collapsed;
						})
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(STextBlock)
							.ToolTipText(LOCTEXT("AimOffsetAssetNameLabelTooltip", "Asset name for the created Aim Offset Blend Space."))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
							.Text(LOCTEXT("AimOffsetAssetNameLabel", "Aim Offset Asset Name"))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SEditableTextBox)
							.ToolTipText(LOCTEXT("AimOffsetAssetNameTooltip", "Name for the Aim Offset asset. By default it uses CurrentAnim_AnimOffset."))
							.Text_Lambda([this]()
							{
								return FText::FromString(AimOffsetAssetName);
							})
							.OnTextChanged_Lambda([this](const FText& InText)
							{
								AimOffsetAssetName = InText.ToString();
							})
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(8.0f, 0.0f, 8.0f, 8.0f)
					[
						SNew(SHorizontalBox)
						.Visibility_Lambda([this]()
						{
							return bCreateAimOffset ? EVisibility::Visible : EVisibility::Collapsed;
						})
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(STextBlock)
							.ToolTipText(LOCTEXT("SetupAimOffsetLabelTooltip", "Apply default axis and smoothing settings and auto-fill samples in the created Aim Offset asset."))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
							.Text(LOCTEXT("SetupAimOffsetLabel", "Setup Anim Offset"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SCheckBox)
							.ToolTipText(LOCTEXT("SetupAimOffsetTooltip", "If enabled, the new Aim Offset asset will be configured with Yaw/Pitch axes, smoothing defaults, and generated samples."))
							.IsChecked_Lambda([this]()
							{
								return bSetupAimOffset ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
							})
							.OnCheckStateChanged_Lambda([this](const ECheckBoxState InState)
							{
								bSetupAimOffset = InState == ECheckBoxState::Checked;
							})
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAimOffsetMakerDialog::BuildDirectionGroup(const FDirectionGroup& InGroup, const UEnum* InDirectionEnum)
{
	TSharedRef<SVerticalBox> groupRowsBox = SNew(SVerticalBox);
	int32 rowIndex = 0;

	for (const EAOM_Direction direction : InGroup.Directions)
	{
		const int64 enumValue = static_cast<int64>(direction);
		if (!InDirectionEnum->IsValidEnumValue(enumValue))
		{
			continue;
		}

		DirectionFrames.FindOrAdd(enumValue) = static_cast<int32>(enumValue);

		groupRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 3.0f)
		[
			SNew(SBorder)
			.ToolTipText(GetDirectionTooltip(direction))
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor((rowIndex % 2 == 0) ? InGroup.RowColorA : InGroup.RowColorB)
			.Padding(FMargin(10.0f, 6.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.ToolTipText(GetDirectionTooltip(direction))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Justification(ETextJustify::Right)
					.Text(FText::FromString(InDirectionEnum->GetNameStringByValue(enumValue)))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.ToolTipText(GetDirectionTooltip(direction))
					.WidthOverride(120.0f)
					[
						SNew(SNumericEntryBox<int32>)
						.ToolTipText(GetDirectionTooltip(direction))
						.Value_Lambda([this, enumValue]() -> TOptional<int32>
						{
							if (const int32* frameValue = DirectionFrames.Find(enumValue))
							{
								return *frameValue;
							}

							return TOptional<int32>();
						})
						.OnValueChanged_Lambda([this, enumValue](const int32 InNewValue)
						{
							DirectionFrames.FindOrAdd(enumValue) = InNewValue;
						})
					]
				]
			]
		];

		++rowIndex;
	}

	return SNew(SBorder)
		.ToolTipText(FText::Format(LOCTEXT("DirectionGroupContainerTooltip", "{0} direction frame settings group."), InGroup.Title))
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.07f, 0.95f))
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.ToolTipText(FText::Format(LOCTEXT("DirectionGroupTooltip", "{0} direction frames."), InGroup.Title))
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(InGroup.HeaderColor)
				.Padding(FMargin(10.0f, 8.0f))
				[
					SNew(STextBlock)
					.ToolTipText(FText::Format(LOCTEXT("DirectionGroupTextTooltip", "{0} direction frames."), InGroup.Title))
					.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
					.ColorAndOpacity(FSlateColor(FLinearColor::Black))
					.Text(InGroup.Title)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(6.0f)
			[
				groupRowsBox
			]
		];
}

#undef LOCTEXT_NAMESPACE
