#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameCharacter.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Distance    UMETA(DisplayName = "Attacco a Distanza"),
    Melee       UMETA(DisplayName = "Attacco a Corto Raggio")
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

    /** Applica danno all'unità */
    void ReceiveDamage(int32 DamageAmount);
    
    /** Uccide l'unità */
    void Die();
    
    // Costruttore
    AGameCharacter();
    
    void StartStepByStepMovement(const TArray<ACell_Actor*>& PathToFollow);
    
    // Funzioni di utilità
    bool IsBrawler() const;
    bool IsSniper() const;
    
    bool CanReachCell(const class ACell_Actor* DestinationCell) const;
    
    // Funzione chiamata ogni frame
    virtual void Tick(float DeltaTime) override;
    
    // Funzione chiamata all'inizio del gioco o quando l'attore viene spawnato
    virtual void BeginPlay() override;
    
    UPROPERTY()
    int32 LastDamageDealt = 0;

    int32 GetLastDamageDealt() const { return LastDamageDealt; }
    
    // ---- Statistiche dell'unità ----
    UPROPERTY()
    class ACell_Actor* CurrentCell;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Turn State")
    bool HasMovedThisTurn = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Turn State")
    bool HasAttackedThisTurn = false;
    
    // Numero di celle massime che può muovere
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MovementRange;
    
    // Range d'attacco (numero di celle)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 AttackRange;
    
    // Punti vita dell'unità
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Health;
    
    // Danno minimo e massimo
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 DamageMin;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 DamageMax;
    
    // Tipo di attacco (distanza o melee)
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
    
    // Aggiungi queste
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeed = 600.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    UMaterialInterface* CounterHitMaterial;

    UMaterialInterface* OriginalMaterial; // per ripristinare

    FTimerHandle CounterFlashTimer;

    void PlayCounterHitFlash();
    void EndCounterHitFlash();

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UParticleSystem* HitEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* HitSound;
    
    // GameCharacter.h
    UPROPERTY()
    ACell_Actor* HighlightedOriginCell = nullptr;

    FVector StartLocation;
    FVector EndLocation;
    float CurrentLerpTime;
    float MaxLerpTime;
    
    // ---- Funzioni ----
    
    // Attacco verso un altro personaggio
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Attack(AGameCharacter* Target);
    
    // Movimento verso una cella
    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual void MoveToCell(class ACell_Actor* DestinationCell, bool bIgnoreRange = false);
    
    // Smooth Movement
    void UpdateSmoothMovement();
    
    // ---- Funzioni per IA e Gameplay ----
    
    void MoveOneStep();
    TArray<ACell_Actor*> StepPath;
    FTimerHandle StepMovementTimer;
    
    // Restituisce il range d'attacco dell'unità
    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    int32 GetAttackRange() const;
    
    // Restituisce il range di movimento massimo dell'unità
    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    int32 GetMaxMovement() const;
    
    // Muove l'unità a una nuova posizione
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(const FVector& NewLocation);
    
    // Reset dello stato del turno
    UFUNCTION(BlueprintCallable, Category = "Turn State")
    void ResetTurnState();
    
};
