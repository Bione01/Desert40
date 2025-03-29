#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MySelectionWidget.generated.h"

UCLASS()
class DESERT0_API UMySelectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void UpdateButtonsVisibility(bool IsSniperPlaced, bool IsBrawlerPlaced);

    UPROPERTY(meta = (BindWidget))
    class UButton* SniperButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* BrawlerButton;
};
