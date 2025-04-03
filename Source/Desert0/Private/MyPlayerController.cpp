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
    SelectedCharacter = nullptr;
    bIsMoving = false;

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
    UE_LOG(LogTemp, Warning, TEXT("[INPUT] Modalità GameOnly attivata"));
}

void AMyPlayerController::OnCharacterSelected(FName CharacterType)
{
    SelectedCharacterType = CharacterType;

    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->RemoveFromParent();
        CharacterSelectionWidget = nullptr;
    }

    // Imposta input su GameOnly
    SetGameInputMode(true);
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

            // Imposta input su UIOnly
            SetGameInputMode(false);
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
    if (!MyGameMode) return;

    if (bIsMoving && MyGameMode->CurrentPhase == EGamePhase::GP_Battle)
    {
        return;
    }

    // === FASE POSIZIONAMENTO ===
    if (MyGameMode->CurrentPhase == EGamePhase::GP_Placement)
    {
        HandlePlacementClick(MyGameMode);
        return;
    }

    // === FASE BATTAGLIA ===
    if (MyGameMode->CurrentPhase == EGamePhase::GP_Battle && MyGameMode->CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        // Se stiamo aspettando un attacco dopo il movimento
        if (bIsWaitingForAttack && SelectedCharacter)
        {
            AGameCharacter* ClickedEnemy = GetClickedUnit();
            if (ClickedEnemy && MyGameMode->GetAIUnits().Contains(ClickedEnemy))
            {
                int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - ClickedEnemy->CurrentRow);
                int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - ClickedEnemy->CurrentColumn);
                if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
                {
                    SelectedCharacter->Attack(ClickedEnemy);
                    SelectedCharacter->HasAttackedThisTurn = true;
                    bIsWaitingForAttack = false;
                    CheckEndOfPlayerUnitTurn();
                    return;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[BATTLE] Nemico fuori dal range d'attacco."));
                }
            }
        }

        // === ATTACCO DIRETTO A INIZIO TURNO ===
        if (!bIsMoving && SelectedCharacter && !SelectedCharacter->HasMovedThisTurn && !SelectedCharacter->HasAttackedThisTurn)
        {
            AGameCharacter* ClickedEnemy = GetClickedUnit();
            if (ClickedEnemy && MyGameMode->GetAIUnits().Contains(ClickedEnemy))
            {
                int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - ClickedEnemy->CurrentRow);
                int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - ClickedEnemy->CurrentColumn);
                if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
                {
                    UE_LOG(LogTemp, Log, TEXT("[BATTLE] Attacco diretto a inizio turno."));
                    SelectedCharacter->Attack(ClickedEnemy);
                    SelectedCharacter->HasAttackedThisTurn = true;
                    CheckEndOfPlayerUnitTurn();
                    SelectedCharacter->HasMovedThisTurn = true;
                              CheckEndOfPlayerUnitTurn();                    return;
                }
            }
        }

        AGameCharacter* ClickedUnit = GetClickedUnit();

        // Click su un'unità Player
        if (ClickedUnit && MyGameMode->GetPlayerUnits().Contains(ClickedUnit))
        {
            if (SelectedCharacter == ClickedUnit)
            {
                DeselectCurrentUnit();
                return;
            }

            if (!ClickedUnit->HasMovedThisTurn)
            {
                DeselectCurrentUnit();
                SelectedCharacter = ClickedUnit;
                Possess(ClickedUnit);
                HighlightReachableCells(ClickedUnit);
                return;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[BATTLE] Unità già mossa."));
                return;
            }
        }

        // Click su una cella evidenziata per il movimento
        ACell_Actor* ClickedCell = GetClickedCell();
        if (ClickedCell && ClickedCell->bIsHighlighted && SelectedCharacter)
        {
            ACell_Actor* StartCell = GridManager->GetCellAt(SelectedCharacter->CurrentRow, SelectedCharacter->CurrentColumn);
            ACell_Actor* TargetCell = ClickedCell;

            TArray<AGameCharacter*> UnitsToIgnore;
            for (AGameCharacter* Unit : MyGameMode->GetPlayerUnits())
            {
                if (Unit && Unit != SelectedCharacter)
                {
                    UnitsToIgnore.Add(Unit);
                }
            }

            TArray<ACell_Actor*> Path = GridManager->FindPathAStarIgnoringUnits(StartCell, TargetCell, UnitsToIgnore);

            if (Path.Num() > 1)
            {
                SelectedCharacter->OnMovementFinished.Clear();
                SelectedCharacter->OnMovementFinished.AddDynamic(this, &AMyPlayerController::OnPlayerMovementFinishedAndCheckAttack);
                bIsMoving = true;
                SelectedCharacter->StartStepByStepMovement(Path);
            }
        }
    }
}


