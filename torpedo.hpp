#include "floating_item.hpp"

void processTorpedo(FloatingItem &t, FloatingItem *targetBegin,
                    FloatingItem *targetEnd) {

    t.force = glm::vec2(cos(t.rotation), sin(t.rotation))*4.0f;

}
