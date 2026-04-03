/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "assetmgr.h"
#include <assert.h>

#include "bittype.h"
#include "chunkio.h"
#include "realcrc.h"

#include "wwdebug.h"

#include "htreemgr.h"
#include "hanimmgr.h"
#include "texture.h"
#include "font3d.h"
#include "render2dsentence.h"
#include "proto.h"
#include "hanim.h"
#include "hcanim.h"
#include "htree.h"
#include "collect.h"
#include "ww3d.h"
#include "ffactory.h"
#include "boxrobj.h"
#include "nullrobj.h"
#include "distlod.h"
#include "hlod.h"
#include "agg_def.h"
#include "wwstring.h"
#include "wwmemlog.h"
#include "dazzle.h"
#include "dx8wrapper.h"
#include "metalmap.h"
#include <ini.h>
#include <windows.h>
#include <stdio.h>
#include "ww3dformat.h"
#include "wwprofile.h"
#include "assetstatus.h"


/*
** Static member variable which keeps track of the single instanced asset manager
*/
WW3DAssetManager *		WW3DAssetManager::TheInstance = NULL;

/*
** Static instance of the Null prototype.  This render object is special cased
** to always be available...
*/
static NullPrototypeClass _NullPrototype;

/*
** Iterator for the Render Objects in the asset manager
*/
class RObjIterator : public RenderObjIterator
{
public:
	virtual bool					Is_Done(void);
	virtual const char *			Current_Item_Name(void);
	virtual int						Current_Item_Class_ID(void);
protected:
	friend class WW3DAssetManager;
};


/*
** Iterators for the other types of 3D assets:
** HAnims, HTrees, Textures, Fonts
*/
class HAnimIterator : public AssetIterator
{
public:
	HAnimIterator(void) : Iterator( WW3DAssetManager::Get_Instance()->HAnimManager ) { };

	virtual void			First(void) { Iterator.First(); }
	virtual void			Next(void)	{ Iterator.Next(); }
	virtual bool			Is_Done(void) { return Iterator.Is_Done(); }
	virtual const char *	Current_Item_Name(void) { return Iterator.Get_Current_Anim()->Get_Name(); }

protected:
	HAnimManagerIterator	Iterator;
	friend class WW3DAssetManager;
};

class HTreeIterator : public AssetIterator
{
public:
	virtual bool					Is_Done(void);
	virtual const char *			Current_Item_Name(void);
protected:
	friend class WW3DAssetManager;
};

class Font3DDataIterator : public AssetIterator
{
public:

	virtual void					First(void) { Node = WW3DAssetManager::Get_Instance()->Font3DDatas.Head(); }
	virtual void					Next(void)	{ Node = Node->Next(); }
	virtual bool					Is_Done(void) { return Node==NULL; }
	virtual const char *			Current_Item_Name(void) { return Node->Data()->Name; }

protected:

	Font3DDataIterator(void)	{ Node = WW3DAssetManager::Get_Instance()->Font3DDatas.Head(); }
	SLNode<Font3DDataClass> *		Node;
	friend class WW3DAssetManager;
};

WW3DAssetManager::WW3DAssetManager(void) :
	PrototypeLoaders		(PROTOLOADERS_VECTOR_SIZE),
	Prototypes				(PROTOTYPES_VECTOR_SIZE),
	TextureCache			(NULL),
	WW3D_Load_On_Demand	(false),
	Activate_Fog_On_Load	(false),
	MetalManager(0)
{
	assert(TheInstance == NULL);
	TheInstance = this;

	PrototypeLoaders.Set_Growth_Step(PROTOLOADERS_GROWTH_RATE);
	Prototypes.Set_Growth_Step(PROTOTYPES_GROWTH_RATE);

	Register_Prototype_Loader(&_MeshLoader);
	Register_Prototype_Loader(&_HModelLoader);
	Register_Prototype_Loader(&_CollectionLoader);
	Register_Prototype_Loader(&_BoxLoader);
	Register_Prototype_Loader(&_HLodLoader);
	Register_Prototype_Loader(&_DistLODLoader);
	Register_Prototype_Loader(&_AggregateLoader);
	Register_Prototype_Loader(&_NullLoader);
	Register_Prototype_Loader(&_DazzleLoader);
	
	PrototypeHashTable = new PrototypeClass * [PROTOTYPE_HASH_TABLE_SIZE];
	memset(PrototypeHashTable,0,sizeof(PrototypeClass *) * PROTOTYPE_HASH_TABLE_SIZE);		
}


