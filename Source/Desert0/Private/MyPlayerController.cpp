#include "MyPlayerController.h"
#include "MyGameModebase.h"
#include "Blueprint/UserWidget.h"
#include "UtilityFunctions.h"
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
    
    this->bEnableMouseOverEvents = true;

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
            CharacterSelectionWidget->SetVisibility(ESlateVisibility::Hidden); // 👈 nascondi inizialmente

            // Imposta input su UIOnly se vuoi (opzionale)
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
        return;

    if (MyGameMode->CurrentPhase == EGamePhase::GP_Placement)
    {
        HandlePlacementClick(MyGameMode);
        return;
    }

    if (MyGameMode->CurrentPhase == EGamePhase::GP_Battle && MyGameMode->CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        // === Attacco in attesa dopo movimento ===
        if (bIsWaitingForAttack && SelectedCharacter && !bIsMoving)
        {
            AGameCharacter* ClickedEnemy = GetClickedUnit();
            if (ClickedEnemy && MyGameMode->GetAIUnits().Contains(ClickedEnemy))
            {
                int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - ClickedEnemy->CurrentRow);
                int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - ClickedEnemy->CurrentColumn);
                if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
                {
                    SelectedCharacter->Attack(ClickedEnemy);
                    if (!IsValid(SelectedCharacter)) return;

                    FString Prefix = TEXT("HP");
                    FString UnitCode = SelectedCharacter->IsSniper() ? TEXT("S") : TEXT("B");
                    FString FromCoord = SelectedCharacter->CurrentCell ? ConvertToChessNotation(SelectedCharacter->CurrentCell->Row, SelectedCharacter->CurrentCell->Column) : TEXT("??");
                    int32 DamageDealt = SelectedCharacter->GetLastDamageDealt();

                    FString LogEntry = FString::Printf(TEXT("%s: %s %s %d"), *Prefix, *UnitCode, *FromCoord, DamageDealt);

                    AMyGameModebase* MyGM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
                    if (MyGM)
                    {
                        MyGM->AddMoveToLog(LogEntry);
                    }

                    SelectedCharacter->HasAttackedThisTurn = true;
                    SelectedCharacter->HasMovedThisTurn = true;
                    bIsWaitingForAttack = false;
                    CheckEndOfPlayerUnitTurn();
                    return;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[BATTLE] Nemico fuori range."));
                }
            }
        }

        // === Attacco diretto a inizio turno ===
        if (!bIsMoving && SelectedCharacter && !SelectedCharacter->HasMovedThisTurn && !SelectedCharacter->HasAttackedThisTurn)
        {
            AGameCharacter* ClickedEnemy = GetClickedUnit();
            if (ClickedEnemy && MyGameMode->GetAIUnits().Contains(ClickedEnemy))
            {
                int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - ClickedEnemy->CurrentRow);
                int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - ClickedEnemy->CurrentColumn);
                if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
                {
                    SelectedCharacter->Attack(ClickedEnemy);
                    if (!IsValid(SelectedCharacter)) return;

                    FString Prefix = TEXT("HP");
                    FString UnitCode = SelectedCharacter->IsSniper() ? TEXT("S") : TEXT("B");
                    FString FromCoord = SelectedCharacter->CurrentCell ? ConvertToChessNotation(SelectedCharacter->CurrentCell->Row, SelectedCharacter->CurrentCell->Column) : TEXT("??");
                    int32 DamageDealt = SelectedCharacter->GetLastDamageDealt();

                    FString LogEntry = FString::Printf(TEXT("%s: %s %s %d"), *Prefix, *UnitCode, *FromCoord, DamageDealt);

                    AMyGameModebase* MyGM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
                    if (MyGM)
                    {
                        MyGM->AddMoveToLog(LogEntry);
                    }

                    SelectedCharacter->HasAttackedThisTurn = true;
                    SelectedCharacter->HasMovedThisTurn = true;
                    bIsWaitingForAttack = false;
                    CheckEndOfPlayerUnitTurn();
                    return;
                }
            }
        }

        // === Click su PlayerUnit ===
        AGameCharacter* ClickedUnit = GetClickedUnit();
        if (ClickedUnit && MyGameMode->GetPlayerUnits().Contains(ClickedUnit))
        {
            if (SelectedCharacter == ClickedUnit)
            {
                if (bIsWaitingForAttack)
                {
                    bIsWaitingForAttack = false;
                    SelectedCharacter->HasAttackedThisTurn = true;
                    CheckEndOfPlayerUnitTurn();
                    return;
                }
                else if (SelectedCharacter->HasMovedThisTurn && !SelectedCharacter->HasAttackedThisTurn)
                {
                    bIsWaitingForAttack = false;
                    SelectedCharacter->HasAttackedThisTurn = true;
                    CheckEndOfPlayerUnitTurn();
                    return;
                }

                DeselectCurrentUnit();
                return;
            }

            if (bIsWaitingForAttack && SelectedCharacter && SelectedCharacter != ClickedUnit)
            {
                bIsWaitingForAttack = false;
                SelectedCharacter->HasAttackedThisTurn = true;
                CheckEndOfPlayerUnitTurn();
            }

            if (!ClickedUnit->HasMovedThisTurn)
            {
                DeselectCurrentUnit();
                SelectedCharacter = ClickedUnit;
                RefreshCellOccupancy();
                HighlightReachableCells(ClickedUnit);
                HighlightEnemyCellsInRange();
                return;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[BATTLE] Unità già mossa."));
                return;
            }
        }

        // === Movimento su cella evidenziata ===
        ACell_Actor* ClickedCell = GetClickedCell();
        if (ClickedCell && ClickedCell->bIsHighlighted && SelectedCharacter)
        {
            ACell_Actor* StartCell = GridManager->GetCellAt(SelectedCharacter->CurrentRow, SelectedCharacter->CurrentColumn);
            ACell_Actor* TargetCell = ClickedCell;

            TArray<AGameCharacter*> UnitsToIgnore = { SelectedCharacter };
            TArray<ACell_Actor*> Path = GridManager->FindPathAStarAvoidingUnits(StartCell, TargetCell, UnitsToIgnore);

            if (Path.Num() <= 1)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MOVEMENT] Destinazione occupata, cerco alternativa..."));
                ACell_Actor* AlternativeCell = nullptr;
                int32 BestDistance = MAX_int32;

                const TArray<FIntPoint> Directions = { {1,0}, {-1,0}, {0,1}, {0,-1} };
                for (const FIntPoint& Dir : Directions)
                {
                    int32 NewRow = TargetCell->Row + Dir.X;
                    int32 NewCol = TargetCell->Column + Dir.Y;
                    ACell_Actor* Neighbor = GridManager->GetCellAt(NewRow, NewCol);

                    if (Neighbor && !Neighbor->bIsOccupied && Neighbor->CellType != ECellType::Obstacle)
                    {
                        TArray<ACell_Actor*> AltPath = GridManager->FindPathAStarAvoidingUnits(StartCell, Neighbor, UnitsToIgnore);
                        if (AltPath.Num() > 1)
                        {
                            int32 Distance = FMath::Abs(Neighbor->Row - StartCell->Row) + FMath::Abs(Neighbor->Column - StartCell->Column);
                            if (Distance < BestDistance)
                            {
                                BestDistance = Distance;
                                AlternativeCell = Neighbor;
                                Path = AltPath;
                            }
                        }
                    }
                }

                if (!AlternativeCell)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MOVEMENT] Nessuna cella alternativa trovata."));
                    return;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MOVEMENT] Alternativa trovata: (%d, %d)"), AlternativeCell->Row, AlternativeCell->Column);
                }
            }

            // ✅ Salva correttamente la cella di partenza
            SelectedCharacter->HighlightedOriginCell = SelectedCharacter->CurrentCell;

            SelectedCharacter->OnMovementFinished.Clear();
            SelectedCharacter->OnMovementFinished.AddDynamic(this, &AMyPlayerController::OnPlayerMovementFinishedAndCheckAttack);
            bIsMoving = true;

            SelectedCharacter->StartStepByStepMovement(Path);
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

        // Blocca propagazione su ostacoli e qualsiasi unità (AI o Player), tranne la selezionata
        AGameCharacter* OccupyingCharacter = Cast<AGameCharacter>(CurrentCell->OccupyingUnit);
        if (CurrentCell->CellType == ECellType::Obstacle ||
            (CurrentCell->bIsOccupied && OccupyingCharacter && OccupyingCharacter != SelectedCharacter))
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

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    // Player Units
    for (AGameCharacter* Unit : MyGameMode->GetPlayerUnits())
    {
        if (Unit && Unit->CurrentCell)
        {
            Unit->CurrentCell->bIsOccupied = true;
            Unit->CurrentCell->OccupyingUnit = Unit;
        }
    }

    // AI Units
    for (AGameCharacter* Unit : MyGameMode->GetAIUnits())
    {
        if (Unit && Unit->CurrentCell)
        {
            Unit->CurrentCell->bIsOccupied = true;
            Unit->CurrentCell->OccupyingUnit = Unit;
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
            Cell->SetAttackHighlight(false);
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
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetWidgetToFocus(nullptr); // 👈 evita che il focus rimanga su un widget
        SetInputMode(InputMode);
    }

    bShowMouseCursor = true;
}

