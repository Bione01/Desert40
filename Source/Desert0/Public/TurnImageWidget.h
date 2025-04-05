#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TurnImageWidget.generated.h"

UCLASS()
class DESERT0_API UTurnImageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetTurnImage(bool bIsPlayerTurn);
    
    UFUNCTION(BlueprintCallable)
    void PlayTurnAnimation(bool bIsPlayerTurn);
    
private:
    bool bIsPlayerCurrentlyVisible = true;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UImage* Image_YourTurn;

    UPROPERTY(meta = (BindWidget))
    class UImage* Image_EnemyTurn;
    
    UPROPERTY(meta = (BindWidgetAnim), Transient)
    UWidgetAnimation* Anim_SlideInPlayer;

    UPROPERTY(meta = (BindWidgetAnim), Transient)
    UWidgetAnimation* Anim_SlideOutPlayer;

    UPROPERTY(meta = (BindWidgetAnim), Transient)
    UWidgetAnimation* Anim_SlideInEnemy;

    UPROPERTY(meta = (BindWidgetAnim), Transient)
    UWidgetAnimation* Anim_SlideOutEnemy;
};
