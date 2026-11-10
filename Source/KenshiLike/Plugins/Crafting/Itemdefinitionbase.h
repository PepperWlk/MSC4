#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "ItemDefinitionBase.generated.h"

// Classe de base C++ pour PDA_ItemDefinition (Blueprint).
// Aucune variable ici : tes données BP existantes restent intactes.
// Le BP surcharge simplement ces 3 fonctions (Override > ...).
UCLASS(Abstract, BlueprintType)
class UItemDefinitionBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	FText GetDisplayName() const;
	virtual FText GetDisplayName_Implementation() const { return FText::GetEmpty(); }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	UTexture2D* GetIcon() const;
	virtual UTexture2D* GetIcon_Implementation() const { return nullptr; }

	// Texte multi-lignes affiché dans le panneau de stats de l'établi
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	FText GetStatsText() const;
	virtual FText GetStatsText_Implementation() const { return FText::GetEmpty(); }
};