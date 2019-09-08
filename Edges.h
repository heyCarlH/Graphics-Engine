
#ifndef GEdge_DEFINED
#define GEdge_DEFINED

struct Edges {

    int topY;
    int bottomY;
    float currentX;
    float slope; 
    int fW;

    Edges(int top, int bot, float x, float m, int w) :topY(top), bottomY(bot), currentX(x), slope(m), fW(w){}

    // Overload operator < for edge comparison
    bool operator<(const Edges &anotherEdge) const {
        if (topY == anotherEdge.topY) {
            if (currentX == anotherEdge.currentX) {
                if (slope < anotherEdge.slope) {
                    return true;
                } else {
                    return false;
                }
            } else if (currentX < anotherEdge.currentX) {
                return true;
            } else {
                return false;
            }
        } else if (topY < anotherEdge.topY) {
            return true;
        } else {
            return false;
        }

    }

};

#endif