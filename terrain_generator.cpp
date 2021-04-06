#include "terrain_generator.hpp"

using namespace std;

namespace TerrainGenerator {

    const int sliceWidth = 600;

    vector<Single2DGrid> slices;



    int currentSlice, nextSlice;
    int sliceProgress;
    int currentOffset;
    int nextOffset;




    void init(const Single2DGrid& flag) {


        auto blurredFlag = DoubleBuffered2DGrid(flag.width, flag.height);

        for(int y = 0; y < flag.height; y++) {
            for(int x = 0; x < flag.width; x++) {
                blurredFlag.f(x,y) = flag(x,y);
                blurredFlag.b(x,y) = flag(x,y);
            }
        }

        for(int y = 0; y < flag.height; y++) {
            blurredFlag.b(0,y) = 1.0;
            blurredFlag.b(flag.width-1,y) = 1.0;
            blurredFlag(0,y) = 1.0;
            blurredFlag(flag.width-1,y) = 1.0;
        }
        for(int i = 0; i < 1000; i++) {
            for(int y = 1; y < flag.height-1; y++) {
                for(int x = 1; x < flag.width-1; x++) {
                    blurredFlag.b(x,y) = 0.5f * blurredFlag.f(x,y) + 0.125f * (blurredFlag.f(x+1,y) + blurredFlag.f(x-1,y) + blurredFlag.f(x,y+1) +blurredFlag.f(x,y-1));
                }
            }
            blurredFlag.swap();
        }
        for(int s = 0; s < 40; s++) {
            int xdir = rand()%2;
            int ydir = rand()%2;
            auto newSlice = Single2DGrid(sliceWidth, flag.height);
            int sliceStart = rand()%(flag.width - sliceWidth);
            for(int y = 0; y < flag.height; y++) {
                for(int x = 0; x < sliceWidth; x++) {
                    int sliceX = xdir ? x + sliceStart : flag.width -x + sliceStart;
                    int sliceY = ydir ? y : flag.height-y-1;
                    newSlice(x,y) = 0.5f *blurredFlag(sliceX, sliceY) + 0.5f * flag(sliceX, sliceY);
                }
            }

            slices.push_back(newSlice);

        }
        currentSlice = -1;
        nextSlice = rand() % slices.size();
        sliceProgress = 0;
        currentOffset = rand() % flag.height;
        nextOffset = rand() % flag.height;
    }

    std::vector<float> generateLine(const Single2DGrid& flag){


        std::vector<float> lastLine(flag.height);
        if(currentSlice == -1) {
            for(int y = 0; y < flag.height; y++) {
                lastLine[y] = flag(flag.width-1,y);
            }
            for(int i = 0; i < 6; i++) {
                for(int y = 1; y < flag.height-1; y++) {
                    lastLine[y] = 0.25f*lastLine[y-1] + 0.5f*lastLine[y] + 0.25f*lastLine[y+1];
                }
            }
        }

        std::vector<float> newLine(flag.height);
        float mix = (float) sliceProgress / sliceWidth * 2;
        float density = 0.0f;
        for(int y = 0; y < flag.height; y++) {

            float val = slices[nextSlice](sliceProgress, (y+nextOffset) % flag.height) * mix;

            if(currentSlice == -1) {
                val += lastLine[y] * (1.0f-mix);
            } else {
                val += slices[currentSlice](sliceProgress + sliceWidth/2, (y+currentOffset) % flag.height) * (1.0f-mix);
            }


            val -= (float) 10.0f / y;
            val -= (float) 10.0f / (flag.height-y);

            newLine[y] = val;
            density += val;
        }

        auto values = newLine;
        sort(begin(values), end(values));
        float threshold = values[values.size()*0.35] * 0.5 + 0.2;

        for(int y = 0; y < flag.height; y++) {
            newLine[y] = newLine[y] > threshold ? 1.0 : 0.0;
        }


        sliceProgress++;
        if(sliceProgress >= sliceWidth/2) {
            sliceProgress = 0;
            currentSlice = nextSlice;
            nextSlice = rand() % slices.size();
            currentOffset = nextOffset;
            nextOffset = rand() % flag.height;
        }
        return newLine;
    }

}
