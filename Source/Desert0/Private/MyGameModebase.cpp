#include "MyGameModebase.h"
#include "Grid_Manager.h"
#include "Engine/World.h"
#include "MyPlayerController.h"
#include "TimerManager.h"
#include "Cell_Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GameCharacter.h"
#include "AIController.h"
#include "MyAIController.h"

AMyGameModebase::AMyGameModebase()
{
    // Imposta il custom PlayerController
    PlayerControllerClass = AMyPlayerController::StaticClass();

    // Inizialmente, il turno (di battaglia) verrà impostato quando la fase passerà a battaglia
    CurrentTurn = ETurnState::TS_PlayerTurn;
    // La partita inizia nella fase di posizionamento
    CurrentPhase = EGamePhase::GP_Placement;

    PlayerUnitsPlaced = 0;
    AIUnitsPlaced = 0;
}

void AMyGameModebase::BeginPlay()
{
    Super::BeginPlay();

    // Se la classe del Grid Manager è assegnata, spawnalo
    if (GridManagerClass)
    {
        FVector SpawnLocation = FVector::ZeroVector;
        FRotator SpawnRotation = FRotator::ZeroRotator;
        GridManager = GetWorld()->SpawnActor<AGrid_Manager>(GridManagerClass, SpawnLocation, SpawnRotation);
        if (GridManager)
        {
            UE_LOG(LogTemp, Log, TEXT("Grid Manager spawnato con successo."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Errore nello spawn del Grid Manager."));
        }
    }
    
    bPlayerStartsPlacement = FMath::RandBool();
       UE_LOG(LogTemp, Log, TEXT("Lancio della moneta: %s inizia il posizionamento."), bPlayerStartsPlacement ? TEXT("Giocatore") : TEXT("IA"));

    CurrentPhase = EGamePhase::GP_Placement;

    UE_LOG(LogTemp, Log, TEXT("Fase di posizionamento iniziata."));

    // Se l'IA deve iniziare il posizionamento, esegui subito la sua azione
    if (bPlayerStartsPlacement)
    {
        UE_LOG(LogTemp, Log, TEXT("Attesa input del giocatore per posizionare la prima unità."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("L'IA sta posizionando la prima unità..."));
        PlaceAIUnit();
    }

}

void AMyGameModebase::StartPlayerTurn()
{
    CurrentTurn = ETurnState::TS_PlayerTurn;
    UE_LOG(LogTemp, Log, TEXT("Turno del giocatore iniziato."));
    // Qui il PlayerController abilita gli input per il giocatore
}

void AMyGameModebase::StartEnemyTurn()
{
    CurrentTurn = ETurnState::TS_EnemyTurn;
    UE_LOG(LogTemp, Log, TEXT("Turno nemico iniziato."));
    // Simula il "pensiero" dell'IA con un ritardo (es. 2 secondi)
    GetWorldTimerManager().SetTimer(EnemyTurnTimerHandle, this, &AMyGameModebase::ExecuteEnemyTurn, 2.0f, false);
}

void AMyGameModebase::ExecuteEnemyTurn()
{
    UE_LOG(LogTemp, Log, TEXT("Esecuzione del turno nemico."));
    
    // Trova tutte le unità di gioco controllate dall'IA (dipende da come distingui le unità IA dalle unità giocatore)
    TArray<AActor*> EnemyUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), EnemyUnits);

    for (AActor* Actor : EnemyUnits)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && !GameChar->bIsAIControlled) // Supponiamo che AGameCharacter abbia IsPlayerControlled()
        {
            if (AAIController* AIContr = Cast<AAIController>(GameChar->GetController()))
            {
                if (AMyAIController* MyAIContr = Cast<AMyAIController>(AIContr))
                {
                    MyAIContr->RunTurn();
                }
            }
        }
    }
    
    // Dopo aver eseguito le azioni per tutte le unità IA, termina il turno
    EndTurn();
}

void AMyGameModebase::EndTurn()
{
    if (CurrentPhase != EGamePhase::GP_Battle)
    {
        return;
    }

    if (CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        StartEnemyTurn();
    }
    else
    {
        StartPlayerTurn();
    }
}