WW3DAssetManager::~WW3DAssetManager(void)
{
	if (MetalManager) delete MetalManager;
	Free();
	TheInstance = NULL;

	if (PrototypeHashTable != NULL) {
		delete [] PrototypeHashTable;
		PrototypeHashTable = NULL;
	}
}

static void Create_Number_String(StringClass& number, unsigned value)
{
	unsigned miljoonat=value/(1024*1028);
	unsigned tuhannet=(value/1024)%1024;
	unsigned ykkoset=value%1024;
	if (miljoonat) {
		number.Format("%d %3.3d %3.3d",miljoonat,tuhannet,ykkoset);
	}
	else if (tuhannet) {
		number.Format("%d %3.3d",tuhannet,ykkoset);
	}
	else {
		number.Format("%d",ykkoset);
	}
}

void	WW3DAssetManager::Load_Procedural_Textures()
{
	int i,count;
	if (!MetalManager)
	{
		INIClass ini;
		ini.Load("metals.ini");
		MetalManager=new MetalMapManagerClass(ini);
	}
	
	count=MetalManager->Metal_Map_Count();		
	for (i=0; i<count; i++)
	{
		TextureClass *tex=MetalManager->Get_Metal_Map(i);
		TextureHash.Insert(tex->Get_Texture_Name(),tex);
	}	
}

static const char * WW3DFormat_Name(WW3DFormat fmt)
{
	switch (fmt) {
	case WW3D_FORMAT_A8R8G8B8:	return "WW3D_FORMAT_A8R8G8B8";
	case WW3D_FORMAT_R8G8B8:	return "WW3D_FORMAT_R8G8B8";
	case WW3D_FORMAT_A4R4G4B4:	return "WW3D_FORMAT_A4R4G4B4";
	case WW3D_FORMAT_A1R5G5B5:	return "WW3D_FORMAT_A1R5G5B5";
	case WW3D_FORMAT_R5G6B5:	return "WW3D_FORMAT_R5G6B5";
	case WW3D_FORMAT_L8:		return "WW3D_FORMAT_L8";
	case WW3D_FORMAT_A8:		return "WW3D_FORMAT_A8";
	case WW3D_FORMAT_P8:		return "WW3D_FORMAT_P8";
	case WW3D_FORMAT_X8R8G8B8:	return "WW3D_FORMAT_X8R8G8B8";
	case WW3D_FORMAT_X1R5G5B5:	return "WW3D_FORMAT_X1R5G5B5";
	case WW3D_FORMAT_R3G3B2:	return "WW3D_FORMAT_R3G3B2";
	case WW3D_FORMAT_A8R3G3B2:	return "WW3D_FORMAT_A8R3G3B2";
	case WW3D_FORMAT_X4R4G4B4:	return "WW3D_FORMAT_X4R4G4B4";
	case WW3D_FORMAT_A8P8:		return "WW3D_FORMAT_A8P8";
	case WW3D_FORMAT_A8L8:		return "WW3D_FORMAT_A8L8";
	case WW3D_FORMAT_A4L4:		return "WW3D_FORMAT_A4L4";
	case WW3D_FORMAT_U8V8:		return "WW3D_FORMAT_U8V8";
	case WW3D_FORMAT_L6V5U5:	return "WW3D_FORMAT_L6V5U5";
	case WW3D_FORMAT_X8L8V8U8:	return "WW3D_FORMAT_X8L8V8U8";
	case WW3D_FORMAT_DXT1:		return "WW3D_FORMAT_DXT1";
	case WW3D_FORMAT_DXT2:		return "WW3D_FORMAT_DXT2";
	case WW3D_FORMAT_DXT3:		return "WW3D_FORMAT_DXT3";
	case WW3D_FORMAT_DXT4:		return "WW3D_FORMAT_DXT4";
	case WW3D_FORMAT_DXT5:		return "WW3D_FORMAT_DXT5";
	default:					return "Unknown";
	}
}

