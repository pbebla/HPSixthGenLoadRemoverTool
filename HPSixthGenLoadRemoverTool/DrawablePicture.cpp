#include "DrawablePicture.h"

DrawablePicture::DrawablePicture() {
    selRect = Rect();
    baseBmp = new Bitmap(100,100,PixelFormat32bppRGB);
}
