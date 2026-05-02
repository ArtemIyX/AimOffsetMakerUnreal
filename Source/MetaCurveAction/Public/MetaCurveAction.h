#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SWindow;

class FMetaCurveActionModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void ApplyMetaDataCurve(const TArray<FAssetData>& InSelectedAssets, const FString& InCurveName) const;
	void RegisterMenus();
	void OpenApplyMetaDataCurveDialog(const TArray<FAssetData>& InSelectedAssets);
	void OnDialogWindowClosed(const TSharedRef<SWindow>& InWindow) const;
	bool CanApplyMetaDataCurve(const TArray<FAssetData>& InSelectedAssets) const;

private:
	mutable TArray<TWeakPtr<SWindow>> OpenDialogWindows;
};