void AMyPlayerController::OnPlayerMovementFinished()
{
    bIsMoving = false;
    UpdateHighlights();

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

    HighlightEnemyCellsInRange();
    
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
    if (!IsValid(SelectedCharacter)) return;

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    // Se l'unità ha mosso e attaccato, il turno è finito
    if (SelectedCharacter->HasMovedThisTurn && SelectedCharacter->HasAttackedThisTurn)
    {
        if (IsValid(SelectedCharacter->HighlightedOriginCell))
        {
            SelectedCharacter->HighlightedOriginCell->SetOriginHighlight(false);
            SelectedCharacter->HighlightedOriginCell = nullptr;
        }

        MyGameMode->NotifyPlayerUnitMoved();
        DeselectCurrentUnit();
        return;
    }


    // Se l'unità non ha ancora mosso
    if (!SelectedCharacter->HasMovedThisTurn)
    {
        return;
    }

    // Se l'unità non ha attaccato
    if (!SelectedCharacter->HasAttackedThisTurn)
    {
        if (!bIsWaitingForAttack)
        {
            // 🔥 SPEGNI LUCE ORIGINE
            if (IsValid(SelectedCharacter->HighlightedOriginCell))
            {
                SelectedCharacter->HighlightedOriginCell->SetOriginHighlight(false);
                SelectedCharacter->HighlightedOriginCell = nullptr;
            }

            MyGameMode->NotifyPlayerUnitMoved();
            DeselectCurrentUnit();
            return;
        }
    }
}

