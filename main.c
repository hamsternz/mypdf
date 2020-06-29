#include <stdio.h>
#include <stdint.h>
#include "mypdf.h"

char *fname = "my.pdf";

static void marker(struct pdf_file *pdf, int x, int y, int dx, int dy) {
   mypdf_path_moveto(pdf,    0*dx+x,   20*dy+y);
   mypdf_path_lineto(pdf,    0*dx+x,   30*dy+y);
   mypdf_path_stroke(pdf);

   mypdf_path_moveto(pdf,    0*dx+x,   25*dy+y);
   mypdf_path_lineto(pdf,   10*dx+x,   25*dy+y);
   mypdf_path_lineto(pdf,   25*dx+x,   10*dy+y);
   mypdf_path_lineto(pdf,   25*dx+x,    0*dy+y);
   mypdf_path_stroke(pdf);

   mypdf_path_moveto(pdf,   20*dx+x,    0*dy+y);
   mypdf_path_lineto(pdf,   30*dx+x,    0*dy+y);
   mypdf_path_stroke(pdf);

   mypdf_path_moveto(pdf,    0*dx+x,    0*dy+y);
   mypdf_path_lineto(pdf,   10*dx+x,   10*dy+y);
   mypdf_path_stroke(pdf);
}

int main(int argc, char *argv[]) {
   struct pdf_file *pdf;
   int w,h;
   int font_id;
   pdf = mypdf_open(fname);
   if(pdf == NULL) {
      fprintf(stderr,"Cannot open file\n");
      return 0;
   } 
   font_id = mypdf_add_font(pdf);

   mypdf_page_begin(pdf);
   mypdf_page_get_size(pdf, &w, &h);

   marker(pdf, 0, 0, 1, 1);
   marker(pdf, w, 0,-1, 1);
   marker(pdf, w, h,-1,-1);
   marker(pdf, 0, h, 1,-1);

   if(font_id >= 0) {
     mypdf_add_text(pdf, font_id, 200, 200, "Hello World");
  }

   mypdf_page_end(pdf);

   mypdf_close(pdf);
   return 0;
}
