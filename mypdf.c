#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "mypdf.h"

struct Xref {
   struct Xref *next;
   int object;
   int address;
   int generation;
};

struct Page {
   struct Page *next;
   int page_obj_id;
   int content_obj_id;
   int width72dpi;
   int height72dpi;
   char *data;
   size_t data_size;
   size_t used_size;
};

struct Font {
   struct Font *next;
   int font_id;
   int font_obj_id;
   char *data;
};

struct pdf_file {
   FILE *f;
   int next_obj_id;
   int page_obj_id;
   size_t xref_offset;
   struct Xref *first_xref;
   struct Page *first_page;
   struct Font *first_font;
   struct Page *last_page;
   int catalog_obj_id;
   int outline_obj_id;
   int pages_obj_id;
   int end_obj_id;
};

/*******************************************************************************/
int page_data_grow(struct Page *p, size_t new_size) { 
   char *d;
   if(new_size < 1) {
     new_size = 1;
   }

   if(p->data == NULL) {
     p->data = malloc(new_size);
     if(p->data == NULL)
        return 0;
     p->data_size = new_size;
     p->used_size = 0;
     p->data[0] = 9;
     return 1;
   }

   if(new_size < p->data_size) {
      return 0;
   }

   printf("Grow data\n");
   d = realloc(p->data, new_size);
   if(d == NULL)
      return 0;
   p->data = d;
   p->data_size = new_size;
   return 1;
}

/*******************************************************************************/
int page_add_data(struct Page *p, char *data, size_t len) {
   if(len == 0)
      return 0;

   if(p->used_size+len < p->data_size)
   {
     strcpy(p->data+p->used_size, data);
     p->used_size += len;
     return 1;
   }

   int new_len = ((p->used_size+len+1+1023)/1024)*1024;
   if(!page_data_grow(p, new_len)) {
     return 0;
   }
   strcpy(p->data+p->used_size, data);
   p->used_size += len;
   return 1;
}
/*******************************************************************************/
void  add_xref(struct pdf_file *pdf, int object, int generation) {
   struct Xref *n, *c;
   n = malloc(sizeof(struct Xref));
   if(n == NULL) {
     fprintf(stderr,"Out of mem\n");
     return;
   }
   n->address    = ftell(pdf->f);  
   n->object     = object;
   n->generation = generation;
   n->next       = NULL;
   if(pdf->first_xref == NULL) {
      pdf->first_xref = n;
      return;
   } 
   c = pdf->first_xref;
   while(c->next != NULL) {
      c = c->next;
   }   
   c->next    = n;
}

/*******************************************************************************/
static int next_obj_id(struct pdf_file *pdf) {
   pdf->next_obj_id++;
   return pdf->next_obj_id;
}

/*******************************************************************************/
int mypdf_add_font(struct pdf_file *pdf) {
   struct Font *f;
   int id = 1;
  
   f = malloc(sizeof(struct Font));
   memset(f, 0, sizeof(struct Font));
   f->font_id = id;

   struct Font *c = pdf->first_font;
   if(c == NULL) {
      pdf->first_font = f;
      return id; 
   } 
   while(c->next != NULL) {
      id++;
      c = c->next;
   }
   c->next = f;
   return id;
}

/*******************************************************************************/
int  mypdf_page_begin(struct pdf_file *pdf) {
   struct Page *p;
  
   p = malloc(sizeof(struct Page));
   if(p == NULL) {
      return 0;
   }
   memset(p, 0, sizeof(struct Page));
   p->width72dpi = 570;
   p->height72dpi = 800;
   page_data_grow(p,1024);

   pdf->last_page = p;

   struct Page *c = pdf->first_page;
   if(c == NULL) {
      pdf->first_page = p;
      return 1; 
   } 

   while(c->next != NULL) {
      c = c->next;
   }
   c->next = p;
   return 1;
}

/*******************************************************************************/
int mypdf_page_set_size(struct pdf_file *pdf, int32_t width,  int32_t height) {
   if(pdf->last_page == NULL) {
      return 0;
   }
   pdf->last_page->width72dpi  = width;
   pdf->last_page->height72dpi = height;
   return 1;
}

/*******************************************************************************/
int mypdf_page_get_size(struct pdf_file *pdf, int32_t *width, int32_t *height) {
   if(pdf->last_page == NULL) {
      return 0;
   }
   *width  = pdf->last_page->width72dpi;
   *height = pdf->last_page->height72dpi;
   return 1;
}

/*******************************************************************************/
void  mypdf_path_moveto(struct pdf_file *pdf, int32_t x, int32_t y) {
   char buffer[20];
   sprintf(buffer,"%i %i m\n",x,y);
   page_add_data(pdf->last_page, buffer,strlen(buffer));
}

/*******************************************************************************/
void  mypdf_path_lineto(struct pdf_file *pdf, int32_t x, int32_t y) {
   char buffer[20];
   sprintf(buffer,"%i %i l\n",x,y);
   page_add_data(pdf->last_page, buffer,strlen(buffer));
}

