#include "GMatrix.h" 
#include "GPoint.h"
#include <math.h> 

// Everything is from Google Doc "Matrix"

void GMatrix::setIdentity() {
    *this = GMatrix(1, 0, 0, 0, 1, 0);
}

void GMatrix::setTranslate(float tx, float ty){
    *this = GMatrix(1, 0, tx, 0, 1, ty);
}

void GMatrix::setScale(float sx, float sy){
    *this = GMatrix(sx, 0, 0, 0, sy, 0);
}

void GMatrix::setRotate(float radians){
    *this = GMatrix(cos(radians), - sin(radians), 0, sin(radians), cos(radians), 0);
}

// This is just matrix multiplication of two 3*3 matrices; details are from Google Doc "Matrix"
void GMatrix::setConcat(const GMatrix& secundo, const GMatrix& primo){
    *this = GMatrix(secundo[0] * primo[0] + secundo[1] * primo[3], secundo[0] * primo[1] + secundo[1] * primo[4], secundo[0] * primo[2] + secundo[1] * primo[5] + secundo[2], secundo[3] * primo[0] + secundo[4] * primo[3], secundo[3] * primo[1] + secundo[4] * primo[4], secundo[3] * primo[2] + secundo[4] * primo[5] + secundo[5]);
}

bool GMatrix::invert(GMatrix* inverse) const{
    float determinant = fMat[0] * fMat[4] - fMat[1] * fMat[3];
    if(determinant == 0){
        return false;
    }else{
        *inverse = GMatrix(fMat[4]/determinant, - fMat[1]/determinant, (fMat[1] * fMat[5] - fMat[4] * fMat[2])/determinant, - fMat[3]/determinant, fMat[0]/determinant, (fMat[3] * fMat[2] - fMat[0] * fMat[5])/determinant);
        return true;
    }
}

void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const{
    for(int i = 0; i < count; i++){
        float x = fMat[0] * src[i].fX + fMat[1] * src[i].fY + fMat[2];
        float y = fMat[3] * src[i].fX + fMat[4] * src[i].fY + fMat[5];
        dst[i] = {x, y};
    }
}
