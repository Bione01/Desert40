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

    // Trova Grid Manager
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

            CharacterSelectionWidget->AddToViewport();
            bShowMouseCursor = true;
            SetInputMode(FInputModeUIOnly());
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
    if (!MyGameMode)
        return;

    if (MyGameMode->CurrentPhase == EGamePhase::GP_Placement)
    {
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

                AGameCharacter* SpawnedUnit = nullptr;

                if (SelectedCharacterType == "Sniper" && SniperCharacterClass && !SpawnedSniper)
                {
                    SpawnedSniper = GetWorld()->SpawnActor<AGameCharacter>(SniperCharacterClass, DesiredSpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    SpawnedUnit = SpawnedSniper;
                    bIsSniperPlaced = true;
                }
                else if (SelectedCharacterType == "Brawler" && BrawlerCharacterClass && !SpawnedBrawler)
                {
                    SpawnedBrawler = GetWorld()->SpawnActor<AGameCharacter>(BrawlerCharacterClass, DesiredSpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    SpawnedUnit = SpawnedBrawler;
                    bIsBrawlerPlaced = true;
                }

                if (SpawnedUnit)
                {
                    SpawnedUnit->bIsAIControlled = false;
                    Possess(SpawnedUnit);
                    PlacementCount++;
                    SpawnedUnit->CurrentRow = ClickedCell->Row;
                    SpawnedUnit->CurrentColumn = ClickedCell->Column;

                    MyGameMode->AddPlayerUnit(SpawnedUnit);
                    MyGameMode->NotifyPlayerUnitPlaced();

                    if (PlacementCount == 1)
                    {
                        ShowCharacterSelectionWidget();
                    }
                }
            }
        }
        return;
    }
    else if (MyGameMode->CurrentPhase == EGamePhase::GP_Battle && MyGameMode->CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        AGameCharacter* ClickedUnit = GetClickedUnit();
        if (ClickedUnit && MyGameMode->GetPlayerUnits().Contains(ClickedUnit))
        {
            Possess(ClickedUnit);
            HighlightReachableCells(ClickedUnit);
            return;
        }

        ACell_Actor* ClickedCell = GetClickedCell();
        if (ClickedCell && ClickedCell->bIsHighlighted)
        {
            AGameCharacter* ControlledCharacter = Cast<AGameCharacter>(GetPawn());
            if (ControlledCharacter)
            {
                ACell_Actor* StartCell = GridManager->GetCellAt(ControlledCharacter->CurrentRow, ControlledCharacter->CurrentColumn);
                ACell_Actor* TargetCell = ClickedCell;

                // Ignora le altre PlayerUnits
                TArray<AGameCharacter*> Ignore;
                AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
                if (MyGameMode)
                {
                    for (AGameCharacter* Unit : MyGameMode->GetPlayerUnits())
                    {
                        if (Unit && Unit != ControlledCharacter)
                        {
                            Ignore.Add(Unit);
                        }
                    }
                }

                TArray<ACell_Actor*> Path = GridManager->FindPathAStarIgnoringUnits(StartCell, TargetCell, Ignore);

                if (Path.Num() > 1) // Se path valido
                {
                    ControlledCharacter->OnMovementFinished.Clear();
                    ControlledCharacter->OnMovementFinished.AddDynamic(this, &AMyPlayerController::OnPlayerMovementFinished);
                    bIsMoving = true;
                    ControlledCharacter->StartStepByStepMovement(Path);
                }

            }
        }
    }
}


void AMyPlayerController::HighlightReachableCells(AGameCharacter* SelectedCharacter)
{
    if (!GridManager || !SelectedCharacter) return;

    TArray<ACell_Actor*> AllCells = GridManager->GetAllCells();

    TMap<FIntPoint, ACell_Actor*> CellMap;
    for (ACell_Actor* Cell : AllCells)
    {
        if (Cell)
        {
            CellMap.Add(FIntPoint(Cell->Row, Cell->Column), Cell);
        }
    }

    TQueue<TPair<FIntPoint, int32>> Queue;

    const FIntPoint Start(SelectedCharacter->CurrentRow, SelectedCharacter->CurrentColumn);
    Queue.Enqueue(TPair<FIntPoint, int32>(Start, 0));

    TSet<FIntPoint> Visited;

    while (!Queue.IsEmpty())
    {
        TPair<FIntPoint, int32> Current;
        Queue.Dequeue(Current);

        const FIntPoint& Coord = Current.Key;
        int32 Distance = Current.Value;

        if (Distance > SelectedCharacter->MovementRange || Visited.Contains(Coord))
            continue;

        Visited.Add(Coord);

        ACell_Actor** FoundCell = CellMap.Find(Coord);
        if (FoundCell && *FoundCell)
        {
            if ((*FoundCell)->CellType == ECellType::Obstacle || ((*FoundCell)->bIsOccupied && Distance > 0))
                continue;

            if (Distance > 0) // 🔥 Non evidenziare la cella di partenza
            {
                (*FoundCell)->SetHighlight(true);
            }
        }

        const TArray<FIntPoint> Directions = {
            FIntPoint(1, 0),
            FIntPoint(-1, 0),
            FIntPoint(0, 1),
            FIntPoint(0, -1)
        };

        for (const FIntPoint& Dir : Directions)
        {
            FIntPoint NextCoord = Coord + Dir;
            if (!Visited.Contains(NextCoord))
            {
                ACell_Actor** NextCell = CellMap.Find(NextCoord);
                if (NextCell && *NextCell)
                {
                    Queue.Enqueue(TPair<FIntPoint, int32>(NextCoord, Distance + 1));
                }
            }
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
void AMyPlayerController::OnPlayerMovementFinished()
{
    bIsMoving = false;
    ClearHighlights();
    AGameCharacter* ControlledCharacter = Cast<AGameCharacter>(GetPawn());
    if (ControlledCharacter)
    {
        ControlledCharacter->CurrentRow = ControlledCharacter->CurrentCell->Row;
        ControlledCharacter->CurrentColumn = ControlledCharacter->CurrentCell->Column;
    }

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (MyGameMode)
    {
        MyGameMode->NotifyPlayerUnitMoved();
    }
}