static void Log_Textures(bool inited, unsigned& total_count, unsigned& total_mem)
{
	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass * tex=ite.Peek_Value();
		if (tex->Is_Initialized()!=inited) continue;

		WW3DFormat fmt = tex->Get_Texture_Format();
		int width = tex->Get_Width();
		int height = tex->Get_Height();
		const char *tex_format = WW3DFormat_Name(fmt);

		unsigned texmem=tex->Get_Texture_Memory_Usage();
		total_mem+=texmem;
		total_count++;
		StringClass number;
		Create_Number_String(number,texmem);

		WWDEBUG_SAY(("%32s	%4d * %4d (%15s), init %d, size: %14s bytes, refs: %d\n",
			tex->Get_Texture_Name(),
			width,
			height,
			tex_format,
			tex->Is_Initialized(),
			number,
			tex->Num_Refs()));

	}	
}

void WW3DAssetManager::Log_Texture_Statistics()
{
	unsigned total_initialized_tex_mem=0;
	unsigned total_uninitialized_tex_mem=0;
	unsigned total_initialized_count=0;
	unsigned total_uninitialized_count=0;
	StringClass number;

	WWDEBUG_SAY(("\nInitialized textures ------------------------------------------\n\n"));
	Log_Textures(true,total_initialized_count,total_initialized_tex_mem);

	Create_Number_String(number,total_initialized_tex_mem);
	WWDEBUG_SAY(("\n%d initialized textures, totalling %14s bytes\n\n",
		total_initialized_count,
		number));

	WWDEBUG_SAY(("\nUn-initialized textures ---------------------------------------\n\n"));
	Log_Textures(false,total_uninitialized_count,total_uninitialized_tex_mem);

	Create_Number_String(number,total_uninitialized_tex_mem);
	WWDEBUG_SAY(("\n%d un-initialized textures, totalling, totalling %14s bytes\n\n",
		total_uninitialized_count,
		number));
}


void WW3DAssetManager::Free(void)
{
	Free_Assets();
}


void WW3DAssetManager::Free_Assets(void)
{
	WWPROFILE( "WW3DAssetManager::Free_Assets" );

	int count = Prototypes.Count();
	while (count-- > 0) {

		PrototypeClass * proto = Prototypes[count];
		Prototypes.Delete(count);

		if (proto != NULL) {
			delete proto;
		}
	}
	
	memset(PrototypeHashTable,0,sizeof(PrototypeClass *) * PROTOTYPE_HASH_TABLE_SIZE);	

	HAnimManager.Free_All_Anims();
	HTreeManager.Free_All_Trees();

	Release_All_Textures();
	Release_All_Font3DDatas();
	Release_All_FontChars();
}


void WW3DAssetManager::Release_Unused_Assets(void)
{
	Release_Unused_Textures();
	Release_Unused_Font3DDatas();
}


bool WW3DAssetManager::Load_3D_Assets( const char * filename )
{
	bool result = false;

	FileClass * file = _TheFileFactory->Get_File( filename );
	if ( file ) {
		if ( file->Is_Available() ) {
			result = WW3DAssetManager::Load_3D_Assets( *file );
		}
		_TheFileFactory->Return_File( file );
	}

	return result;
}


bool WW3DAssetManager::Load_3D_Assets(FileClass & w3dfile)
{
	WWPROFILE( "WW3DAssetManager::Load_3D_Assets" );
	if (!w3dfile.Open()) {
		return false;
	}

	ChunkLoadClass cload(&w3dfile);

	while (cload.Open_Chunk()) {

		switch (cload.Cur_Chunk_ID()) {

			case W3D_CHUNK_HIERARCHY:
				HTreeManager.Load_Tree(cload);
				break;

			case W3D_CHUNK_ANIMATION:
			case W3D_CHUNK_COMPRESSED_ANIMATION:
			case W3D_CHUNK_MORPH_ANIMATION:
				HAnimManager.Load_Anim(cload);
				break;
        
			default:
				Load_Prototype(cload);
				break;
		}

		cload.Close_Chunk();
	}

	w3dfile.Close();

	return true;
}


