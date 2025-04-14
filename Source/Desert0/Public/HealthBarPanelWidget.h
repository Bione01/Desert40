#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarPanelWidget.generated.h"

class UVerticalBox;
class UHealthBarSlotWidget;
class AGameCharacter;

UCLASS()
class DESERT0_API UHealthBarPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void AddHealthBarForCharacter(AGameCharacter* Character);

protected:
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* TopContainer;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* BottomContainer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    TSubclassOf<UHealthBarSlotWidget> HealthBarSlotWidgetClass;

};
