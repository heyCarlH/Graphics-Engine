#include "GCanvas.h"
#include "GBitmap.h"
#include "GPixel.h"
#include "GRect.h"
#include "GColor.h"
#include "GPaint.h"
#include "GPoint.h"
#include "Edges.h"
#include "GMatrix.h"
#include "GShader.h"
#include "GBlendMode.h"
#include "GPath.h"
#include "math.h"
#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include "ProxyShader.cpp"
#include "TriColorShader.cpp"
#include "ComposeShader.cpp"



class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {} // pass a parameter to the superclass constructor

    // layer list
    std::vector<GMatrix> ctmList;

    void save() override {
        ctmList.push_back(ctm);
    }

    void restore() override {
        if(ctmList.empty() == false){
            ctm = ctmList.back();
            ctmList.pop_back();
        }
    }



    void concat(const GMatrix& matrix) override {
        ctm.setConcat(ctm, matrix);
    }

    static void resortBackwards(int edgeIndex, std::vector<Edges> &edges){
        if(edgeIndex <= 0){
            return;
        }
        for(int i = edgeIndex; i > 0; i--){
            for(int j = edgeIndex; j > edgeIndex - i; j--){
                if(edges[j].currentX < edges[j - 1].currentX){
                    Edges temp = edges[j];
                    edges[j] = edges[j - 1];
                    edges[j - 1] = temp;
                }
            }
        }
    }


    // find minY and maxY in E
    static int FindMinY(const std::vector<Edges>& edge){
        int minY = edge[0].topY;
        for(int i = 1; i < edge.size(); i++){
            if(edge[i].topY < minY){
                minY = edge[i].topY;
            }
        }
        return minY;
    }
    static int FindMaxY(const std::vector<Edges>& edge){
        int maxY = edge[0].bottomY;
        for(int i = 1; i < edge.size(); i++){
            if(edge[i].bottomY > maxY){
                maxY = edge[i].bottomY;
            }
        }
        return maxY;
    }

    GPixel convertSColor(const GColor& color) {
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

    // turn 0xAABBCCDD into 0x00AA00CC00BB00DD
    static uint64_t expand(uint32_t x) {
        uint64_t hi = x & 0xFF00FF00;  // the A and G components
        uint64_t lo = x & 0x00FF00FF;// the R and B components
        return (hi << 24) | lo;
    }

    // turn 0xXX into 0x00XX00XX00XX00XX
    static uint64_t replicate(uint64_t x) {
        return (x << 48) | (x << 32) | (x << 16) | x;
    }

    // turn 0x..AA..CC..BB..DD into 0xAABBCCDD
    static uint32_t compact(uint64_t x) {
        return ((x >> 24) & 0xFF00FF00) | (x & 0xFF00FF);
    }

    static uint32_t quad_mul_div255(uint32_t x, uint8_t invA) {
        uint64_t prod = expand(x) * invA;
        prod += replicate(128);
        prod += (prod >> 8) & replicate(0xFF);
        prod >>= 8;
        return compact(prod);
    }

    static unsigned div255(unsigned x) {
        x += 128;
        return x + (x >> 8) >> 8;
    }


    // Define a new type BlendProc and create blend function pointer
    typedef GPixel (* BlendProc)(GPixel, GPixel);
    const BlendProc blend_array[12] = {&Clear, &Source, &Destination, &SourceOver, &DestinationOver, &SourceIn, &DestinationIn, &SourceOut, &DestinationOut, &SourceAtTop, &DestinationAtTop, &Xor};

    // Blend functions
    static GPixel Clear(GPixel source, GPixel destination) {
        return 0;
    }

    static GPixel Source(GPixel source, GPixel destination) {
        return source;
    }

    static GPixel Destination(GPixel source, GPixel destination) {
        return destination;
    }

    static GPixel SourceOver(GPixel source, GPixel destination) {
        int sourceAlpha = GPixel_GetA(source);
        return source + quad_mul_div255(destination, 255 - sourceAlpha);
    }

    static GPixel DestinationOver(GPixel source, GPixel destination) {
        int destAlpha = GPixel_GetA(destination);
        return destination + quad_mul_div255(source, 255 - destAlpha);
    }

    static GPixel SourceIn(GPixel source, GPixel destination) {
        int destAlpha = GPixel_GetA(destination);
        return quad_mul_div255(source, destAlpha);
    }

    static GPixel DestinationIn(GPixel source, GPixel destination) {
        int sourceAlpha = GPixel_GetA(source);
        return quad_mul_div255(destination, sourceAlpha);
    }

    static GPixel SourceOut(GPixel source, GPixel destination) {
        int destAlpha = GPixel_GetA(destination);
        return quad_mul_div255(source, 255 - destAlpha);
    }

    static GPixel DestinationOut(GPixel source, GPixel destination) {
        int sourceAlpha = GPixel_GetA(source);
        return quad_mul_div255(destination, 255 - sourceAlpha);
    }

    static GPixel SourceAtTop(GPixel source, GPixel destination) {
        int sourceAlpha = GPixel_GetA(source);
        int destAlpha = GPixel_GetA(destination);
        return quad_mul_div255(source, destAlpha) + quad_mul_div255(destination, 255 - sourceAlpha);
    }

    static GPixel DestinationAtTop(GPixel source, GPixel destination) {
        int sourceAlpha = GPixel_GetA(source);
        int destAlpha = GPixel_GetA(destination);
        return quad_mul_div255(source, 255 - destAlpha)
               + quad_mul_div255(destination, sourceAlpha);
    }

    static GPixel Xor(GPixel source, GPixel destination) {
        int sourceAlpha = GPixel_GetA(source);
        int destAlpha = GPixel_GetA(destination);
        return quad_mul_div255(source, 255 - destAlpha) + quad_mul_div255(destination, 255 - sourceAlpha);
    }


    // Blit function for the polygon
    void blit(int y, int leftX, int rightX, const GPaint& paint) {
        GPixel source = this->convertSColor(paint.getColor());
        BlendProc blender = blend_array[static_cast<int>(paint.getBlendMode())]; // Initialize a blender type object
        for (int x = leftX; x < rightX; x++) {
            GPixel* address = this->fDevice.getAddr(x, y);
            GPixel destination = *address;
            GPixel resultColor = blender(source, destination);
            *address = resultColor;
        }
    }

    // blit_shadeRow function
    void blit_shadeRow(int y, int leftX, int rightX, const GPaint& paint) {
        BlendProc proc = blend_array[static_cast<int>(paint.getBlendMode())];
        // Source
        GPixel storage[rightX - leftX];
        paint.getShader()->shadeRow(leftX, y, rightX - leftX, storage);

        int sourceIndex = 0;
        for(int x = leftX; x < rightX; x++){
            GPixel* address = this->fDevice.getAddr(x, y); // destination already present in the device’s bitmap
            GPixel destination = *address;
            GPixel resultColor = proc(storage[sourceIndex], destination); // call blend by pointer
            *address = resultColor; // change the value pointed by address
            sourceIndex += 1;
        }
    }


    // sort function
    static GPoint SortYTop(const GPoint &top, const GPoint &bottom){
        if(top.fY < bottom.fY){
            return top;
        }else{
            return bottom;
        }
    }
    static GPoint SortYBot(const GPoint &top, const GPoint &bottom){
        if(top.fY < bottom.fY){
            return bottom;
        }else{
            return top;
        }
    }
    static GPoint SortXLeft(const GPoint &left, const GPoint &right){
        if(left.fX <= right.fX){
            return left;
        }else{
            return right;
        }
    }
    static GPoint SortXRight(const GPoint &left, const GPoint &right){
        if(left.fX <= right.fX){
            return right;
        }else{
            return left;
        }
    }

    // clipping function
    void Clip(const GPoint &topPoint, const GPoint &botPoint, std::vector<Edges> &edge){
        int a = 1;
        if(topPoint.fY == botPoint.fY){
            return;
        }
        // Sort Y; topPointY: actual top point sorted accoring to Y; botPointY: actual bottom point sorted according to Y
        GPoint topPointY = SortYTop(topPoint, botPoint);
        GPoint botPointY = SortYBot(topPoint, botPoint);
        if(topPointY == topPoint){
            a = - a;
        }
        int topY = 0;
        int botY = this->fDevice.height();
        int leftX = 0;
        int rightX = this->fDevice.width();
        float m = (botPointY.fX - topPointY.fX)/(botPointY.fY - topPointY.fY);

        // Return if completely above or below the bitmap
        if(botPointY.fY <= topY || topPointY.fY >= botY){
            return;
        }

        // Chop top and bottom; since we did botPointY < top before, we know that botPointY > top; thus uppon checking topPointY < top, we can chop
        if(topPointY.fY < topY){
            float top_x = topPointY.fX + m * (topY - topPointY.fY);
            topPointY.set(top_x, topY);
        }
        if(botPointY.fY > botY){
            float bot_x = botPointY.fX + m * (botY - botPointY.fY);
            botPointY.set(bot_x, botY);
        }

        // sort X
        GPoint leftPointX = SortXLeft(topPointY, botPointY);
        GPoint rightPointX = SortXRight(topPointY, botPointY);

        int roundTopY0 = GRoundToInt(fmin(leftPointX.fY, rightPointX.fY));
        int roundBotY0 = GRoundToInt(fmax(leftPointX.fY, rightPointX.fY));

        // Return if completely left or right to the bitmap
        if(leftPointX.fX >= rightX){
            leftPointX.set(rightX, leftPointX.fY);
            rightPointX.set(rightX, rightPointX.fY);
            // Project segments onto the sides of the clip
            if(roundTopY0 != roundBotY0){
                edge.push_back(Edges(roundTopY0, roundBotY0, rightX, 0, a));
            }
        }else if(rightPointX.fX <= leftX){
            leftPointX.set(leftX, leftPointX.fY);
            rightPointX.set(leftX, rightPointX.fY);
            // Project segments onto the sides of the clip
            if(roundTopY0 != roundBotY0){
                edge.push_back(Edges(roundTopY0, roundBotY0, leftX, 0, a));
            }
        }else if(leftPointX.fX < leftX && rightPointX.fX <= rightX){
            // If left point is out of the device and right point is within device
            // Get the new Left most point (which is on the left edge of the device)
            GPoint newLeftPoint = GPoint::Make(leftX, leftPointX.fY + (leftX - leftPointX.fX)/m );
            leftPointX.set(leftX, leftPointX.fY);
            // Project segments onto the sides of the clip
            int rounTopY = GRoundToInt(fmin(leftPointX.fY, newLeftPoint.fY));
            int roundBotY = GRoundToInt(fmax(leftPointX.fY, newLeftPoint.fY));
            if(rounTopY != roundBotY){
                edge.push_back(Edges(rounTopY, roundBotY, leftX, 0, a));
            }
            // non-vertical edge
            GPoint topPointLR = SortYTop(rightPointX, newLeftPoint);
            GPoint botPointLR = SortYBot(rightPointX, newLeftPoint);
            int roundTopPointLR = GRoundToInt(topPointLR.fY);
            int roundBotPointLR = GRoundToInt(botPointLR.fY);
            if(roundTopPointLR != roundBotPointLR){
                float currentX = topPointLR.fX + m * (roundTopPointLR - topPointLR.fY + 0.5);
                edge.push_back(Edges(roundTopPointLR, roundBotPointLR, currentX, m, a));
            }
        }else if(leftPointX.fX < leftX && rightPointX.fX > rightX) {
            // If left point is out of the device and right point is also out of device
            // Get the new left most point (which is on the left edge of the device) and the right most point
            GPoint newLeftP = GPoint::Make(leftX, leftPointX.fY + (leftX - leftPointX.fX / m));
            GPoint newRightP = GPoint::Make(rightX, rightPointX.fY + (rightX - rightPointX.fX) / m);
            leftPointX.set(leftX, leftPointX.fY);
            rightPointX.set(rightX, rightPointX.fY);
            // Project left segment onto the left side of the clip
            int roundLTopY = GRoundToInt(fmin(leftPointX.fY, newLeftP.fY));
            int roundLBotY = GRoundToInt(fmax(leftPointX.fY, newLeftP.fY));
            if (roundLTopY != roundLBotY) {
                edge.push_back(Edges(roundLTopY, roundLBotY, leftX, 0, a));
            }
            // Project right segment onto the right side of the clip
            int roundRTopY0 = GRoundToInt(fmin(rightPointX.fY, newRightP.fY));
            int roundRBotY0 = GRoundToInt(fmax(rightPointX.fY, newRightP.fY));
            if (roundRTopY0 != roundRBotY0) {
                edge.push_back(Edges(roundRTopY0, roundRBotY0, rightX, 0, a));
            }
            // Sort the new left point and new right point according to their Y
            GPoint topPointLR = SortYTop(newLeftP, newRightP);
            GPoint botPointLR = SortYBot(newLeftP, newRightP);
            int rounTopPointLR = GRoundToInt(topPointLR.fY);
            int rountBotPointLR = GRoundToInt(botPointLR.fY);
            if (rounTopPointLR != rountBotPointLR) {
                float currentX = topPointLR.fX + m * (rounTopPointLR - topPointLR.fY + 0.5);
                edge.push_back(Edges(rounTopPointLR, rountBotPointLR, currentX, m, a));
            }
        }else if(leftPointX.fX >= leftX && rightPointX.fX > rightX){
            // If left point is in the device and right point is out of device
            // Get the new right most point (which is on the right edge of the device)
            GPoint newRightP0 = GPoint::Make(rightX, rightPointX.fY + (rightX - rightPointX.fX)/m );
            rightPointX.set(rightX, rightPointX.fY);
            // Project segments onto the sides of the clip
            int roundTopPointLR0 = GRoundToInt(fmin(rightPointX.fY, newRightP0.fY));
            int roundBotPointLR0 = GRoundToInt(fmax(rightPointX.fY, newRightP0.fY));
            if(roundTopPointLR0 != roundBotPointLR0){
                edge.push_back(Edges(roundTopPointLR0, roundBotPointLR0, rightX, 0, a));
            }
            // Sort the left point and new right point according to their Y
            GPoint topPointLR0 = SortYTop(leftPointX, newRightP0);
            GPoint botPointLR0 = SortYBot(leftPointX, newRightP0);
            int roundTopPointLR1 = GRoundToInt(topPointLR0.fY);
            int roundBotPointLR1 = GRoundToInt(botPointLR0.fY);
            if(roundTopPointLR1 != roundBotPointLR1){
                float currentX = topPointLR0.fX + m * (roundTopPointLR1 - topPointLR0.fY + 0.5);
                edge.push_back(Edges(roundTopPointLR1, roundBotPointLR1, currentX, m, a));
            }
        }else{
            GPoint topPointLR = SortYTop(leftPointX, rightPointX);
            GPoint botPointLR = SortYBot(leftPointX, rightPointX);
            int roundTopPointLR = GRoundToInt(topPointLR.fY);
            int roundBotPointLR = GRoundToInt(botPointLR.fY);
            if(roundTopPointLR != roundBotPointLR){
                float currentX = topPointLR.fX + m * (roundTopPointLR - topPointLR.fY + 0.5);
                edge.push_back(Edges(roundTopPointLR, roundBotPointLR, currentX, m, a));
            }
        }
    }



    void drawPaint(const GPaint& paint) override {
        int leftX = 0;
        int rightX = this->fDevice.width();
        for(int y = 0; y < this->fDevice.height(); y++){
            if(paint.getShader() == nullptr){
                blit(y, leftX, rightX, paint);
            }else{
                // If the shader fails, return with nothing
                if (!paint.getShader()->setContext(ctm)) {
                    return;
                }
                blit_shadeRow(y, leftX, rightX, paint);
           }
        }
    }


    void drawRect(const GRect& rect, const GPaint& paint) override {
        BlendProc blender = blend_array[static_cast<int>(paint.getBlendMode())]; // blend function pointer
        if(paint.getShader() != nullptr){
            if (!paint.getShader()->setContext(ctm)) {
                return;
            } // If the shader fails, return with nothing
            GPoint p1 = {rect.fLeft, rect.fTop};
            GPoint p2 = {rect.fRight, rect.fTop};
            GPoint p3 = {rect.fRight, rect.fBottom};
            GPoint p4 = {rect.fLeft, rect.fBottom};
            GPoint rect_points[4] = { p1, p2, p3, p4};
            drawConvexPolygon(rect_points, 4, paint);
        }else{
            GPoint p1 = {rect.fLeft, rect.fTop};
            GPoint p2 = {rect.fRight, rect.fTop};
            GPoint p3 = {rect.fRight, rect.fBottom};
            GPoint p4 = {rect.fLeft, rect.fBottom};
            GPoint rect_points[4] = { p1, p2, p3, p4};
            drawConvexPolygon(rect_points, 4, paint);
        }
    }

    void drawPath(const GPath& path, const GPaint& paint) override {
        // Map path by ctm
        GPath copyPath = path;
        copyPath.transform(ctm);

        std::vector<Edges> edges;
        GPath::Edger edger(copyPath);
        GPoint points[4];
        GPath::Verb nextEdge = edger.next(points);
        while(nextEdge != GPath::kDone){
            if(nextEdge == GPath::kLine){
                Clip(points[0], points[1], edges);
            }else if(nextEdge == GPath::kCubic){
                float leftX = points[0].fX - 2 * points[1].fX + points[2].fX;
                float rightX = points[1].fX - 2 * points[2].fX + points[3].fX;
                float topY = points[0].fY - 2 * points[1].fY + points[2].fY;
                float botY = points[1].fY - 2 * points[2].fY + points[3].fY;
                float leftLength = sqrtf(leftX * leftX + topY * topY);
                float rightLength = sqrtf(rightX * rightX + botY * botY);
                float a = (float) 3 / 4 * fmax(leftLength, rightLength);
                int lineSegmentCount = GCeilToInt(sqrtf(a / 0.25));
                float deltaM = (float) 1 / lineSegmentCount;

                float AX = 3 * points[1].fX + points[3].fX - points[0].fX - 3 * points[2].fX;
                float BX = 3 * (points[0].fX - 2 * points[1].fX + points[2].fX);
                float CX = 3 * (points[1].fX - points[0].fX);
                float DX = points[0].fX;

                float AY = 3 * points[1].fY + points[3].fY - points[0].fY - 3 * points[2].fY;
                float BY = 3 * (points[0].fY - 2 * points[1].fY + points[2].fY);
                float CY = 3 * (points[1].fY - points[0].fY);
                float DY = points[0].fY;

                float tile = 0.0;
                for(int i = 0; i < lineSegmentCount; i++){
                    float tile0 = tile;
                    float tile1 = tile + deltaM;
                    GPoint point0 = {((AX * tile0 + BX) * tile0 + CX) * tile0 + DX, ((AY * tile0 + BY) * tile0 + CY) * tile0 + DY};
                    GPoint point1 = {((AX * tile1 + BX) * tile1 + CX) * tile1 + DX, ((AY * tile1 + BY) * tile1 + CY) * tile1 + DY};
                    Clip(point0, point1, edges);
                    tile += deltaM;
                }
            } else if(nextEdge == GPath::kQuad){
                float AX = points[0].fX - 2 * points[1].fX + points[2].fX;
                float BX = 2 * (points[1].fX - points[0].fX);
                float CX = points[0].fX;

                float AY = points[0].fY - 2 * points[1].fY + points[2].fY;
                float BY = 2 * (points[1].fY - points[0].fY);
                float CY = points[0].fY;

                float leftX = (points[0].fX - 2 * points[1].fX + points[2].fX) / 4;
                float leftY = (points[0].fY - 2 * points[1].fY + points[2].fY) / 4;
                float a = sqrtf(leftX * leftX + leftY * leftY);

                int lineSegmentCount = GCeilToInt(sqrt(a / 0.25));
                float deltaM = (float) 1 / lineSegmentCount;
                float tile = 0.0;
                for(int j = 0; j < lineSegmentCount; j++){
                    float tile0 = tile;
                    float tile1 = tile + deltaM;
                    GPoint point0 = {(AX * tile0 + BX) * tile0 + CX, (AY * tile0 + BY) * tile0 + CY};
                    GPoint point1 = {(AX * tile1 + BX) * tile1 + CX, (AY * tile1 + BY) * tile1 + CY};
                    Clip(point0, point1, edges);
                    tile += deltaM;
                }
            }
            nextEdge = edger.next(points);
        }
        
        if(edges.empty() == false){
            // Sort the edges according Y from top to bottom, then according X from left to right, then according to slope
            std::sort(edges.begin(), edges.end());
            // Walk through the array
            int minY = FindMinY(edges);
            int maxY = FindMaxY(edges);
            int edgeAmount = edges.size();
            for(int y = minY; y < maxY; ){
                int leftX;
                int rightX;
                int edgeIndex = 0;
                int a = 0;
                Edges edge = edges.front();

                while(edge.topY <= y){ // For each edge active for this y
                    if(a == 0) {
                        leftX = GRoundToInt(edge.currentX);
                    }
                    a += edge.fW;
                    if(a == 0){
                        rightX = GRoundToInt(edge.currentX);
                        // boundary check
                        if(leftX < 0 ){
                            leftX = 0;
                        } else if(leftX > this->fDevice.width() - 1){
                            leftX = this->fDevice.width()-1; }
                        if(rightX < 0 ){
                            leftX = 0;
                        } else if( rightX > this->fDevice.width() - 1){
                            rightX = this->fDevice.width() - 1;
                        }

                        if(paint.getShader() == nullptr){ // Shader
                            blit(y, leftX, rightX, paint);
                        }else{
                            if (!paint.getShader()->setContext(ctm)) {
                                return;
                            }
                            blit_shadeRow(y, leftX, rightX, paint);
                        }
                    }
                    Edges next = Edges(0, 0, 0.0, 0.0, 0);
                    if(edgeIndex + 1 < edges.size()){
                        next = edges[edgeIndex + 1];
                    }

                    // All done with edge
                    if(edge.bottomY == y + 1){
                        std::vector<Edges>::iterator iteration = edges.begin();
                        // Remove the edge from edges
                        edges.erase(iteration + edgeIndex);
                        edgeIndex--;
                    }else{
                        edges[edgeIndex].currentX += edge.slope;
                        resortBackwards(edgeIndex, edges);
                    }
                    if(edgeIndex + 1 >= edges.size()){
                        break;
                    }else{
                        edge = next;
                        edgeIndex++; // edgeIndex => next
                    }
                }
                y  = y + 1;
                while(edge.topY == y){
                    Edges next = Edges(0, 0, 0, 0, 0);
                    if(edgeIndex + 1 < edges.size()){
                        next = edges[edgeIndex + 1];
                    }
                    resortBackwards(edgeIndex, edges);
                    if(edgeIndex + 1 >= edges.size()){
                        break;

                    }else{
                        edge = next;
                        edgeIndex++;
                    }
                }
            }
        }
    }


    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) override {
        // map points by ctm
        GPoint mapPoints[count];
        for(int i = 0; i < count; i++){
            GPoint myPoint = ctm.mapPt(points[i]);
            mapPoints[i] = myPoint;
        }
        // make edge list
        std::vector<Edges> edge;
        for(int a = 0; a < count; a++){
            int b;
            if(a == count -1 ){
                b = 0;
            }else{
                b = a + 1;
            }
            Clip(mapPoints[a], mapPoints[b], edge);
        }
        if(edge.empty() == false){
            // Sort the edges according Y from top to bottom, then according X from left to right, then according to slope
            std::sort(edge.begin(),edge.end());
            // Walk through the array
            int minY = FindMinY(edge);
            int maxY = FindMaxY(edge);
            Edges leftEdge = edge[0];
            Edges rightEdge = edge[1];
            int edgeAmount = edge.size();
            int currentEdgeIndex = 2;

            for(int y = minY; y < maxY; y++){
                // Find leftX for leftEdge
                float leftX = leftEdge.currentX;
                float rightX = rightEdge.currentX;
                // Round leftX and rightX
                int roundedLeftX = GRoundToInt(leftX);
                int roundedRightX = GRoundToInt(rightX);
                // Boundary check
                if(roundedLeftX < 0 ){
                    roundedLeftX = 0;
                } else if(roundedLeftX > this->fDevice.width()-1){
                    roundedLeftX = this->fDevice.width()-1;
                }
                if(roundedRightX < 0 ){
                    roundedLeftX = 0;
                } else if(roundedRightX > this->fDevice.width()-1){
                    roundedRightX = this->fDevice.width()-1;
                }
                
                if(paint.getShader() == nullptr){ // Use shader
                    blit(y, roundedLeftX, roundedRightX, paint);
                }else{
                    // If the shader fails, return nothing
                    if (!paint.getShader()->setContext(ctm)) {
                        return;
                    }
                    blit_shadeRow(y, roundedLeftX, roundedRightX, paint);
                }
                // Increment x
                leftEdge.currentX += leftEdge.slope;
                rightEdge.currentX += rightEdge.slope;
                // Switch edges
                if(leftEdge.bottomY <= y){
                         leftEdge = edge[currentEdgeIndex];
                         currentEdgeIndex++;
                }
                if(rightEdge.bottomY <= y){
                         rightEdge = edge[currentEdgeIndex];
                         currentEdgeIndex++;
                }
            }
        }
    }
    
    void clear(const GColor& color) {
		GPaint paint(color);
		paint.setBlendMode(GBlendMode::kSrc);
		this->drawPaint(paint);
	}

	void fillRect(const GRect& rect, const GColor& color) {
		this->drawRect(rect, GPaint(color));
	}

	GMatrix computeBasis(const GPoint p0, const GPoint p1, const GPoint p2){
        GMatrix matrix(p1.fX-p0.fX, p2.fX-p0.fX, p0.fX,
                       p1.fY-p0.fY, p2.fY-p0.fY, p0.fY);
        return matrix;
    }
