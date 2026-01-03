#include "Grass/TerraDyneGrassTypes.h"

#if WITH_EDITOR
void UTerraDyneGrassProfile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for (FTerraDyneGrassVariety& Var : Varieties)
	{
		if (Var.Density < 0.001f)
		{
			Var.Density = 0.001f;
		}

		if (Var.ScaleRange.X > Var.ScaleRange.Y)
		{
			float Temp = Var.ScaleRange.X;
			Var.ScaleRange.X = Var.ScaleRange.Y;
			Var.ScaleRange.Y = Temp;
		}
	}
}
#endif