bool WW3DAssetManager::Load_Prototype(ChunkLoadClass & cload)
{
	WWPROFILE( "WW3DAssetManager::Load_Prototype" );
	WWMEMLOG(MEM_GEOMETRY);

	int chunk_id = cload.Cur_Chunk_ID();

	PrototypeLoaderClass * loader = Find_Prototype_Loader(chunk_id);
	PrototypeClass * newproto = NULL;

	if (loader != NULL) {

		newproto = loader->Load_W3D(cload);

	} else {

		WWDEBUG_SAY(("Unknown chunk type encountered!  Chunk Id = %d\r\n",chunk_id));
		return false;
	}

	if (newproto != NULL) {

		if (!Render_Obj_Exists(newproto->Get_Name())) {
				
			Add_Prototype(newproto);

		} else {

			WWDEBUG_SAY(("Render Object Name Collision: %s\r\n",newproto->Get_Name()));
			delete newproto;
			newproto = NULL;
			return false;
		}
	
	} else {

		WWDEBUG_SAY(("Could not generate prototype!  Chunk  = %d\r\n",chunk_id));
		return false;
	}
	
	return true;
}


RenderObjClass * WW3DAssetManager::Create_Render_Obj(const char * name)
{
	WWPROFILE( "WW3DAssetManager::Create_Render_Obj" );
	WWMEMLOG(MEM_GEOMETRY);

	PrototypeClass * proto = Find_Prototype(name);

	if (WW3D_Load_On_Demand && proto == NULL) {
		AssetStatusClass::Peek_Instance()->Report_Load_On_Demand_RObj(name);

		char filename [MAX_PATH];
		char *mesh_name = ::strchr (name, '.');
		if (mesh_name != NULL) {
			int len = (int)(mesh_name - name) + 1;
			::lstrcpyn (filename, name, len);
			::lstrcat (filename, ".w3d");
		} else {
			sprintf( filename, "%s.w3d", name);
		}

		if ( Load_3D_Assets( filename ) == false ) {
			StringClass	new_filename(StringClass("..\\"),true);
			new_filename+=filename;
			Load_3D_Assets( new_filename );
		}

		proto = Find_Prototype(name);
	}

	if (proto == NULL) {
		AssetStatusClass::Peek_Instance()->Report_Missing_RObj(name);
		return NULL;
	}

	return proto->Create();
}


bool WW3DAssetManager::Render_Obj_Exists(const char * name)
{
	if (Find_Prototype(name) == NULL) return false;
	else return true;
}


RenderObjIterator * WW3DAssetManager::Create_Render_Obj_Iterator(void)
{
	return new RObjIterator();
}


void WW3DAssetManager::Release_Render_Obj_Iterator(RenderObjIterator * it)
{
	WWASSERT(it != NULL);
	delete it;
}

AssetIterator * WW3DAssetManager::Create_HAnim_Iterator(void)
{
	return new HAnimIterator();
}														


AssetIterator * WW3DAssetManager::Create_HTree_Iterator(void)
{
	return new HTreeIterator();
}


AssetIterator * WW3DAssetManager::Create_Font3DData_Iterator(void)
{
	return new Font3DDataIterator();
}

HAnimClass *	WW3DAssetManager::Get_HAnim(const char * name)
{
	WWPROFILE( "WW3DAssetManager::Get_HAnim" );

	HAnimClass * anim = HAnimManager.Get_Anim(name);

	if (WW3D_Load_On_Demand && anim == NULL) {
		
		if ( !HAnimManager.Is_Missing( name ) ) {

			AssetStatusClass::Peek_Instance()->Report_Load_On_Demand_HAnim(name);

			char filename[ MAX_PATH ];
			char *animname = strchr( name, '.');
			if (animname != NULL) {
				sprintf( filename, "%s.w3d", animname+1);
			} else {
				WWDEBUG_SAY(( "Animation %s has no . in the name\n", name ));
				WWASSERT( 0 );
				return NULL;
			}

			if ( Load_3D_Assets( filename ) == false ) {
				StringClass	new_filename = StringClass("..\\") + filename;
				Load_3D_Assets( new_filename );
			}

			anim = HAnimManager.Get_Anim(name);
			if (anim == NULL) {
				HAnimManager.Register_Missing( name );
				AssetStatusClass::Peek_Instance()->Report_Missing_HAnim(name);
			}
		}
	}

	return anim;
}										 


