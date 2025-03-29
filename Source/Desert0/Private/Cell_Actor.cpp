#include "Cell_Actor.h"
#include "Components/StaticMeshComponent.h"

// Costruttore: imposta i valori di default
ACell_Actor::ACell_Actor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Inizializzazione delle proprietà della cella
    Row = 0;
    Column = 0;
    CellType = ECellType::Normal;
    bIsOccupied = false;
    OccupyingUnit = nullptr;

}

// Chiamato quando il gioco inizia o l'attore viene spawnato
void ACell_Actor::BeginPlay()
{
    Super::BeginPlay();
    
    // Eventuali inizializzazioni aggiuntive possono essere effettuate qui
}

// Chiamato ad ogni frame
void ACell_Actor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    MyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyMesh"));
    RootComponent = MyMesh;
}

// Funzione per evidenziare la cella
void ACell_Actor::SetHighlight(bool bHighlight)
{
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


