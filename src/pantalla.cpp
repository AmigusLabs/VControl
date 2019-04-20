#include "pantalla.h"

void pantallaBienvenida(U8G2 u8g2)
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_logisoso16_tr);
    u8g2.drawStr(5, 38, "@migus Labs");
    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(18, 54, "Power Module v4A");

  } while (u8g2.nextPage());

  delay(300);
}