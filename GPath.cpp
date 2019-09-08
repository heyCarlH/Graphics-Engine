#include <GPath.h>
#include <GRect.h>
#include <GPoint.h>
#include <GMatrix.h>

GPath& GPath::addRect(const GRect& rectangle, Direction direction){
	GPoint a = {rectangle.fLeft, rectangle.fTop};
    GPoint b = {rectangle.fRight, rectangle.fTop};
    GPoint c = {rectangle.fRight, rectangle.fBottom};
    GPoint d = {rectangle.fLeft, rectangle.fBottom};
	if (direction == GPath::kCCW_Direction){
		this->moveTo(a).lineTo(d).lineTo(c).lineTo(b);
	}else if(direction == GPath::kCW_Direction) {
		this->moveTo(a).lineTo(b).lineTo(c).lineTo(d);
	}
	return *this;
}

GPath& GPath::addPolygon(const GPoint points[], int count){
	this->moveTo(points[0]);
	for(int i = 1; i < count; i++){
		this->lineTo(points[i]);
	}
	return *this;
}

GRect GPath::bounds() const{
	int number = (int)fPts.size();
	if(number > 0){
		float leftX = fPts[0].fX;
		float rightX = fPts[0].fX;
		float topY = fPts[0].fY;
		float botY = fPts[0].fY;
		for(int i = 1; i < number; i++){
			if( fPts[i].fX < leftX ){
				leftX = fPts[i].fX;
			}
			if( fPts[i].fX > rightX ){
				rightX = fPts[i].fX;
			}
			if( fPts[i].fY < topY ){
				topY = fPts[i].fY;
			}
			if( fPts[i].fY > botY ){
				botY = fPts[i].fY;
			}
		}
		GRect rect = GRect::MakeLTRB(leftX, topY, rightX, botY);
		return rect;
	}
	return GRect::MakeLTRB(0, 0, 0, 0);
}

void GPath::transform(const GMatrix& matrix){
	int number = (int)fPts.size();
	GMatrix copyMatrix = matrix;
	GPoint* pointArray = fPts.data();
	copyMatrix.mapPoints(pointArray, number);
}


GPath& GPath::addCircle(GPoint center, float radius, Direction direction){
	GMatrix matrix = GMatrix();

	//TODO might have bugs
	matrix.setTranslate(center.fX, center.fY);
	matrix.setScale(radius, radius);

	GPoint pointA = matrix.mapPt(GPoint::Make(1, 0));
	GPoint pointB = matrix.mapPt(GPoint::Make(1, tanf(M_PI/8)));
	GPoint pointC = matrix.mapPt(GPoint::Make(sqrtf(2)/2, sqrtf(2)/2));
	GPoint pointD = matrix.mapPt(GPoint::Make(tanf(M_PI/8), 1));
	GPoint pointE = matrix.mapPt(GPoint::Make(0, 1));
	GPoint pointF = matrix.mapPt(GPoint::Make(-tanf(M_PI/8), 1));
	GPoint pointG = matrix.mapPt(GPoint::Make(-sqrtf(2)/2, sqrtf(2)/2));
	GPoint pointH = matrix.mapPt(GPoint::Make(-1, tanf(M_PI/8)));
	GPoint pointI = matrix.mapPt(GPoint::Make(-1, 0));
	GPoint pointJ = matrix.mapPt(GPoint::Make(-1, -tanf(M_PI/8)));
	GPoint pointK = matrix.mapPt(GPoint::Make(-sqrtf(2)/2, -sqrtf(2)/2));
	GPoint pointL = matrix.mapPt(GPoint::Make(-tanf(M_PI/8), -1));
	GPoint pointM = matrix.mapPt(GPoint::Make(0, -1));
	GPoint pointN = matrix.mapPt(GPoint::Make(tanf(M_PI/8), -1));
	GPoint pointO = matrix.mapPt(GPoint::Make(sqrtf(2)/2, -sqrtf(2)/2));
	GPoint pointP = matrix.mapPt(GPoint::Make(1, -tanf(M_PI/8)));

	if (direction == GPath::kCCW_Direction){ //counter clockwise
		this->moveTo(pointA).quadTo(pointB, pointC).quadTo(pointD, pointE).quadTo(pointF, pointG).quadTo(pointH, pointI).quadTo(pointJ, pointK).quadTo(pointL, pointM).quadTo(pointN, pointO).quadTo(pointP, pointA);
	}else if(direction == GPath::kCW_Direction){ //clockwise
		this->moveTo(pointA).quadTo(pointP, pointO).quadTo(pointN, pointM).quadTo(pointL, pointK).quadTo(pointJ, pointI).quadTo(pointH, pointG).quadTo(pointF, pointE).quadTo(pointD, pointC).quadTo(pointB, pointA);
	}
	return *this;
}

void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t){
	// src[0] is point A, 1 is point B, 2 is point C
	GPoint lineAB = {src[0].fX * (1 - t) + src[1].fX * t, src[0].fY * (1 - t) + src[1].fY * t};
	GPoint lineBC = {src[1].fX * (1 - t) + src[2].fX * t, src[1].fY * (1 - t) + src[2].fY * t};
	GPoint lineABC = {lineAB.fX * (1 - t) + lineBC.fX * t, lineAB.fY * (1 - t) + lineBC.fY * t};

	dst[0] = src[0];
	dst[1] = lineAB;
	dst[2] = lineABC;
	dst[3] = lineBC;
	dst[4] = src[2];
}

void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t){
	// src[0] is point A, 1 is point B, 2 is point C, 3 is point D
	GPoint lineAB = {src[0].fX * (1 - t) + src[1].fX * t, src[0].fY * (1 - t) + src[1].fY * t};
	GPoint lineBC = {src[1].fX * (1 - t) + src[2].fX * t, src[1].fY * (1 - t) + src[2].fY * t};
	GPoint lineCD = {src[2].fX * (1 - t) + src[3].fX * t, src[2].fY * (1 - t) + src[3].fY * t};
	GPoint lineABC = {lineAB.fX * (1 - t) + lineBC.fX * t, lineAB.fY * (1 - t) + lineBC.fY * t};
	GPoint lineBCD = {lineBC.fX * (1 - t) + lineCD.fX * t, lineBC.fY * (1 - t) + lineCD.fY * t};
	GPoint lineABCD = {lineABC.fX * (1 - t) + lineBCD.fX * t, lineABC.fY * (1 - t) + lineBCD.fY * t};

	dst[0] = src[0];
	dst[1] = lineAB;
	dst[2] = lineABC;
	dst[3] = lineABCD;
	dst[4] = lineBCD;
	dst[5] = lineCD;
	dst[6] = src[3];
}