void AMyPlayerController::HighlightReachableCells(AGameCharacter* CharacterToHighlight)
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
        if (!FoundCell || !*FoundCell)
            continue;

        ACell_Actor* CurrentCell = *FoundCell;

        // Blocca propagazione su ostacoli e AIUnits
        AGameCharacter* OccupyingCharacter = Cast<AGameCharacter>(CurrentCell->OccupyingUnit);
        if (CurrentCell->CellType == ECellType::Obstacle ||
            (CurrentCell->bIsOccupied && OccupyingCharacter && OccupyingCharacter->bIsAIControlled))
        {
            continue;
        }

        // Evidenzia solo se non è la cella di partenza
        if (Distance > 0)
        {
            CurrentCell->SetHighlight(true);
        }

        // Propagazione
        const TArray<FIntPoint> Directions = {
            FIntPoint(1, 0),
            FIntPoint(-1, 0),
            FIntPoint(0, 1),
            FIntPoint(0, -1)
        };

        for (const FIntPoint& Dir : Directions)
        {
            FIntPoint NextCoord = Coord + Dir;
            if (Visited.Contains(NextCoord))
                continue;

            ACell_Actor** NextCell = CellMap.Find(NextCoord);
            if (NextCell && *NextCell)
            {
                Queue.Enqueue(TPair<FIntPoint, int32>(NextCoord, Distance + 1));
            }
        }
    }
}


