bool processExplosion(FloatingItem &e, float simTime) {
    e.age += simTime;
    e.frame = e.age * 16.0 * 8.0;

    if(e.age > 0.125) {
        return true;
    }
    return false;
}
