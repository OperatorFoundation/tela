//
// Created by Dr. Brandon Wiley on 5/29/25.
//

#include "soft_cursor.h"

SoftCursor::SoftCursor(Clock& clock, Canvas& canvas, Logger& logger) : Cursor(clock), canvas(canvas), logger(logger) {}

void SoftCursor::show()
{
  logger.debugf("SoftCursor::show@(%d,%d)", col, row);

  int char_width = canvas.getCharWidth();
  int char_height = canvas.getCharHeight();
  int pixel_x = col * char_width;
  int pixel_y = row * char_height;
  uint16_t color = canvas.getFgColor();

  switch(shape)
  {
    case block:
      canvas.fillRect(pixel_x, pixel_y, char_width, char_height, color);
      break;

    case underline:
    case underline_steady:
      canvas.fillRect(pixel_x, pixel_y + char_height - 2, char_width, 2, color);
      break;

    case bar:
    case bar_steady:
    case bar_blinking:
      canvas.fillRect(pixel_x, pixel_y, 2, char_height, color);
      break;
  }
}

void SoftCursor::hide()
{
  logger.debugf("SoftCursor::hide@(%d,%d)", col, row);

  int char_width = canvas.getCharWidth();
  int char_height = canvas.getCharHeight();
  int pixel_x = col * char_width;
  int pixel_y = row * char_height;

  // Always clear the cursor rectangle first
  canvas.clearRect(pixel_x, pixel_y, char_width, char_height);

  if(screen)
  {
    VTermScreenCell cell;
    VTermPos pos = {.row = row, .col = col};

    if(force_invisible)
    {
      pos = {.row = row - 1, .col = col};
    }

    int result = vterm_screen_get_cell(screen, pos, &cell);

    if(result && cell.chars[0])
    {
      logger.debugf("SoftCursor::hide - redrawing char: %c", cell.chars[0]);
      canvas.drawCharacter(col, row, cell.chars[0]);
    }
  }
}