void AMyPlayerController::RefreshCellOccupancy()
{
    if (!GridManager) return;

    // Reset occupazione
    TArray<ACell_Actor*> AllCells = GridManager->GetAllCells();
    for (ACell_Actor* Cell : AllCells)
    {
        if (Cell)
        {
            Cell->bIsOccupied = false;
            Cell->OccupyingUnit = nullptr;
        }
    }

    // Ricalcola occupazione per PlayerUnits
    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (MyGameMode)
    {
        for (AGameCharacter* Unit : MyGameMode->GetPlayerUnits())
        {
            if (Unit && Unit->CurrentCell)
            {
                Unit->CurrentCell->bIsOccupied = true;
                Unit->CurrentCell->OccupyingUnit = Unit;
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

void AMyPlayerController::SetGameInputMode(bool bGameOnly)
{
    if (bGameOnly)
    {
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);
    }
    else
    {
        FInputModeUIOnly InputMode;
        SetInputMode(InputMode);
    }
    bShowMouseCursor = true;
}


void AMyPlayerController::OnPlayerMovementFinished()
{
    bIsMoving = false;
    ClearHighlights();

    if (!SelectedCharacter) return;

    RefreshCellOccupancy();

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    bool bAttacked = false;
    for (AGameCharacter* Enemy : MyGameMode->GetAIUnits())
    {
        if (Enemy && SelectedCharacter->CurrentCell && Enemy->CurrentCell)
        {
            int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - Enemy->CurrentRow);
            int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - Enemy->CurrentColumn);
            if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
            {
                SelectedCharacter->Attack(Enemy);
                SelectedCharacter->HasAttackedThisTurn = true;
                bAttacked = true;
                break;
            }
        }
    }

    SelectedCharacter->HasMovedThisTurn = true;

    if (bAttacked)
    {
        UE_LOG(LogTemp, Log, TEXT("Attacco eseguito dopo il movimento."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Nessun nemico in range dopo il movimento."));
    }

    CheckEndOfPlayerUnitTurn();
}

void AMyPlayerController::CheckEndOfPlayerUnitTurn()
{
    if (!SelectedCharacter) return;

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    if (SelectedCharacter->HasMovedThisTurn && SelectedCharacter->HasAttackedThisTurn)
    {
        MyGameMode->NotifyPlayerUnitMoved();
        DeselectCurrentUnit();
        return;
    }

    if (!SelectedCharacter->HasMovedThisTurn)
    {
        // Può ancora muoversi
        return;
    }

    if (!SelectedCharacter->HasAttackedThisTurn)
    {
        // Dopo il movimento, se non ha attaccato ed è in range, aspettiamo click → già gestito in HandleClick
        if (!bIsWaitingForAttack)
        {
            MyGameMode->NotifyPlayerUnitMoved();
            DeselectCurrentUnit();
        }
    }
}
void AMyPlayerController::OnPlayerMovementFinishedAndCheckAttack()
{
    bIsMoving = false;
    ClearHighlights();

    if (!SelectedCharacter) return;

    if (SelectedCharacter->CurrentCell)
    {
        SelectedCharacter->CurrentRow = SelectedCharacter->CurrentCell->Row;
        SelectedCharacter->CurrentColumn = SelectedCharacter->CurrentCell->Column;
    }

    RefreshCellOccupancy();

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    // Dopo il movimento controlla se ci sono nemici in range
    bool bEnemyInRange = false;
    for (AGameCharacter* Enemy : MyGameMode->GetAIUnits())
    {
        if (Enemy && SelectedCharacter->CurrentCell && Enemy->CurrentCell)
        {
            int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - Enemy->CurrentRow);
            int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - Enemy->CurrentColumn);
            if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
            {
                bEnemyInRange = true;
                break;
            }
        }
    }

    SelectedCharacter->HasMovedThisTurn = true;

    if (bEnemyInRange)
    {
        UE_LOG(LogTemp, Log, TEXT("[BATTLE] Nemico in range, attendendo click per attacco."));
        bIsWaitingForAttack = true;
    }
    else
    {
        CheckEndOfPlayerUnitTurn();
    }
}


void AMyPlayerController::DeselectCurrentUnit()
{
    if (SelectedCharacter)
    {
        ClearHighlights();
        UnPossess();
        SelectedCharacter = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Unità deselezionata."));
    }
}
void AMyPlayerController::HandlePlacementClick(AMyGameModebase* MyGameMode)
{
    if (SelectedCharacterType == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PLACEMENT] Seleziona un personaggio prima di cliccare una cella."));
        return;
    }

    if (PlacementCount >= 2) return;

    ACell_Actor* ClickedCell = GetClickedCell();
    if (!ClickedCell) return;

    if (ClickedCell->CellType == ECellType::Obstacle || ClickedCell->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PLACEMENT] Cella (%d, %d) non valida."), ClickedCell->Row, ClickedCell->Column);
        return;
    }

    DesiredSpawnLocation = ClickedCell->GetActorLocation();
    if (DesiredSpawnLocation.IsNearlyZero()) return;

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
        FVector NewLocation = DesiredSpawnLocation + FVector(0, 0, SpawnedUnit->UnitSpawnZOffset);
        SpawnedUnit->SetActorLocation(NewLocation);

        SpawnedUnit->bIsAIControlled = false;
        PlacementCount++;
        SpawnedUnit->CurrentRow = ClickedCell->Row;
        SpawnedUnit->CurrentColumn = ClickedCell->Column;

        MyGameMode->AddPlayerUnit(SpawnedUnit);
        MyGameMode->NotifyPlayerUnitPlaced();

        UE_LOG(LogTemp, Warning, TEXT("[PLACEMENT] Unità %s posizionata nella cella (%d, %d)"), *SpawnedUnit->GetName(), ClickedCell->Row, ClickedCell->Column);

        SelectedCharacterType = NAME_None;

        if (PlacementCount < 2)
        {
            ShowCharacterSelectionWidget();
        }
    }
}
