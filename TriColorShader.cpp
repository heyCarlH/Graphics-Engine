#include "GShader.h"
#include "GMatrix.h"
#include "GBitmap.h"
#include "GPoint.h"
#include "GMath.h"
#include <algorithm>
#include <vector>

class TriColorShader : public GShader {
public:
    TriColorShader(const GPoint pts[], const GColor color[]) {
        fP0 = pts[0];
        fP1 = pts[1];
        fP2 = pts[2];

        fC0 = color[0];
        fC1 = color[1];
        fC2 = color[2];

        fLocalMatrix.set6(fP1.fX-fP0.fX, fP2.fX-fP0.fX, fP0.fX,
                       fP1.fY-fP0.fY, fP2.fY-fP0.fY, fP0.fY);
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
        if (fDevice.isOpaque()) {
            return true;
        }
        return false;
    }

    bool setContext(const GMatrix& ctm) override {
        GMatrix temp;
        temp.setConcat(ctm, fLocalMatrix);

        return temp.invert(&fInverse);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPoint local = fInverse.mapXY(x + 0.5, y + 0.5);

        float px, py;

        for (int i = 0; i < count; ++i) {
            px = local.fX;
            py = local.fY;

            float alpha = px*fC1.fA + py*fC2.fA + (1-px-py)*fC0.fA;
            float red = px*fC1.fR + py*fC2.fR + (1-px-py)*fC0.fR;
            float green = px*fC1.fG + py*fC2.fG + (1-px-py)*fC0.fG;
            float blue = px*fC1.fB + py*fC2.fB + (1-px-py)*fC0.fB;

            GColor color = GColor::MakeARGB(alpha,red,green,blue);

            row[i] = convertSColor(color);

            local.fX += fInverse[GMatrix::SX];
            local.fY += fInverse[GMatrix::KY];
        }
    }

private:
    const GBitmap fDevice;

    GMatrix fLocalMatrix;
    GMatrix fInverse;

    GPoint fP0;
    GPoint fP1;
    GPoint fP2;
    GColor fC0;
    GColor fC1;
    GColor fC2;


};