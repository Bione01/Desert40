#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "CoinFlipActor.generated.h" 

UCLASS()
class DESERT0_API ACoinFlipActor : public AActor
{
    GENERATED_BODY()

public:
    ACoinFlipActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
    UStaticMeshComponent* CoinMesh;

    void StartFlip(bool bPlayerStarts);

private:
    bool bIsFlipping;
    float RotationSpeed;
    float FlipDuration;
    float ElapsedTime;
    bool bPlayerStartsFlip;
    
    FTimerHandle TimerHandle_AfterPause;

    void OnFlipFinished();

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Face_Top;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Face_Bottom;
};