HTreeClass *	WW3DAssetManager::Get_HTree(const char * name)
{
	WWPROFILE( "WW3DAssetManager::Get_HTree" );

	HTreeClass * htree = HTreeManager.Get_Tree(name);

	if (WW3D_Load_On_Demand && htree == NULL) {
		
		AssetStatusClass::Peek_Instance()->Report_Load_On_Demand_HTree(name);

		char filename[ MAX_PATH ];
		sprintf( filename, "%s.w3d", name);

		if ( Load_3D_Assets( filename ) == false ) {
			StringClass	new_filename("..\\",true);
			new_filename+=filename;
			Load_3D_Assets( new_filename );
		}

		htree = HTreeManager.Get_Tree(name);

		if (htree == NULL) {
			AssetStatusClass::Peek_Instance()->Report_Missing_HTree(name);
		}
	}

	return htree;
}

TextureClass * WW3DAssetManager::Get_Texture(
	const char * filename, 
	TextureClass::MipCountType mip_level_count,
	WW3DFormat texture_format,
	bool allow_compression)
{
	WWPROFILE( "WW3DAssetManager::Get_Texture 1" );

	if (texture_format==WW3D_FORMAT_U8V8) {
		mip_level_count=TextureClass::MIP_LEVELS_1;
	}

	if ((filename == NULL) || (strlen(filename) == 0)) {
		return NULL;
	}

	StringClass lower_case_name(filename,true);
	_strlwr(lower_case_name.Peek_Buffer());

	TextureClass* tex = TextureHash.Get(lower_case_name);
	if (tex && (tex->Is_Initialized() == true) && (texture_format!=WW3D_FORMAT_UNKNOWN)) {
		WWASSERT_PRINT(tex->Get_Texture_Format()==texture_format,("Texture %s has already been loaded with different format",filename));
	}

	if (!tex) {
		tex = NEW_REF(TextureClass,(lower_case_name, NULL, mip_level_count, texture_format, allow_compression));
		TextureHash.Insert(tex->Get_Texture_Name(),tex);
	}

	tex->Add_Ref();
	return tex;
}


void WW3DAssetManager::Release_All_Textures(void)
{
	HashTemplateIterator<StringClass,TextureClass*> ite(TextureHash);
	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass * tex=ite.Peek_Value();
		tex->Release_Ref();
	}
	TextureHash.Remove_All();
}


void WW3DAssetManager::Release_Unused_Textures(void)
{
	unsigned count=0;
	TextureClass* temp_textures[256];

	HashTemplateIterator<StringClass,TextureClass*> ite(TextureHash);
	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass* tex=ite.Peek_Value();
		if (tex->Num_Refs() == 1) {
			temp_textures[count++]=tex;
			if (count==256) {
				for (unsigned i=0;i<256;++i) {
					TextureHash.Remove(temp_textures[i]->Get_Texture_Name());
					temp_textures[i]->Release_Ref();
				}
				count=0;
				ite.First();
			}
		}
	}
	for (unsigned i=0;i<count;++i) {
		TextureHash.Remove(temp_textures[i]->Get_Texture_Name());
		temp_textures[i]->Release_Ref();
	}
}

void WW3DAssetManager::Release_Texture(TextureClass *tex)
{
	TextureHash.Remove(tex->Get_Texture_Name());
	tex->Release_Ref();
}

