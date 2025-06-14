#include </home/hy258/ics2024/navy-apps/libs/libbmp/include/BMP.h>
#include </home/hy258/ics2024/navy-apps/libs/libndl/include/NDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  NDL_Init(0);
  int w, h;
  // 检查BMP_Load中的路径是否正确
  printf("尝试加载图片: /share/pictures/projectn.bmp\n");
  void *bmp = BMP_Load("/share/pictures/projectn.bmp", &w, &h);
  printf("加载结果: %p, 尺寸: %d x %d\n", bmp, w, h);
  assert(bmp);
  NDL_OpenCanvas(&w, &h);
  NDL_DrawRect(bmp, 0, 0, w, h);
  free(bmp);
  NDL_Quit();
  printf("Test ends! Spinning...\n");
  while (1);
  return 0;
}
