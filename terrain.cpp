#include "terrain.hpp"

float Terrain::subSample(int ix, int iy) {
  float sum = 0.0;
  for (int sy = 0; sy < scale; sy++) {
    for (int sx = 0; sx < scale; sx++) {
      sum += flagFullRes(ix * scale + sx, iy * scale + sy);
    }
  }
  return (sum / scale / scale) > 0.99f ? 1.0f : 0.0f;
}

struct rgba {
  unsigned char r, g, b, a;
  bool operator==(const rgba &o) {
    return r == o.r && g == o.g && b == o.b && a == o.a;
  }
};

Terrain::Terrain(std::string filename, int scale) : scale(scale) {

  std::vector<unsigned char> image;
  unsigned int image_width, image_height;

  unsigned error = lodepng::decode(image, image_width, image_height, filename);
  rgba *rgba_image = reinterpret_cast<rgba *>(image.data());

  if (error)
    std::cout << "decoder error " << error << ": " << lodepng_error_text(error)
              << std::endl;

  flagFullRes = {(int)image_width, (int)image_height};
  flagSimRes = {(int)image_width / scale, (int)image_height / scale};

  for (int y = 0; y < (int)image_height; y++) {
    for (int x = 0; x < (int)image_width; x++) {
      int idx = (image_height - y - 1) * image_width + x;
      flagFullRes(x, y) = rgba_image[idx].r / 255.0f;
    }
  }

  for (int y = 0; y < flagSimRes.height; y++) {
    for (int x = 0; x < flagSimRes.width; x++) {
      flagSimRes(x, y) = subSample(x, y);
    }
  }

  init(flagFullRes);
}

void Terrain::init(const Single2DGrid &flag) {

  auto blurredFlag = DoubleBuffered2DGrid(flag.width, flag.height);

  for (int y = 0; y < flag.height; y++) {
    for (int x = 0; x < flag.width; x++) {
      blurredFlag.f(x, y) = flag(x, y);
      blurredFlag.b(x, y) = flag(x, y);
    }
  }

  for (int y = 0; y < flag.height; y++) {
    blurredFlag.b(0, y) = 1.0;
    blurredFlag.b(flag.width - 1, y) = 1.0;
    blurredFlag(0, y) = 1.0;
    blurredFlag(flag.width - 1, y) = 1.0;
  }
  for (int i = 0; i < 1000; i++) {
    for (int y = 1; y < flag.height - 1; y++) {
      for (int x = 1; x < flag.width - 1; x++) {
        blurredFlag.b(x, y) =
            0.5f * blurredFlag.f(x, y) +
            0.125f * (blurredFlag.f(x + 1, y) + blurredFlag.f(x - 1, y) +
                      blurredFlag.f(x, y + 1) + blurredFlag.f(x, y - 1));
      }
    }
    blurredFlag.swap();
  }
  for (int s = 0; s < 40; s++) {
    int xdir = rand() % 2;
    int ydir = rand() % 2;
    auto newSlice = Single2DGrid(sliceWidth, flag.height);
    int sliceStart = rand() % (flag.width - sliceWidth - 1);
    for (int y = 0; y < flag.height; y++) {
      for (int x = 0; x < sliceWidth; x++) {
        int sliceX = xdir ? x + sliceStart : sliceStart + sliceWidth - x - 1;
        int sliceY = ydir ? y : flag.height - y - 1;
        newSlice(x, y) =
            0.5f * blurredFlag(sliceX, sliceY) + 0.5f * flag(sliceX, sliceY);
      }
    }

    slices.push_back(newSlice);
  }

  auto newSlice = Single2DGrid(sliceWidth, flag.height);
  newSlice.fill(1.0f);
  slices.push_back(newSlice);
  slices.push_back(newSlice);

  currentSlice = -1;
  nextSlice = rand() % slices.size();
  sliceProgress = 0;
  currentOffset = rand() % flag.height;
  nextOffset = rand() % flag.height;
}