/*******************************************************************************/
void  mypdf_path_close(struct pdf_file *pdf) {
   page_add_data(pdf->last_page, "h\n", 2);
}

/*******************************************************************************/
void  mypdf_path_stroke(struct pdf_file *pdf) {
   page_add_data(pdf->last_page, "S\n", 2);
}

/*******************************************************************************/
void  mypdf_path_fill(struct pdf_file *pdf) {
   page_add_data(pdf->last_page, "f\n", 2);
}
/*******************************************************************************/
void  mypdf_add_text(struct pdf_file *pdf, int32_t font_id, int32_t size, int32_t x, int32_t y, char *text) {
   char buffer[40];
   int i;
   int open_brackets = 0;
   page_add_data(pdf->last_page, " BT\n",4);
   sprintf(buffer,"   /F%i %i Tf\n", font_id, size);
   page_add_data(pdf->last_page, buffer, strlen(buffer));
   sprintf(buffer,"   %i %i Td\n",x,y);
   page_add_data(pdf->last_page, buffer, strlen(buffer));
   /* Super inefficient escaping of special codes */
   page_add_data(pdf->last_page, "   (", 4);
   for(i = 0; i < strlen(text); i++) {
     if(text[i] == ')') {
	if(open_brackets == 0) {
           page_add_data(pdf->last_page, "\\)",2);
        } else {
           page_add_data(pdf->last_page, ")",1);
	   open_brackets--;
        }
     } else if(text[i] == '\\') {
        page_add_data(pdf->last_page, "\\\\",2);
     } else if(text[i] == '(') {
        page_add_data(pdf->last_page, "(",1);
	open_brackets++;
     } else {
        page_add_data(pdf->last_page, text+i,1);
     }
   }
   page_add_data(pdf->last_page, ") Tj\n",5);
   page_add_data(pdf->last_page, " ET\n",4);
}


/*******************************************************************************/
void  mypdf_page_end(struct pdf_file *pdf) {
}

/*******************************************************************************/
static void catalog(struct pdf_file *pdf) {
   add_xref(pdf, pdf->catalog_obj_id, 0);
   fprintf(pdf->f, "%i 0 obj\n",            pdf->catalog_obj_id);
   fprintf(pdf->f, " << /Type /Catalog\n");
   fprintf(pdf->f, "    /Outline %i 0 R\n", pdf->outline_obj_id);
   fprintf(pdf->f, "    /Pages %i 0 R\n",   pdf->pages_obj_id);
   fprintf(pdf->f, " >>\n");
   fprintf(pdf->f, "endobj\n");
   fprintf(pdf->f, "\n");
}
/*******************************************************************************/
static void outlines(struct pdf_file *pdf) {
   add_xref(pdf, pdf->outline_obj_id, 0);
   fprintf(pdf->f, "%i 0 obj\n", pdf->outline_obj_id);
   fprintf(pdf->f, " << /Type /Outlines\n");
   fprintf(pdf->f, "    /Count 0\n");
   fprintf(pdf->f, " >>\n");
   fprintf(pdf->f, "endobj\n");
   fprintf(pdf->f, "\n");
}
/*******************************************************************************/
static void pageholder(struct pdf_file *pdf) {
   struct Page *p = pdf->first_page;
   int count = 0;

   add_xref(pdf, pdf->pages_obj_id, 0);
   fprintf(pdf->f, "%i 0 obj\n", pdf->pages_obj_id);
   fprintf(pdf->f, " << /Type /Pages\n");
   fprintf(pdf->f, "    /Kids [\n");
   while(p != NULL) {
      fprintf(pdf->f, "           %i 0 R\n", p->page_obj_id);
      count++;
      p = p->next;
   }
   fprintf(pdf->f, "    ]\n");
   fprintf(pdf->f, "    /Count %i\n", count);
   fprintf(pdf->f, " >>\n");
   fprintf(pdf->f, "endobj\n");
   fprintf(pdf->f, "\n");
}

/*******************************************************************************/
static void page_content(struct pdf_file *pdf, struct Page *page) {
   add_xref(pdf, page->content_obj_id, 0);
   fprintf(pdf->f, "%i 0 obj\n", page->content_obj_id);
   fprintf(pdf->f, " << /Length %li >>\n", strlen(page->data));
   fprintf(pdf->f, "stream\n");
   fputs(          page->data,                               pdf->f);
   fprintf(pdf->f, "endstream\n");
   fprintf(pdf->f, "endobj\n");
   fprintf(pdf->f, "\n");
}

