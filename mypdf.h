struct pdf_file;

struct pdf_file *mypdf_open(char *fname);
void  mypdf_close(struct pdf_file *pdf);

int  mypdf_add_font(struct pdf_file *pdf);
void mypdf_add_text(struct pdf_file *pdf, int font_id, int32_t x, int32_t y, char *text);

int   mypdf_page_begin(struct pdf_file *pdf);
void  mypdf_page_end(struct pdf_file *pdf);

int   mypdf_page_set_size(struct pdf_file *pdf, int32_t width,  int32_t height);
int   mypdf_page_get_size(struct pdf_file *pdf, int32_t *width, int32_t *height);

void  mypdf_path_moveto(struct pdf_file *pdf, int32_t x, int32_t y);
void  mypdf_path_lineto(struct pdf_file *pdf, int32_t x, int32_t y);
void  mypdf_path_close(struct pdf_file *pdf);
void  mypdf_path_stroke(struct pdf_file *pdf);
void  mypdf_path_fill(struct pdf_file *pdf);