void WW3DAssetManager::Log_All_Textures(void)
{
	Log_Texture_Statistics();

	HashTemplateIterator<StringClass,TextureClass*> ite(TextureHash);

	WWDEBUG_SAY((
		"Lightmap textures: %d\n\n"
		"size     name\n"
		"--------------------------------------\n"
		,
		TextureClass::_Get_Total_Lightmap_Texture_Count()));

	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass* t=ite.Peek_Value();
		if (!t->Is_Lightmap()) continue;

		StringClass tmp(true);
		unsigned bytes=t->Get_Texture_Memory_Usage();
		if (!t->Is_Initialized()) {
			tmp+="*";
		}
		else {
			tmp+=" ";
		}
		WWDEBUG_SAY(("%4.4dkb %s%s\n",bytes/1024,tmp,t->Get_Texture_Name()));
	}

	WWDEBUG_SAY((
		"Procedural textures: %d\n\n"
		"size     name\n"
		"--------------------------------------\n"
		,
		TextureClass::_Get_Total_Procedural_Texture_Count()));

	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass* t=ite.Peek_Value();
		if (!t->Is_Procedural()) continue;

		StringClass tmp(true);
		unsigned bytes=t->Get_Texture_Memory_Usage();
		if (!t->Is_Initialized()) {
			tmp+="*";
		}
		else {
			tmp+=" ";
		}
		WWDEBUG_SAY(("%4.4dkb %s%s\n",bytes/1024,tmp,t->Get_Texture_Name()));
	}

	WWDEBUG_SAY((
		"Ordinary textures: %d\n\n"
		"size     name\n"
		"--------------------------------------\n"
		,
		TextureClass::_Get_Total_Texture_Count()-TextureClass::_Get_Total_Lightmap_Texture_Count()-TextureClass::_Get_Total_Procedural_Texture_Count()));

	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass* t=ite.Peek_Value();
		if (t->Is_Procedural()) continue;
		if (t->Is_Lightmap()) continue;

		StringClass tmp(true);
		unsigned bytes=t->Get_Texture_Memory_Usage();
		if (!t->Is_Initialized()) {
			tmp+="*";
		}
		else {
			tmp+=" ";
		}
		WWDEBUG_SAY(("%4.4dkb %s%s\n",bytes/1024,tmp,t->Get_Texture_Name()));
	}

}



Font3DInstanceClass * WW3DAssetManager::Get_Font3DInstance( const char *name )
{
	WWPROFILE( "WW3DAssetManager::Get_Font3DInstance" );
	return NEW_REF( Font3DInstanceClass, ( name ));
}


Font3DDataClass * WW3DAssetManager::Get_Font3DData( const char *name )
{
	WWPROFILE( "WW3DAssetManager::Get_Font3DData" );
	for (	SLNode<Font3DDataClass> *node = Font3DDatas.Head(); node; node = node->Next()) {
		if (!stricmp(name, node->Data()->Name)) {
			node->Data()->Add_Ref();
			return node->Data();
		}
	}

	Font3DDataClass * font = NEW_REF( Font3DDataClass, ( name ));

	Add_Font3DData( font);

	return font;
}

void WW3DAssetManager::Add_Font3DData(Font3DDataClass * font)
{
	font->Add_Ref();
	Font3DDatas.Add_Head(font);
}

void WW3DAssetManager::Remove_Font3DData(Font3DDataClass * font)
{
	font->Release_Ref();
	Font3DDatas.Remove(font);
}

void	WW3DAssetManager::Release_All_Font3DDatas( void )
{
	Font3DDataClass *head;
	while ((head = Font3DDatas.Remove_Head()) != NULL )	{
		head->Release_Ref();
	}
}

void	WW3DAssetManager::Release_Unused_Font3DDatas( void )
{
	SLNode<Font3DDataClass> *node, * next;
	for (	node = Font3DDatas.Head(); node; node = next) {
		next = node->Next();
		Font3DDataClass *font = node->Data();
		if (font->Num_Refs() == 1) {
			Font3DDatas.Remove(font);
			font->Release_Ref();
		}
	}
}

FontCharsClass *	WW3DAssetManager::Get_FontChars( const char * name, int point_size, bool is_bold )
{
	WWPROFILE( "WW3DAssetManager::Get_FontChars" );

	for ( int i = 0; i < FontCharsList.Count(); i++ ) {
		if ( FontCharsList[i]->Is_Font( name, point_size, is_bold ) ) {
			FontCharsList[i]->Add_Ref();
			return FontCharsList[i];
		}
	}

	FontCharsClass * font = NEW_REF( FontCharsClass, () );
	font->Initialize_GDI_Font( name, point_size, is_bold );
	font->Add_Ref();
	FontCharsList.Add( font );
	return font;
}


