#include "GShader.h"
#include "GMatrix.h"
#include "GColor.h"
#include "GMath.h"
#include <stdio.h>

class MyLinearGradientShader : public GShader {
public:
	MyLinearGradientShader(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode){
		 fColors = new GColor[count];
		 // memory copy
		 memcpy(fColors, colors, count * sizeof(GColor));
		 fLocalMatrix = GMatrix(p1.fX - p0.fX, p0.fY - p1.fY, p0.fX, p1.fY - p0.fY, p1.fX - p0.fX, p0.fY);
		 fCount = count;
		fTileMode = mode;
	}


	static GPixel convertSColor(const GColor &color){
		float pinAlphaVal = GPinToUnit(color.fA);
		float pinRedVal = GPinToUnit(color.fR);
		float pinGreenVal = GPinToUnit(color.fG);
		float pinBlueVal = GPinToUnit(color.fB);
		int a = GRoundToInt(pinAlphaVal * 255);
		int r = GRoundToInt(pinAlphaVal * pinRedVal * 255);
		int g = GRoundToInt(pinAlphaVal * pinGreenVal * 255);
		int b = GRoundToInt(pinAlphaVal * pinBlueVal * 255);
		return GPixel_PackARGB(a, r, g, b);
	}

	bool isOpaque() override {
		// Return true if all elements in colors are opaque
		for(int i = 0; i < fCount; i++){
			if(fColors[i].fA == 0){
				return false;
			}
		}
		return true;
	}

	bool setContext(const GMatrix& ctm) override {
        GMatrix temp = fLocalMatrix;
        fLocalMatrix.setConcat(ctm, fLocalMatrix);
        if(!fLocalMatrix.invert(&fInverse)){
            fLocalMatrix = temp;
            return false;
        } else {
            fLocalMatrix = temp;
            return true;
        }
    }

	void shadeRow(int x, int y, int count, GPixel row[]) override {
        // Compute local coordinates from device coordinates; we need to add 0.5 to x and y because of rounding issues
		GPoint local = fInverse.mapXY(x + 0.5, y + 0.5);
		for (int i = 0; i < count; ++i) {
			if(fCount != 1){
                float localX = local.fX;
				if(fTileMode == GShader::kRepeat){ // repeat
					localX = localX - GFloorToInt(localX);
				}
				 else if(fTileMode == GShader::kClamp){ // clamp
					localX = GPinToUnit(localX);
				} else if(fTileMode == GShader::kMirror){ // mirror
					localX *= 0.5;
					localX = localX - GFloorToInt(localX);
					if(localX > 0.5){
						localX = 1 - localX;
					}
					localX = localX * 2;
				}
                // find the two color to be combined
                int colorOne = 0;
                int colorTwo = 1;
                float bound = 1.0 / (fCount-1);
                for(int i = 0; i < fCount - 1; i++){
                    if(bound * colorOne <= localX && localX <= bound * colorTwo){
                        break;
                    }
                    colorOne = colorOne + 1;
                    colorTwo = colorTwo + 1;
                }
                // Correct localX by weight
                float adjustedLocalX = (localX - bound * colorOne) / bound;

                // interpolate alph, red, green and blue
                float alphaOne = fColors[colorOne].fA;
                float alphaTwo = fColors[colorTwo].fA;

                float redOne = fColors[colorOne].fR;
                float redTwo = fColors[colorTwo].fR;

                float greenOne = fColors[colorOne].fG;
                float greenTwo = fColors[colorTwo].fG;

                float blueOne = fColors[colorOne].fB;
                float blueTwo = fColors[colorTwo].fB;

                float pinnedAlpha = GPinToUnit((1 - adjustedLocalX) * alphaOne + adjustedLocalX * alphaTwo);
                float pinnedRed = GPinToUnit((1 - adjustedLocalX) * redOne + adjustedLocalX * redTwo);
                float pinnedGreen = GPinToUnit((1 - adjustedLocalX) * greenOne + adjustedLocalX * greenTwo);
                float pinnedBlue = GPinToUnit((1 - adjustedLocalX) * blueOne + adjustedLocalX * blueTwo);

                int roundedAlpha = GRoundToInt(255 * pinnedAlpha);
                int roundedRed = GRoundToInt(255 * pinnedAlpha * pinnedRed);
                int roundedGreen = GRoundToInt(255 * pinnedAlpha * pinnedGreen);
                int roundedBlue = GRoundToInt(255 * pinnedAlpha * pinnedBlue);

                row[i] = GPixel_PackARGB(roundedAlpha, roundedRed, roundedGreen, roundedBlue);
			}else{
                row[i] = convertSColor(fColors[0]);
			}
			local.fX += fInverse[GMatrix::SX];
			local.fY += fInverse[GMatrix::KY];
		}
	}

    


private:
    GColor* fColors;
    GMatrix fLocalMatrix;
    GMatrix fInverse;
    int fCount;
    GShader::TileMode fTileMode;

};

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode) {
	if(count < 1){
		return nullptr;
	}
	return std::unique_ptr<GShader> (new MyLinearGradientShader(p0, p1, colors, count, mode));
}