/*
    void drawTriangle(const GPoint pts[3], const GPaint& paint){
        if(paint.getShader() != nullptr){
            if (!paint.getShader()->setContext(ctm)) {
                return;
            } // If the shader fails, return with nothing
            drawConvexPolygon(pts, 3, paint);
        }else{
            drawConvexPolygon(pts, 3, paint);
        }
    }
*/
    /*
    void drawTriangle(const GPoint vertices[3], const GPaint& paint) {
        this->drawConvexPolygon(vertices, 3, paint);
    }

    */

    void drawTriangleWithTex(const GPoint pts[],
                              const GPoint tex[],
                              const GPaint& paint) {
        GMatrix P, T, invT;

        P = computeBasis(pts[0], pts[1], pts[2]);
        T = computeBasis(tex[0], tex[1], tex[2]);

        T.invert(&invT);

        GMatrix temp;
        temp.setConcat(P, invT);

        ProxyShader proxy(paint.getShader(), temp);

        this->drawConvexPolygon(pts, 3, GPaint(&proxy));
    }

    void drawTriangleWithColor(const GPoint pts[],
                             const GColor colors[],
                             const GPaint& paint) {

        TriColorShader colorShader(pts, colors);
        // this->drawTriangle(pts, GPaint(&colorShader));
        this->drawConvexPolygon(pts, 3, GPaint(&colorShader));
    }

    void drawTriangleWithBoth(const GPoint pts[], const GColor colors[], const GPoint texs[], const GPaint& paint) {

        GMatrix P, T, invT;
        P = computeBasis(pts[0], pts[1], pts[2]);
        T = computeBasis(texs[0], texs[1], texs[2]);

        T.invert(&invT);
        GMatrix temp;
        temp.setConcat(P, invT);

        TriColorShader color(pts,colors);
        ProxyShader proxy(paint.getShader(), temp);

        ComposeShader compShader(&color, &proxy);

        //this->drawTriangle(pts, GPaint(&compShader));
        this->drawConvexPolygon(pts, 3, GPaint(&compShader));

    }


    /**
         *  Draw a mesh of triangles, with optional colors and/or texture-coordinates at each vertex.
         *
         *  The triangles are specified by successive triples of indices.
         *      int n = 0;
         *      for (i = 0; i < count; ++i) {
         *          point0 = vertx[indices[n+0]]
         *          point1 = verts[indices[n+1]]
         *          point2 = verts[indices[n+2]]
         *          ...
         *          n += 3
         *      }
         *
         *  If colors is not null, then each vertex has an associated color, to be interpolated
         *  across the triangle. The colors are referenced in the same way as the verts.
         *          color0 = colors[indices[n+0]]
         *          color1 = colors[indices[n+1]]
         *          color2 = colors[indices[n+2]]
         *
         *  If texs is not null, then each vertex has an associated texture coordinate, to be used
         *  to specify a coordinate in the paint's shader's space. If there is no shader on the
         *  paint, then texs[] should be ignored. It is referenced in the same way as verts and colors.
         *          texs0 = texs[indices[n+0]]
         *          texs1 = texs[indices[n+1]]
         *          texs2 = texs[indices[n+2]]
         *
         *  If both colors and texs[] are specified, then at each pixel their values are multiplied
         *  together, component by component.
         */
        virtual void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[],
                              int count, const int indices[], const GPaint& paint){

            int n = 0;
            for (int i = 0; i < count; ++i) {
                if (texs != nullptr && colors == nullptr) {
                    GPoint point0 = verts[indices[n + 0]];
                    GPoint point1 = verts[indices[n + 1]];
                    GPoint point2 = verts[indices[n + 2]];

                    GPoint pts1[] = {point0, point1, point2};

                    GPoint texs0 = texs[indices[n + 0]];
                    GPoint texs1 = texs[indices[n + 1]];
                    GPoint texs2 = texs[indices[n + 2]];

                    GPoint texture[] = {texs0, texs1, texs2};

                    drawTriangleWithTex(pts1, texture, paint);
                } else if (colors != nullptr && texs == nullptr) {
                    //TODO: draw triangle with colors

                    GPoint point0 = verts[indices[n+0]];
                    GPoint point1 = verts[indices[n+1]];
                    GPoint point2 = verts[indices[n+2]];

                    GPoint pts2[] = {point0, point1, point2};

                    GColor color0 = colors[indices[n+0]];
                    GColor color1 = colors[indices[n+1]];
                    GColor color2 = colors[indices[n+2]];

                    GColor clr[] = {color0, color1, color2};

                    drawTriangleWithColor(pts2, clr, paint);
                } else if(colors != nullptr && texs != nullptr) {
                    //TODO: draw triangle with colors and texs
                    GPoint point0 = verts[indices[n + 0]];
                    GPoint point1 = verts[indices[n + 1]];
                    GPoint point2 = verts[indices[n + 2]];

                    GPoint pts3[] = {point0, point1, point2};

                    GColor color0 = colors[indices[n + 0]];
                    GColor color1 = colors[indices[n + 1]];
                    GColor color2 = colors[indices[n + 2]];

                    GColor clr2[] = {color0, color1, color2};

                    GPoint texs0 = texs[indices[n + 0]];
                    GPoint texs1 = texs[indices[n + 1]];
                    GPoint texs2 = texs[indices[n + 2]];

                    GPoint texture2[] = {texs0, texs1, texs2};

                    drawTriangleWithBoth(pts3, clr2, texture2, paint);
                } else {
                    // Draw nothing
                }
                n+=3;
            }
        }

    GPoint makePoint(const GPoint *pts, float u, float v) {
        GPoint p0 = pts[0];
        GPoint p1 = pts[1];
        GPoint p2 = pts[2];
        GPoint p3 = pts[3];

        float x = (1-u)*(1-v)*p0.fX + (1-v)*u*p1.fX + (1-u)*v*p3.fX + u*v*p2.fX;
        float y = (1-u)*(1-v)*p0.fY + (1-v)*u*p1.fY + (1-u)*v*p3.fY + u*v*p2.fY;
        return GPoint::Make(x, y);
    }

    GColor makeColor(const GColor clr[], float u, float v) {
        GColor c0 = clr[0];
        GColor c1 = clr[1];
        GColor c2 = clr[2];
        GColor c3 = clr[3];

        float alpha = (1-u)*(1-v)*c0.fA + (1-v)*u*c1.fA + (1-u)*v*c3.fA + u*v*c2.fA;
        float red = (1-u)*(1-v)*c0.fR + (1-v)*u*c1.fR + (1-u)*v*c3.fR + u*v*c2.fR;
        float green = (1-u)*(1-v)*c0.fG + (1-v)*u*c1.fG + (1-u)*v*c3.fG + u*v*c2.fG;
        float blue = (1-u)*(1-v)*c0.fB + (1-v)*u*c1.fB + (1-u)*v*c3.fB + u*v*c2.fB;
        return GColor::MakeARGB(alpha, red, green, blue);
    }


        /**
         *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
         *  the quad based on "level":
         *      level == 0 --> 1 quad  -->  2 triangles
         *      level == 1 --> 4 quads -->  8 triangles
         *      level == 2 --> 9 quads --> 18 trianglesa
         *      ...
         *  The 4 corners of the quad are specified in this order:
         *      top-left --> top-right --> bottom-right --> bottom-left
         *  Each quad is triangulated on the diagonal top-right --> bottom-left
         *      0---1
         *      |  /|
         *      | / |
         *      |/  |
         *      3---2
         *
         *  colors and/or texs can be null. The resulting triangles should be passed to drawMesh(...).
         */
        virtual void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4],
                              int level, const GPaint& paint) {
            /*GPoint p0 = verts[0];
            GPoint p1 = verts[1];
            GPoint p2 = verts[2];
            GPoint p3 = verts[3];

            for (int i = 0; i < level + 1; i++) {
                for (int j = 0; j < level + 1; j++) {

                }
            }*/

            int indices[6] = {0, 1, 3, 3, 2, 1};
            GPoint updatedVerts[4];
            GColor updatedColor[4];
            GPoint updatedTexs[4];

            enum{
                containsTex = 1 << 1,
                containsColor = 1 << 0,
            };

            int flag = 0;

            if(texs){
                flag |= containsTex;
            }
            if (colors){
                flag |= containsColor;
            }


            float stride = 1.0 / (level + 1);
            float u;
            float v = 0.0;

            bool updateTexs;
            bool updateColor;

            while (v < 1){
                u = 0.0;
                while (u < 1){
                    updateTexs = false;
                    updateColor = false;

                    updatedVerts[0] = makePoint(verts, u, v);
                    updatedVerts[1] = makePoint(verts, u + stride, v);
                    updatedVerts[2] = makePoint(verts, u + stride, v + stride);
                    updatedVerts[3] = makePoint(verts, u, v + stride);

                    if (flag & containsTex){
                        updatedTexs[0] = makePoint(texs, u, v);
                        updatedTexs[1] = makePoint(texs, u + stride, v);
                        updatedTexs[2] = makePoint(texs, u + stride, v + stride);
                        updatedTexs[3] = makePoint(texs, u, v + stride);

                        updateTexs = true;
                    }

                    if (flag & containsColor){
                        updatedColor[0] = makeColor(colors, u, v);
                        updatedColor[1] = makeColor(colors, u + stride, v);
                        updatedColor[2] = makeColor(colors, u + stride, v + stride);
                        updatedColor[3] = makeColor(colors, u, v + stride);
                        updateColor = true;
                    }

                    if (updateTexs == true && updateColor == true){
                        this->drawMesh(updatedVerts, updatedColor, updatedTexs, 2, indices, paint);
                    } else if (updateTexs == true) {
                        this->drawMesh(updatedVerts, colors, updatedTexs, 2, indices, paint);
                    } else {
                        this->drawMesh(updatedVerts, updatedColor, texs, 2, indices, paint);
                    }

                    u += stride;
                }
                v += stride;
            }
        }



