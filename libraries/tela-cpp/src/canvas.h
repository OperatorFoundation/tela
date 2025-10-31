//
// Created by Dr. Brandon Wiley on 5/13/25.
//

#ifndef CANVAS_H
#define CANVAS_H

#include <cstdint>

#include "colors.h"

class Canvas
{
  public:
    virtual ~Canvas() = default;

    // In charcters
    virtual uint16_t getHeight() = 0;
    virtual uint16_t getWidth() = 0;

    // Coordinates are in characters
    virtual void drawCharacter(int16_t x, int16_t y, char c) = 0;
    virtual void fill(uint16_t color) = 0;
    virtual void setFont(int fontNumber) = 0;

    virtual int getCharWidth() = 0;
    virtual int getCharHeight() = 0;
    virtual uint16_t getFgColor() = 0;
    virtual void clearRect(int x, int y, int w, int h) = 0;
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;

    virtual int move(int sourceStartX, int sourceStartY, int sourceEndX, int sourceEndY, int destStartX, int destStartY, int destEndX, int destEndY) = 0;
};

#endif //CANVAS_H
