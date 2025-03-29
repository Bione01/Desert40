#include "MyPlayerController.h"
#include "MyGameModebase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "MySelectionWidget.h"
#include "Cell_Actor.h"
#include "GameCharacter.h"
#include "Grid_Manager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    SpawnedSniper = nullptr;
    SpawnedBrawler = nullptr;
    PlacementCount = 0;
    DesiredSpawnLocation = FVector::ZeroVector;
    SelectedCharacterType = NAME_None;
    bIsSniperPlaced = false;
    bIsBrawlerPlaced = false;

    // Trova il Grid Manager
    for (TActorIterator<AGrid_Manager> It(GetWorld()); It; ++It)
    {
        GridManager = *It;
        break;
    }

    ShowCharacterSelectionWidget();
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &AMyPlayerController::HandleLeftMouseClick);
}

void AMyPlayerController::OnCharacterSelected(FName CharacterType)
{
    SelectedCharacterType = CharacterType;

    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->RemoveFromParent();
        CharacterSelectionWidget = nullptr;
    }
    bShowMouseCursor = true;
    SetGameInputMode();
}

void AMyPlayerController::ShowCharacterSelectionWidget()
{
    if (CharacterSelectionWidgetClass)
    {
        CharacterSelectionWidget = CreateWidget<UUserWidget>(this, CharacterSelectionWidgetClass);
        if (CharacterSelectionWidget)
        {
            UMySelectionWidget* SelectionWidget = Cast<UMySelectionWidget>(CharacterSelectionWidget);
            if (SelectionWidget)
            {
                SelectionWidget->UpdateButtonsVisibility(bIsSniperPlaced, bIsBrawlerPlaced);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Cast a UMySelectionWidget fallito!"));
            }

            CharacterSelectionWidget->AddToViewport();
            bShowMouseCursor = true;
            SetInputMode(FInputModeUIOnly());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterSelectionWidget non creato!"));
        }
    }
}

ACell_Actor* AMyPlayerController::GetClickedCell()
{
    float MouseX, MouseY;
    if (!GetMousePosition(MouseX, MouseY))
        return nullptr;

    FVector2D ScreenPosition(MouseX, MouseY);
    FVector WorldLocation, WorldDirection;
    if (DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLocation, WorldDirection))
    {
        FVector Start = WorldLocation;
        FVector End = Start + WorldDirection * 10000.f;
        TArray<FHitResult> HitResults;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetPawn());
        if (GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility, Params))
        {
            for (const FHitResult& Hit : HitResults)
            {
                ACell_Actor* HitCell = Cast<ACell_Actor>(Hit.GetActor());
                if (HitCell)
                    return HitCell;
            }
        }
    }
    return nullptr;
}

AGameCharacter* AMyPlayerController::GetClickedUnit()
{
    float MouseX, MouseY;
    if (!GetMousePosition(MouseX, MouseY))
        return nullptr;

    FVector2D ScreenPosition(MouseX, MouseY);
    FVector WorldLocation, WorldDirection;
    if (DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLocation, WorldDirection))
    {
        FVector Start = WorldLocation;
        FVector End = Start + WorldDirection * 10000.f;
        TArray<FHitResult> HitResults;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetPawn());
        if (GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Pawn, Params))
        {
            for (const FHitResult& Hit : HitResults)
            {
                AGameCharacter* HitCharacter = Cast<AGameCharacter>(Hit.GetActor());
                if (HitCharacter)
                    return HitCharacter;
            }
        }
    }
    return nullptr;
}

void AMyPlayerController::HandleLeftMouseClick()
{
    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (MyGameMode && MyGameMode->CurrentTurn != ETurnState::TS_PlayerTurn)
        return;

    if (CharacterSelectionWidget != nullptr)
        return;

    if (PlacementCount < 2)
    {
        ACell_Actor* ClickedCell = GetClickedCell();
        if (ClickedCell)
        {
            DesiredSpawnLocation = ClickedCell->GetActorLocation();
            if (DesiredSpawnLocation.IsNearlyZero())
                return;

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            if (PlacementCount == 0)
            {
                if (SelectedCharacterType == "Sniper" && SniperCharacterClass && !SpawnedSniper)
                {
                    SpawnedSniper = GetWorld()->SpawnActor<AGameCharacter>(SniperCharacterClass, DesiredSpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    if (SpawnedSniper)
                    {
                        Possess(SpawnedSniper);
                        bIsSniperPlaced = true;
                        PlacementCount++;
                    }
                }
                else if (SelectedCharacterType == "Brawler" && BrawlerCharacterClass && !SpawnedBrawler)
                {
                    SpawnedBrawler = GetWorld()->SpawnActor<AGameCharacter>(BrawlerCharacterClass, DesiredSpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    if (SpawnedBrawler)
                    {
                        Possess(SpawnedBrawler);
                        bIsBrawlerPlaced = true;
                        PlacementCount++;
                    }
                }

                if (PlacementCount == 1)
                    ShowCharacterSelectionWidget();
            }
            else if (PlacementCount == 1)
            {
                if (SelectedCharacterType == "Sniper" && SniperCharacterClass && !SpawnedSniper)
                {
                    SpawnedSniper = GetWorld()->SpawnActor<AGameCharacter>(SniperCharacterClass, DesiredSpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    if (SpawnedSniper)
                    {
                        Possess(SpawnedSniper);
                        bIsSniperPlaced = true;
                        PlacementCount++;
                    }
                }
                else if (SelectedCharacterType == "Brawler" && BrawlerCharacterClass && !SpawnedBrawler)
                {
                    SpawnedBrawler = GetWorld()->SpawnActor<AGameCharacter>(BrawlerCharacterClass, DesiredSpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    if (SpawnedBrawler)
                    {
                        Possess(SpawnedBrawler);
                        bIsBrawlerPlaced = true;
                        PlacementCount++;
                    }
                }
            }
            return;
        }
    }

    // Se clicchi un personaggio
    AGameCharacter* ClickedUnit = GetClickedUnit();
    if (ClickedUnit && ClickedUnit != GetPawn())
    {
        Possess(ClickedUnit);
        HighlightReachableCells(ClickedUnit);
        return;
    }

    // Se clicchi una cella per muoverti
    ACell_Actor* ClickedCell = GetClickedCell();
    if (ClickedCell)
    {
        AGameCharacter* ControlledCharacter = Cast<AGameCharacter>(GetPawn());
        if (ControlledCharacter)
        {
            ControlledCharacter->MoveToCell(ClickedCell);
            ClearHighlights();
        }
    }
}

void AMyPlayerController::HighlightReachableCells(AGameCharacter* SelectedCharacter)
{
    if (!GridManager) return;

    TArray<ACell_Actor*> AllCells = GridManager->GetAllCells();
    FVector CharacterLocation = SelectedCharacter->GetActorLocation();

    for (ACell_Actor* Cell : AllCells)
    {
        if (!Cell) continue;

        float Distance = FVector::Dist2D(Cell->GetActorLocation(), CharacterLocation);
        if (Distance <= MovementRange * CellSize) // MovementRange deve essere una tua variabile, ad es. 2 o 3
        {
            Cell->SetHighlight(true);
        }
        else
        {
            Cell->SetHighlight(false);
        }
    }
}

void AMyPlayerController::ClearHighlights()
{
    if (!GridManager) return;

    TArray<ACell_Actor*> AllCells = GridManager->GetAllCells();
    for (ACell_Actor* Cell : AllCells)
    {
        if (Cell)
        {
            Cell->SetHighlight(false);
        }
    }
}

void AMyPlayerController::SetGameInputMode()
{
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    bShowMouseCursor = true;
}