// Notifica che il giocatore ha posizionato una unità
void AMyGameModebase::PlacePlayerUnit()
{
    UE_LOG(LogTemp, Log, TEXT("Il giocatore posiziona una unità."));

    if (AIUnitClasses.Num() == 0 || !GridManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("AIUnitClasses vuoto o GridManager non impostato!"));
        return;
    }

    int32 Row = GridManager->NumRows - 1;
    int32 Column = PlayerUnitsPlaced;

    ACell_Actor* DestinationCell = nullptr;
    for (ACell_Actor* Cell : GridManager->GridCells)
    {
        if (Cell && Cell->Row == Row && Cell->Column == Column)
        {
            DestinationCell = Cell;
            break;
        }
    }

    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("Nessuna cella trovata in (%d, %d)"), Row, Column);
        return;
    }

    // Scegli una classe casuale per il Player
    int32 RandomIndex = FMath::RandRange(0, AIUnitClasses.Num() - 1);
    TSubclassOf<AGameCharacter> SelectedClass = AIUnitClasses[RandomIndex];

    FVector StartLocation = GridManager->GetStartLocation();
    float Step = GridManager->GetCellStep();

    FVector SpawnLocation = StartLocation + FVector(
        Column * Step,
        Row * Step,
        UnitSpawnZOffset
    );

    FRotator SpawnRotation = FRotator::ZeroRotator;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGameCharacter* SpawnedPlayerUnit = GetWorld()->SpawnActor<AGameCharacter>(SelectedClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (SpawnedPlayerUnit)
    {
        SpawnedPlayerUnit->MoveToCell(DestinationCell);
        UE_LOG(LogTemp, Log, TEXT("Unità del giocatore spawnata con successo: %s"), *SpawnedPlayerUnit->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Errore nello spawn dell'unità del giocatore."));
    }

    NotifyPlayerUnitPlaced();
}

void AMyGameModebase::NotifyPlayerUnitPlaced()
{
    PlayerUnitsPlaced++;
    UE_LOG(LogTemp, Log, TEXT("Unità del Giocatore posizionata. Totale: %d"), PlayerUnitsPlaced);

    if (PlayerUnitsPlaced >= MaxUnitsPerSide && AIUnitsPlaced >= MaxUnitsPerSide)
    {
        StartBattlePhase();
    }
    else
    {
        if (bPlayerStartsPlacement)
        {
            // Giocatore inizia → logica normale
            if (PlayerUnitsPlaced > AIUnitsPlaced)
            {
                PlaceAIUnit();
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Tocca al giocatore a posizionare."));
            }
        }
        else
        {
            // IA inizia → aspetta che il giocatore abbia messo almeno UNA unità
            if (PlayerUnitsPlaced == AIUnitsPlaced)
            {
                GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Tocca al giocatore a posizionare."));
            }
        }
    }
}

void AMyGameModebase::NotifyAIUnitPlaced()
{
    AIUnitsPlaced++;
    UE_LOG(LogTemp, Log, TEXT("Unità dell'IA posizionata. Totale: %d"), AIUnitsPlaced);

    if (PlayerUnitsPlaced >= MaxUnitsPerSide && AIUnitsPlaced >= MaxUnitsPerSide)
    {
        StartBattlePhase();
    }
    else
    {
        if (AIUnitsPlaced >= PlayerUnitsPlaced)
        {
            // Tocca al giocatore
            UE_LOG(LogTemp, Log, TEXT("Tocca al giocatore a posizionare."));
        }
        else
        {
            // Dopo un leggero delay per evitare chiamate immediate
            GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
        }
    }
}

void AMyGameModebase::PlaceAIUnit()
{
    if (!GridManager || AIUnitClasses.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("GridManager non impostato o AIUnitClasses vuoto!"));
        return;
    }

    TArray<ACell_Actor*> FreeCells;
    for (ACell_Actor* Cell : GridManager->GridCells)
    {
        if (Cell && !Cell->bIsOccupied && Cell->CellType != ECellType::Obstacle)
        {
            bool bOccupiedByCharacter = false;

            TArray<AActor*> FoundCharacters;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundCharacters);

            for (AActor* Actor : FoundCharacters)
            {
                AGameCharacter* Character = Cast<AGameCharacter>(Actor);
                if (Character && Character->CurrentRow == Cell->Row && Character->CurrentColumn == Cell->Column)
                {
                    bOccupiedByCharacter = true;
                    break;
                }
            }

            if (!bOccupiedByCharacter)
            {
                FreeCells.Add(Cell);
            }
        }
    }

    if (FreeCells.Num() == 0 || AIUnitsPlaced >= MaxUnitsPerSide)
    {
        UE_LOG(LogTemp, Log, TEXT("Nessuna cella libera o IA ha già piazzato tutte le unità."));
        return;
    }

    TSubclassOf<AGameCharacter> UnitToSpawn = nullptr;
    if (AIUnitsPlaced == 0)
    {
        UnitToSpawn = BrawlerCharacter;
    }
    else if (AIUnitsPlaced == 1)
    {
        UnitToSpawn = SniperCharacter;
    }

    if (UnitToSpawn)
    {
        int32 RandomCellIndex = FMath::RandRange(0, FreeCells.Num() - 1);
        ACell_Actor* DestinationCell = FreeCells[RandomCellIndex];

        FVector SpawnLocation = DestinationCell->GetActorLocation();
        SpawnLocation.Z = UnitSpawnZOffset;

        FRotator SpawnRotation = FRotator::ZeroRotator;
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AGameCharacter* SpawnedAIUnit = GetWorld()->SpawnActor<AGameCharacter>(UnitToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
        if (SpawnedAIUnit)
        {
            AMyAIController* AIController = GetWorld()->SpawnActor<AMyAIController>(AMyAIController::StaticClass());
            if (AIController)
            {
                AIController->Possess(SpawnedAIUnit);
            }

            SpawnedAIUnit->MoveToCell(DestinationCell);
            UE_LOG(LogTemp, Log, TEXT("Unità IA spawnata: %s nella cella (%d, %d)"), *SpawnedAIUnit->GetName(), DestinationCell->Row, DestinationCell->Column);

            NotifyAIUnitPlaced();
        }
    }
}


void AMyGameModebase::StartBattlePhase()
{
    CurrentPhase = EGamePhase::GP_Battle;
    UE_LOG(LogTemp, Log, TEXT("Fase di battaglia iniziata."));

    if (bPlayerStartsPlacement)
    {
        StartPlayerTurn();
    }
    else
    {
        StartEnemyTurn();
    }
}
