#pragma once

class IndexBufferClass;
class VertexBufferClass;
class DX8RenderTypeArrayClass;
class MeshClass;
class MeshModelClass;
class DX8PolygonRendererClass;
class Vertex_Split_Table;
class DX8FVFCategoryContainer;
class DecalMeshClass;
class MaterialPassClass;
class MatPassTaskClass;
class PolyRenderTaskClass;
class TextureClass;
class VertexMaterialClass;
class CameraClass;

class DX8MeshRendererCompatClass
{
public:
	void Invalidate()
	{
	}

	static void Request_Log_Statistics()
	{
	}
};

static DX8MeshRendererCompatClass TheDX8MeshRenderer;
