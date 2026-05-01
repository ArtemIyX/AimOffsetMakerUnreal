#pragma once

#include "Templates/SharedPointer.h"
#include "Modules/ModuleManager.h"

class FSlateStyleSet;
class UAnimSequence;

class FAimOffsetMakerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void RegisterStyle();
	void UnregisterStyle();
	void OpenAimOffsetDialog() const;
	UAnimSequence* GetCurrentAnimSequenceFromContext(const struct FToolMenuContext& InContext) const;
	bool IsAnimSequenceEditorContext(const struct FToolMenuContext& InContext) const;

private:
	FString ButtonIconName = TEXT("ButtonIcon16");
	mutable TWeakObjectPtr<UAnimSequence> PendingDialogSequence;
	TSharedPtr<FSlateStyleSet> StyleSet;
};
