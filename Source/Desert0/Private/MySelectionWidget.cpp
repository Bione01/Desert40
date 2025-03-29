#include "MySelectionWidget.h"
#include "Components/Button.h"

void UMySelectionWidget::UpdateButtonsVisibility(bool IsSniperPlaced, bool IsBrawlerPlaced)
{
    if (SniperButton)
    {
        SniperButton->SetVisibility(IsSniperPlaced ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (BrawlerButton)
    {
        BrawlerButton->SetVisibility(IsBrawlerPlaced ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}