void Terrain::shiftMap() {
  for (int y = 0; y < flagFullRes.height; y++) {
    for (int x = 0; x < flagFullRes.width - scale; x++) {
      flagFullRes(x, y) = flagFullRes(x + scale, y);
    }
  }
  for (int y = 0; y < flagSimRes.height; y++) {
    for (int x = 0; x < flagSimRes.width - 1; x++) {
      flagSimRes(x, y) = flagSimRes(x + 1, y);
    }
  }
  for (int s = 0; s < scale; s++) {
    auto line = generateLine(flagFullRes);
    for (int y = 0; y < flagFullRes.height; y++) {
      flagFullRes(flagFullRes.width - scale + s, y) = line[y];
    }
  }

  for (int i = 0; i < 10; i++) {
    for (int xs = 0; xs < 4; xs++) {
      for (int y = 0; y < flagFullRes.height; y++) {
        int x = flagFullRes.width - 3 - xs;
        float curvature =
            0.25f * (flagFullRes(x + 1, y) + flagFullRes(x, y + 1) +
                     flagFullRes(x - 1, y) + flagFullRes(x, y - 1)) -
            flagFullRes(x, y);
        float maxCurvature = 0.02f;
        if (abs(curvature) > maxCurvature)
          flagFullRes(x, y) =
              0.25f * (flagFullRes(x + 1, y) + flagFullRes(x, y + 1) +
                       flagFullRes(x - 1, y) + flagFullRes(x, y - 1)) -
              glm::sign(curvature) * maxCurvature;
      }
    }
  }

  for (int y = 0; y < flagSimRes.height; y++) {
    float sum = 0.0;
    for (int sy = 0; sy < scale; sy++) {
      for (int sx = 0; sx < scale; sx++) {
        sum += flagFullRes(flagFullRes.width - scale + sx, y * scale + sy);
      }
    }
    flagSimRes(flagSimRes.width - 1, y) =
        sum / (scale * scale) > 0.5f ? 1.0f : 0.0f;
  }
}

std::vector<float> Terrain::generateLine(const Single2DGrid &flag) {

  std::vector<float> lastLine(flag.height);
  if (currentSlice == -1) {
    for (int y = 0; y < flag.height; y++) {
      lastLine[y] = flag(flag.width - 1, y);
    }
    for (int i = 0; i < 6; i++) {
      for (int y = 1; y < flag.height - 1; y++) {
        lastLine[y] = 0.25f * lastLine[y - 1] + 0.5f * lastLine[y] +
                      0.25f * lastLine[y + 1];
      }
    }
  }

  std::vector<float> newLine(flag.height);
  float mix = (float)sliceProgress / sliceWidth * 2;
  float density = 0.0f;
  for (int y = 0; y < flag.height; y++) {

    float val =
        slices[nextSlice](sliceProgress, (y + nextOffset) % flag.height) * mix;

    if (currentSlice == -1) {
      val += lastLine[y] * (1.0f - mix);
    } else {
      val += slices[currentSlice](sliceProgress + sliceWidth / 2,
                                  (y + currentOffset) % flag.height) *
             (1.0f - mix);
    }

    val -= (float)10.0f / y;
    val -= (float)10.0f / (flag.height - y);

    newLine[y] = val;
    density += val;
  }

  auto values = newLine;
  sort(begin(values), end(values));
  float threshold = values[values.size() * 0.35] * 0.5 + 0.1;

  for (int y = 0; y < flag.height; y++) {
    newLine[y] = newLine[y] > threshold ? 1.0 : 0.0;
  }

  sliceProgress++;
  if (sliceProgress >= sliceWidth / 2) {
    sliceProgress = 0;
    currentSlice = nextSlice;
    nextSlice = rand() % slices.size();
    currentOffset = nextOffset;
    nextOffset = rand() % flag.height;
  }
  return newLine;
}

void Terrain::drawCircle(glm::vec2 ic, int diam, float val) {

  for (int y = -diam; y <= diam; y++) {
    for (int x = -diam; x <= diam; x++) {
      if (x * x + y * y > diam * diam || ic.x + x < 0 ||
          x + ic.x > flagFullRes.width || y + ic.y < 2 ||
          y + ic.y >= flagFullRes.height - 3)
        continue;

      flagFullRes(x + ic.x, y + ic.y) = val;
    }
  }

  int scaledDiam = (diam - 1) / scale + 1;

  for (int y = -scaledDiam; y <= scaledDiam; y++) {
    for (int x = -scaledDiam; x <= scaledDiam; x++) {
      flagSimRes(ic.x / scale + x, ic.y / scale + y) =
          subSample(ic.x / scale + x, ic.y / scale + y);
    }
  }
}