void	WW3DAssetManager::Release_All_FontChars( void )
{
	while ( FontCharsList.Count() ) {
		FontCharsList[0]->Release_Ref();
		FontCharsList.Delete( 0 );
	}
}

void	WW3DAssetManager::Release_Unused_FontChars( void )
{
	for ( int index = FontCharsList.Count() - 1; index >= 0; index-- ) {
		FontCharsClass * font = FontCharsList[index];
		if ( font->Num_Refs() == 1 ) {
			FontCharsList.Delete(index);
			font->Release_Ref();
		}
	}
}

void WW3DAssetManager::Register_Prototype_Loader(PrototypeLoaderClass * loader)
{
	WWASSERT(loader != NULL);
	PrototypeLoaders.Add(loader);
}


PrototypeLoaderClass * WW3DAssetManager::Find_Prototype_Loader(int chunk_id)
{
	for (int i=0; i<PrototypeLoaders.Count(); i++) {
		PrototypeLoaderClass * loader = PrototypeLoaders[i];
		if (loader && loader->Chunk_Type() == chunk_id) {
			return loader;
		}
	}
	return NULL;
}


void WW3DAssetManager::Add_Prototype(PrototypeClass * newproto)
{
	WWASSERT(newproto != NULL);
	int hash = CRC_Stringi(newproto->Get_Name()) & PROTOTYPE_HASH_MASK;
	newproto->NextHash = PrototypeHashTable[hash];
	PrototypeHashTable[hash] = newproto;
	Prototypes.Add(newproto);
}


void WW3DAssetManager::Remove_Prototype(PrototypeClass *proto)
{
	WWASSERT(proto != NULL);
	if (proto != NULL) {

		const char *pname = proto->Get_Name ();
		bool bfound = false;
		PrototypeClass *prev = NULL;
		int hash = CRC_Stringi(pname) & PROTOTYPE_HASH_MASK;				
		for (PrototypeClass *test = PrototypeHashTable[hash];
			  (test != NULL) && (bfound == false);
			  test = test->NextHash) {
			
			if (::stricmp (test->Get_Name(), pname) == 0) {
				
				if (prev == NULL) {
					PrototypeHashTable[hash] = test->NextHash;
				} else {
					prev->NextHash = test->NextHash;
				}

				bfound = true;
			}

			prev = test;
		}

		Prototypes.Delete (proto);
	}

	return;
}


void WW3DAssetManager::Remove_Prototype(const char *name)
{
	WWASSERT(name != NULL);
	if (name != NULL) {

		PrototypeClass *proto = Find_Prototype (name);
		if (proto != NULL) {

			Remove_Prototype (proto);
			delete proto;
		}
	}

	return;
}


PrototypeClass * WW3DAssetManager::Find_Prototype(const char * name)
{
	if (stricmp(name,"NULL") == 0) {
		return &(_NullPrototype);
	}
	
	int hash = CRC_Stringi(name) & PROTOTYPE_HASH_MASK;
	PrototypeClass * test = PrototypeHashTable[hash];

	while (test != NULL) {
		if (stricmp(test->Get_Name(),name) == 0) {
			return test;
		}
		test = test->NextHash;
	}
	return NULL;
}

/*
** Iterator Implementations.
*/

bool RObjIterator::Is_Done(void)
{
	return !(Index < WW3DAssetManager::Get_Instance()->Prototypes.Count());
}

const char * RObjIterator::Current_Item_Name(void)
{
	if (Index < WW3DAssetManager::Get_Instance()->Prototypes.Count()) {
		return WW3DAssetManager::Get_Instance()->Prototypes[Index]->Get_Name();
	} else {
		return NULL;
	}
}

int RObjIterator::Current_Item_Class_ID(void)
{
	if (Index < WW3DAssetManager::Get_Instance()->Prototypes.Count()) {
		return WW3DAssetManager::Get_Instance()->Prototypes[Index]->Get_Class_ID();
	} else {
		return -1;
	}
}

bool HTreeIterator::Is_Done(void)
{
	return !(Index < WW3DAssetManager::Get_Instance()->HTreeManager.Num_Trees());
}

const char * HTreeIterator::Current_Item_Name(void)
{
	return WW3DAssetManager::Get_Instance()->HTreeManager.Get_Tree(Index)->Get_Name();
}
