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

  if(screen)
  {
    logger.debugf("SoftCursor::hide -  screen", col, row);

    VTermScreenCell cell;
    VTermPos pos = {.row = row - 1, .col = col - 1};

    int result = vterm_screen_get_cell(screen, pos, &cell);
    logger.debugf("SoftCursor::hide (%d, %d), result: %d - got screen cell result", col, row, result);
    if(result && cell.chars[0])
    {
      logger.debugf("SoftCursor::hide - got cell char: %d", cell.chars[0]);
      logger.debugf("SoftCursor::hide@(%d,%d)", col, row);
      canvas.drawCharacter(col, row, cell.chars[0]);
      return;
    }
  }

  // Fallback case if anything above fails
  logger.debugf("SoftCursor::hide fallback", col, row);
  canvas.drawCharacter(col, row, ' ');
}