private:
    GBitmap fDevice;
    GMatrix ctm = GMatrix();
};

void GDrawSomething_polys(GCanvas* canvas){

    float colorRed = 0.95;
    float colorGreen = 0.55;
    float colorBlue = 0.55;
    float alphaVal = 0.45;

    const GPoint pts[] {
            /*
            { 0, 0 }, { -1, 5 }, { 0, 6 }, { 1, 5 },
            */
            { 1, 1.1f }, { -2, 6 }, { 0, 6 }, { 2, 6 },
    };

    canvas->translate(250, 250);
    canvas->scale(35, 35);

    float steps = 18;
    float step = 1 / (steps - 1);

    GPaint paint;
    std::unique_ptr<GShader> myShader;
/*
    const GBitmap* tex = nullptr;
    if (tex) {
        shader = GCreateBitmapShader(*tex, GMatrix::MakeScale(50, 50));
        paint.setShader(shader.get());
    }
*/
    for (float turn = 0; turn < 2 * M_PI - 0.001f; turn += 2 * M_PI/steps) {
        paint.setColor({ alphaVal, colorRed, colorGreen, colorBlue });
        canvas->save();
        canvas->rotate(turn);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
        colorRed -= 0.02;
        colorGreen -= 0.08;
        colorBlue = colorBlue - 0.02;
        alphaVal = alphaVal * 1.1;
    }
}

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    if (!device.pixels()) {
        return nullptr;
    }
    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}