#pragma once

#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "Modules/ModuleManager.h"

class FSlateStyleSet;
class SWindow;
class UAnimSequence;

class FAimOffsetMakerModule : public IModuleInterface
{
public:
#pragma region ModuleOverrides
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
#pragma endregion

#pragma region Methods
private:
	void RegisterMenus();
	void RegisterStyle();
	void UnregisterStyle();
	void OpenAimOffsetDialog() const;
	void OnAimOffsetDialogClosed(const TSharedRef<SWindow>& InWindow) const;
	UAnimSequence* GetCurrentAnimSequenceFromContext(const struct FToolMenuContext& InContext) const;
	bool IsAnimSequenceEditorContext(const struct FToolMenuContext& InContext) const;
#pragma endregion

#pragma region Properties
private:
	FString ButtonIconName = TEXT("ButtonIcon16");
	mutable TWeakObjectPtr<UAnimSequence> PendingDialogSequence;
	mutable TArray<TWeakPtr<SWindow>> OpenDialogWindows;
	TSharedPtr<FSlateStyleSet> StyleSet;
#pragma endregion
};
