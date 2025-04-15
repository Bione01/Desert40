#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

UCLASS()
class DESERT0_API UMyGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Difficulty")
    bool bIsHardMode = true; // Default: hard mode attivo

    UFUNCTION(BlueprintCallable, Category = "Difficulty")
    void SetHardMode(bool bHard);
};