void AMyPlayerController::OnPlayerMovementFinishedAndCheckAttack()
{
    bIsMoving = false;
    ClearHighlights();

    if (!IsValid(SelectedCharacter)) return;
    
    RefreshCellOccupancy();

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    SelectedCharacter->HasMovedThisTurn = true;

    FString Prefix = TEXT("HP");
    FString UnitCode = SelectedCharacter->IsSniper() ? TEXT("S") : TEXT("B");

    // ✅ Salva la coordinata PRIMA di azzerare
    FString FromCoord = TEXT("??");
    if (IsValid(SelectedCharacter->HighlightedOriginCell))
    {
        FromCoord = ConvertToChessNotation(
            SelectedCharacter->HighlightedOriginCell->Row,
            SelectedCharacter->HighlightedOriginCell->Column
        );

        SelectedCharacter->HighlightedOriginCell->SetOriginHighlight(false);
        SelectedCharacter->HighlightedOriginCell = nullptr;
    }

    FString ToCoord = SelectedCharacter->CurrentCell ?
        ConvertToChessNotation(SelectedCharacter->CurrentCell->Row, SelectedCharacter->CurrentCell->Column) :
        TEXT("??");

    FString LogEntry = FString::Printf(TEXT("%s: %s %s -> %s"), *Prefix, *UnitCode, *FromCoord, *ToCoord);

    AMyGameModebase* MyGM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
    if (MyGM)
    {
        MyGM->AddMoveToLog(LogEntry);
    }

    // Check attacco possibile dopo il movimento
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

    HighlightEnemyCellsInRange();

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


void AMyPlayerController::SkipCurrentAttack()
{
    if (SelectedCharacter && SelectedCharacter->HasMovedThisTurn && !SelectedCharacter->HasAttackedThisTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("[BATTLE] Attacco skippato su %s"), *SelectedCharacter->GetName());
        SelectedCharacter->HasAttackedThisTurn = true;
        bIsWaitingForAttack = false;

        CheckEndOfPlayerUnitTurn();
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
        FVector NewLocation = MyGameMode->GetCellLocationWithOffset(ClickedCell);
        SpawnedUnit->SetActorLocation(NewLocation);

        SpawnedUnit->bIsAIControlled = false;
        PlacementCount++;
        SpawnedUnit->CurrentRow = ClickedCell->Row;
        SpawnedUnit->CurrentColumn = ClickedCell->Column;
        SpawnedUnit->CurrentCell = ClickedCell;
        
        ClickedCell->bIsOccupied = true;
        ClickedCell->OccupyingUnit = SpawnedUnit;
        
        ClickedCell->SetOriginHighlight(true);
        SpawnedUnit->HighlightedOriginCell = ClickedCell;
        
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

void AMyPlayerController::HighlightEnemyCellsInRange()
{
    if (!GridManager || !SelectedCharacter) return;

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (!MyGameMode) return;

    for (AGameCharacter* Enemy : MyGameMode->GetAIUnits())
    {
        if (Enemy && Enemy->CurrentCell)
        {
            int32 RowDiff = FMath::Abs(SelectedCharacter->CurrentRow - Enemy->CurrentRow);
            int32 ColDiff = FMath::Abs(SelectedCharacter->CurrentColumn - Enemy->CurrentColumn);
            if (RowDiff + ColDiff <= SelectedCharacter->AttackRange)
            {
                Enemy->CurrentCell->SetAttackHighlight(true);
// 🔥 Qui evidenzi in rosso
            }
        }
    }
}

void AMyPlayerController::UpdateHighlights()
{
    ClearHighlights();
    HighlightReachableCells(SelectedCharacter);
    HighlightEnemyCellsInRange();
}

void AMyPlayerController::HideCharacterSelectionWidget()
{
    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void AMyPlayerController::SetCharacterSelectionVisibility(bool bVisible)
{
    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}
void AMyPlayerController::DeselectCurrentUnit()
{
    if (SelectedCharacter)
    {
        // 🔥 NON toccare la luce d'origine
        // Spegni solo le celle evidenziate per il movimento e attacco
        for (ACell_Actor* Cell : GridManager->GetAllCells())
        {
            if (Cell)
            {
                Cell->SetHighlight(false);
                Cell->SetAttackHighlight(false);
            }
        }

        UnPossess();
        SelectedCharacter = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Unità deselezionata."));
    }
}
