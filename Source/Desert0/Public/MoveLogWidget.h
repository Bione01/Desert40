#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MoveLogWidget.generated.h"

UCLASS()
class DESERT0_API UMoveLogWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void AddMoveEntry(const FString& MoveText);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UScrollBox* MoveLogScrollBox;
};
