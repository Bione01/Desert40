#include "Cell_Actor.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameModebase.h"
#include "TurnState.h"
#include "Components/StaticMeshComponent.h"


ACell_Actor::ACell_Actor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    MyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyMesh"));
    RootComponent = MyMesh;
    
    MyMesh->SetGenerateOverlapEvents(true);
    MyMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    MyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MyMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    MyMesh->SetRenderCustomDepth(true);
    
    // Cell property
    Row = 0;
    Column = 0;
    CellType = ECellType::Normal;
    bIsOccupied = false;
    OccupyingUnit = nullptr;

}

// Call when game start or actor is call
void ACell_Actor::BeginPlay()
{
    Super::BeginPlay();
}

// called every frame
void ACell_Actor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

// Function for Highlight cells
void ACell_Actor::SetHighlight(bool bHighlight)
{
    bIsHighlighted = bHighlight;
    
    if (MyMesh)
    {
        if (bHighlight && HighlightMaterial)
        {
            MyMesh->SetMaterial(0, HighlightMaterial);
        }
        else if (DefaultMaterial)
        {
            MyMesh->SetMaterial(0, DefaultMaterial);
        }
    }
}

void ACell_Actor::SetAttackHighlight(bool bHighlight)
{
    if (MyMesh)
    {
        if (bHighlight && AttackHighlightMaterial)
        {
            MyMesh->SetMaterial(0, AttackHighlightMaterial);
        }
        else if (DefaultMaterial)
        {
            MyMesh->SetMaterial(0, DefaultMaterial);
        }
    }
}

void ACell_Actor::SetOriginHighlight(bool bOn)
{
    if (MyMesh)
    {
        if (bOn && OriginHighlightMaterial)
        {
            MyMesh->SetMaterial(0, OriginHighlightMaterial);
        }
        else if (DefaultMaterial)
        {
            MyMesh->SetMaterial(0, DefaultMaterial);
        }
    }
}

void ACell_Actor::NotifyActorBeginCursorOver()
{
    Super::NotifyActorBeginCursorOver();

    AMyGameModebase* GM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
    if (!GM) return;

    // Ignore if !playerturn, !placementphase, input is disable
    if (GM->GetCurrentTurn() != ETurnState::TS_PlayerTurn ||
        GM->GetCurrentPhase() != EGamePhase::GP_Placement ||
        !GM->bIsPlayerInputEnabled)
    {
        return;
    }

    // Ignore if cell occupy
    if (bIsOccupied || OccupyingUnit != nullptr)
    {
        return;
    }

    // if everithing is ok, highlight
    SetHighlight(true);
}

void ACell_Actor::NotifyActorEndCursorOver()
{
    Super::NotifyActorEndCursorOver();

    AMyGameModebase* GM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
    if (!GM) return;

    if (GM->GetCurrentTurn() == ETurnState::TS_PlayerTurn &&
        GM->GetCurrentPhase() == EGamePhase::GP_Placement &&
        GM->bIsPlayerInputEnabled)
    {
        // turn off only if highlight with HighlightMaterial (hover), not Origin
        if (MyMesh && MyMesh->GetMaterial(0) == HighlightMaterial)
        {
            SetHighlight(false);
        }
    }
}