/*******************************************************************************/
static void pages(struct pdf_file *pdf) {
   struct Page *page;
   page = pdf->first_page;
   
   while(page != NULL) { 
      add_xref(pdf, page->page_obj_id, 0);
      fprintf(pdf->f, "%i 0 obj\n", page->page_obj_id);
      fprintf(pdf->f, " << /Type /Page\n");
      fprintf(pdf->f, "    /Parent 3 0 R\n");
      fprintf(pdf->f, "    /MediaBox [0 0 %i %i]\n", page->width72dpi, page->height72dpi);
      fprintf(pdf->f, "    /Contents %i 0 R\n", page->content_obj_id);
      fprintf(pdf->f, "    /Resources << \n");
      fprintf(pdf->f, "         /ProcSet 6 0 R\n");

      struct Font *f = pdf->first_font;
      while(f != NULL) {
         fprintf(pdf->f, "         /Font << /F%i %i 0 R >>\n", f->font_id, f->font_obj_id);  
         f = f->next;
      }
      fprintf(pdf->f, "     >>\n");
      fprintf(pdf->f, " >>\n");
      fprintf(pdf->f, "endobj\n");
      fprintf(pdf->f, "\n");
      page_content(pdf, page);

      page = page->next;
   }
}

/*******************************************************************************/
static void doc_end_section(struct pdf_file *pdf) {
   add_xref(pdf, pdf->end_obj_id, 0);
   fprintf(pdf->f, "%i 0 obj\n", pdf->end_obj_id);
   fprintf(pdf->f, " [/PDF]\n");
   fprintf(pdf->f, "endobj\n");
   fprintf(pdf->f, "\n");
}

/*******************************************************************************/
static void fonts(struct pdf_file *pdf) {
   struct Font *f;
   f = pdf->first_font;
   while(f != NULL) { 
      add_xref(pdf, f->font_obj_id, 0);
      fprintf(pdf->f, "%i 0 obj\n", f->font_obj_id);
      fprintf(pdf->f, " << /Type /Font\n");
      fprintf(pdf->f, "    /Subtype /Type1\n");
      fprintf(pdf->f, "    /Name /F%i\n", f->font_id);
      fprintf(pdf->f, "    /BaseFont /Helvetica\n");
      fprintf(pdf->f, "    /Encoding /MacRomanEncoding\n");
      fprintf(pdf->f, " >>\n");
      fprintf(pdf->f, "endobj\n");
      fprintf(pdf->f, "\n");

      f = f->next;
   }
}

/*******************************************************************************/
/*******************************************************************************/
static void xref(struct pdf_file *pdf) {
   struct Xref *xref;
   pdf->xref_offset = ftell(pdf->f);
   fputs("xref\n",pdf->f);
   fprintf(pdf->f,"0 %i\n", pdf->next_obj_id+1);
   xref = pdf->first_xref;
   fputs("0000000000 65535 f \n",pdf->f);
   while(xref != NULL) {
      fprintf(pdf->f, "%010i %05i n \n",xref->address, xref->generation);
      xref = xref->next;
   }
   fputs("\n",                                     pdf->f);
}

/*******************************************************************************/
static void trailer(struct pdf_file *pdf) {
   fprintf(pdf->f, "trailer\n");
   fprintf(pdf->f, " << /Size %i\n", pdf->next_obj_id+1);
   fprintf(pdf->f, "    /Root 1 0 R\n");
   fprintf(pdf->f, " >>\n");
   fprintf(pdf->f, "startxref\n");
   fprintf(pdf->f, "%li\n",   pdf->xref_offset);
   fprintf(pdf->f, "%%%%EOF\n");
}

/*******************************************************************************/
struct pdf_file *mypdf_open(char *fname) {
   struct pdf_file *pdf;
   pdf = malloc(sizeof(struct pdf_file));
   if(pdf == NULL)
     return NULL;
   memset(pdf,0,sizeof(struct pdf_file));
   pdf->f = fopen(fname,"wb");
   if(pdf->f == NULL) {
     free(pdf);
     return NULL;
   }
   return pdf;
}
/*******************************************************************************/
void  mypdf_close(struct pdf_file *pdf) {

   if(pdf->f != NULL) {
      /* Assign section numbers */
      pdf->catalog_obj_id = next_obj_id(pdf);
      pdf->outline_obj_id = next_obj_id(pdf);
      pdf->pages_obj_id   = next_obj_id(pdf);
      struct Page *p = pdf->first_page;
      while(p != NULL) {
         p->page_obj_id     = next_obj_id(pdf);
         p->content_obj_id  = next_obj_id(pdf);;
         p = p->next;
      }
      pdf->end_obj_id   = next_obj_id(pdf);
      struct Font *f = pdf->first_font;
      while(f != NULL) {
         f->font_obj_id = next_obj_id(pdf);
         f = f->next;
      }

      fputs("%PDF-1.7\n", pdf->f);
      catalog(pdf);
      outlines(pdf);
      pageholder(pdf);
      pages(pdf);
      doc_end_section(pdf);
      fonts(pdf);
      xref(pdf);
      trailer(pdf);
      fclose(pdf->f);
      pdf->f = NULL; 
   }
   free(pdf);
}
/*******************************************************************************/
