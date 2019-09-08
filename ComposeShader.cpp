#include "GShader.h"
#include "GMatrix.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GMath.h"

class ComposeShader : public GShader {
    GShader* fColorShader;
    GShader* fTextureShader;
public:
    ComposeShader(GShader* shaderOne, GShader* shaderTwo)
            : fColorShader(shaderOne), fTextureShader(shaderTwo) {}

    bool isOpaque() override {
        return fColorShader->isOpaque() && fTextureShader->isOpaque();
    }

    bool setContext(const GMatrix& ctm) override{
        return fColorShader->setContext(ctm) && fTextureShader->setContext(ctm);
    }

    //TODO: There is still a loop that has not yet been implemented
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        //GPixel savedRow[count];
        /*
        for (int j = 0; j < count; j++){
            savedRow[j] = row[j];
        }
         */
        GPixel shaderOneRow[count];
        GPixel shaderTwoRow[count];

        for (int i = 0; i < count; ++i) {
            shaderOneRow[i] = row[i];
            shaderTwoRow[i] = row[i];
        }

        fColorShader->shadeRow(x, y, count, shaderOneRow);

        fTextureShader->shadeRow(x, y, count, shaderTwoRow);

        for (int i = 0; i < count; ++i){
            int alpha = GRoundToInt((GPixel_GetA(shaderOneRow[i]) * GPixel_GetA(shaderTwoRow[i]))/255.0f);
            int red = GRoundToInt((GPixel_GetR(shaderOneRow[i]) * GPixel_GetR(shaderTwoRow[i]))/255.0f);
            int green = GRoundToInt((GPixel_GetG(shaderOneRow[i]) * GPixel_GetG(shaderTwoRow[i]))/255.0f);
            int blue = GRoundToInt((GPixel_GetB(shaderOneRow[i]) * GPixel_GetB(shaderTwoRow[i]))/255.0f);

            row[i] = GPixel_PackARGB(alpha,red,green,blue);
        }
    }
};