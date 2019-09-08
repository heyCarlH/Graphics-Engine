#include "GShader.h"
#include "GMatrix.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GMath.h"

class ProxyShader : public GShader {
    GShader* fRealShader;
    GMatrix  fExtraTransform;
public:
    ProxyShader(GShader* shader, const GMatrix& extraTransform)
            : fRealShader(shader), fExtraTransform(extraTransform) {}

    bool isOpaque() override {
        return fRealShader->isOpaque();
    }

    bool setContext(const GMatrix& ctm) override{
        GMatrix temp;
        temp.setConcat(ctm, fExtraTransform);
        return fRealShader->setContext(temp);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        fRealShader->shadeRow(x, y, count, row);
    }
};
