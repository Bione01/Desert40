#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameCharacter.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Distance    UMETA(DisplayName = "attack distance"),
    Melee       UMETA(DisplayName = "attack melee")
};

UCLASS(Abstract)
class DESERT0_API AGameCharacter : public ACharacter
{
    GENERATED_BODY()
    
public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementFinished);
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMovementFinished OnMovementFinished;
    
    virtual void HandleCounterAttack(AGameCharacter* Attacker);
    virtual void HandleDeath();

    void ReceiveDamage(int32 DamageAmount);

    void Die();
    
    // Costruct
    AGameCharacter();
    
    void StartStepByStepMovement(const TArray<ACell_Actor*>& PathToFollow);
    
    // utility function
    bool IsBrawler() const;
    bool IsSniper() const;
    
    bool CanReachCell(const class ACell_Actor* DestinationCell) const;
    
    virtual void Tick(float DeltaTime) override;

    virtual void BeginPlay() override;
    
    UPROPERTY()
    int32 LastDamageDealt = 0;

    int32 GetLastDamageDealt() const { return LastDamageDealt; }
    
    // units statistic
    UPROPERTY()
    class ACell_Actor* CurrentCell;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 MaxHealth = 100;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Health = 100;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Turn State")
    bool HasMovedThisTurn = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Turn State")
    bool HasAttackedThisTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MovementRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 DamageMin;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 DamageMax;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    EAttackType AttackType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 CurrentRow;
    
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 CurrentColumn;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float UnitSpawnZOffset = 50.f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bIsAIControlled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeed = 600.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    UMaterialInterface* CounterHitMaterial;

    UMaterialInterface* OriginalMaterial;

    FTimerHandle CounterFlashTimer;

    void PlayCounterHitFlash();
    void EndCounterHitFlash();

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* HitEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* HitSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* SpawnSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* DeathSound;

    UPROPERTY()
    ACell_Actor* HighlightedOriginCell = nullptr;

    FVector StartLocation;
    FVector EndLocation;
    float CurrentLerpTime;
    float MaxLerpTime;
    
    // function
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Attack(AGameCharacter* Target);
 
    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual void MoveToCell(class ACell_Actor* DestinationCell, bool bIgnoreRange = false);

    void UpdateSmoothMovement();
    
    // gameplay function
    
    void MoveOneStep();
    TArray<ACell_Actor*> StepPath;
    FTimerHandle StepMovementTimer;

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    int32 GetAttackRange() const;

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    int32 GetMaxMovement() const;
    
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(const FVector& NewLocation);

    UFUNCTION(BlueprintCallable, Category = "Turn State")
    void ResetTurnState();
    
};
