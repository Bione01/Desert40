#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarSlotWidget.generated.h"

class UProgressBar;
class AGameCharacter;

UCLASS()
class DESERT0_API UHealthBarSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UHealthBarSlotWidget(const FObjectInitializer& ObjectInitializer);
    
    void Init(AGameCharacter* InCharacter);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icons")
    UTexture2D* PlayerBrawlerIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icons")
    UTexture2D* PlayerSniperIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icons")
    UTexture2D* AIBrawlerIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icons")
    UTexture2D* AISniperIcon;

    UPROPERTY(meta = (BindWidget))
    class UImage* IconImage;

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION()
    void UpdateHealthBar();

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthBar;
    
private:
    UPROPERTY()
    AGameCharacter* CharacterRef;
};
