#include "GShader.h"
#include "GMatrix.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GMath.h"


class MyBitmapShader : public GShader {
public:
	MyBitmapShader(const GBitmap& device, const GMatrix& localMatrix, GShader::TileMode mode): myDevice(device), fLocalMatrix(localMatrix), fTileMode(mode){

	}

	bool isOpaque() override {
		return this->myDevice.isOpaque();
	}

	bool setContext(const GMatrix& ctm) override {
		GMatrix temp = fLocalMatrix;
		fLocalMatrix.setConcat(ctm, fLocalMatrix);
		if(!fLocalMatrix.invert(&fInverse)){
			fLocalMatrix = temp;
			return false;
		} else {
			fLocalMatrix = temp;
			fInverse.postScale((float) 1 / this->myDevice.width(), (float) 1 / this->myDevice.height());
			return true;
		}
	}

	void shadeRow(int x, int y, int count, GPixel row[]) override {
		// Compute local coordinates from device coordinates; we need to add 0.5 to x and y because of rounding issues
		GPoint local = fInverse.mapXY(x + 0.5, y + 0.5);
		for (int i = 0; i < count; ++i) {
			float tileX = local.fX;
			float tileY = local.fY;
			if (fTileMode == GShader::kRepeat) { // repeat
				tileX = tileX - GFloorToInt(tileX);
				tileY = tileY - GFloorToInt(tileY);
			}else if (fTileMode == GShader::kClamp) { // clamp
				tileX = fmin(fmax(tileX, 0.0), 0.99999f);
				tileY = fmin(fmax(tileY, 0.0), 0.99999f);
			}else if (fTileMode == GShader::kMirror) { // mirror
				tileX *= 0.5;
				tileX = tileX - GFloorToInt(tileX);
				if (tileX > 0.5) {
					tileX = 1 - tileX;
				}
				tileX *= 2;

				tileY *= 0.5;
				tileY = tileY - GFloorToInt(tileY);
				if (tileY > 0.5) {
					tileY = 1.0 - tileY;
				}
				tileY *= 2;
			}
				// Adjust to bitmap size
				tileX *= this->myDevice.width();
				tileY *= this->myDevice.height();
				// Find the colors by local coordinates
				row[i] = *this->myDevice.getAddr(GFloorToInt(tileX), GFloorToInt(tileY));
				local.fX += fInverse[GMatrix::SX];
				local.fY += fInverse[GMatrix::KY];
			}
		}


private:
	const GBitmap myDevice;
	GMatrix fLocalMatrix;
	// Final inverse; fInverse is a private matrix stored in MyShader.
	GMatrix fInverse;
	GShader::TileMode fTileMode;

};


std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& device, const GMatrix& localMatrix, GShader::TileMode mode) {
	if (!device.pixels()) {
		return nullptr;
	}
	return std::unique_ptr<GShader>(new MyBitmapShader(device, localMatrix, mode));
}
