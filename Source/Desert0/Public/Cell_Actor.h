#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cell_Actor.generated.h"

UENUM(BlueprintType)
enum class ECellType : uint8 {
    Normal    UMETA(DisplayName = "Normal"),
    Obstacle  UMETA(DisplayName = "Obstacle")
};

UCLASS()
class DESERT0_API ACell_Actor : public AActor
{
    GENERATED_BODY()
    
public:
    // Costruct, set value
    ACell_Actor();

protected:
    // called after sapwned actor
    virtual void BeginPlay() override;

public:
    // Chiamato ad ogni frame
    virtual void Tick(float DeltaTime) override;
    virtual void NotifyActorBeginCursorOver() override;
    virtual void NotifyActorEndCursorOver() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 Row;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 Column;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    ECellType CellType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bIsOccupied;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    AActor* OccupyingUnit;
    
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    bool bIsHighlighted;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    UStaticMeshComponent* MyMesh;
    
    UPROPERTY(BlueprintReadOnly)
    FString CellName;
    
    UFUNCTION(BlueprintCallable)
    void SetHighlight(bool bHighlight);
    
    UPROPERTY(EditAnywhere)
    UMaterialInterface* DefaultMaterial;

    UPROPERTY(EditAnywhere)
    UMaterialInterface* HighlightMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* AttackHighlightMaterial;

    UFUNCTION(BlueprintCallable)
    void SetAttackHighlight(bool bHighlight);
    
    UFUNCTION(BlueprintCallable)
    void SetOriginHighlight(bool bOn);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* OriginHighlightMaterial;

};
