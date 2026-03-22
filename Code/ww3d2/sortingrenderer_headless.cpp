#include "sortingrenderer.h"

bool SortingRendererClass::_EnableTriangleDraw = true;

void SortingRendererClass::Flush_Sorting_Pool() {}
void SortingRendererClass::Insert_To_Sorting_Pool(SortingNodeStruct *) {}
void SortingRendererClass::Insert_Triangles(const SphereClass &, unsigned short, unsigned short, unsigned short, unsigned short) {}
void SortingRendererClass::Insert_Triangles(unsigned short, unsigned short, unsigned short, unsigned short) {}
void SortingRendererClass::Flush() {}
void SortingRendererClass::Deinit() {}
